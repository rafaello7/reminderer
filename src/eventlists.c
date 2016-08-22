#include "reminderer.h"
#include <glib/gstdio.h>
#include <string.h>
#include "eventstore.h"
#include "eventlists.h"
#include "preferences.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


static GHashTable *g_events;
static GtkListStore *g_eventList, *g_remindList, *g_previewList, *g_todoList;
static gboolean g_showDeleted = FALSE;


struct evl_remind;



struct evl_event { /* g_eventList and g_todoList PTR parameter */
	reminderer_event re;
    struct evl_remind **reminds;
    gint remind_count;
    gboolean isOnEventList;     /* whether the item is on g_eventList */
    gboolean isOnTodoList;      /* whether the item is on g_todoList */
	GtkTreeIter iterEv;	        /* position on g_eventList */
	GtkTreeIter iterTodo;       /* position on g_todoList */
};

struct evl_remind { /* g_remindList PTR parameter */
    reminderer_remind rr;
    struct evl_event *ev;
    gboolean isOnRmdList;       /* whether the item is on g_remindList */
	GtkTreeIter iterRmd;		/* position of this remind on g_remindList */
    gboolean isOnPvwList;       /* whether the item is on g_previewList */
	GtkTreeIter iterPvw;        /* position of this remind on g_previewList */
};

static const GdkRGBA *adjBrightness(const char *colorSpec)
{
    static GdkRGBA rgba;
    gdouble lightenFactor = prefs_coloredBgLighten() / 100.0;

    if( ! gdk_rgba_parse(&rgba, colorSpec) ) {
        rgba.red = rgba.green = rgba.blue = rgba.alpha = 1.0;
    }
    rgba.red = lightenFactor + rgba.red * (1 - lightenFactor);
    rgba.green = lightenFactor + rgba.green * (1 - lightenFactor);
    rgba.blue = lightenFactor + rgba.blue * (1 - lightenFactor);
    return &rgba;
}

static const GdkRGBA *ndays_to_color(gboolean deleted, int ndays)
{
    const char *colorSpec;

    if( deleted ) {
        colorSpec = "gray";
    }else{
        switch( ndays ) {
            case 6: colorSpec = "cyan";         break;
            case 5: colorSpec = "medium spring green"; break;
            case 4: colorSpec = "green1";       break;
            case 3: colorSpec = "lawn green";   break;
            case 2: colorSpec = "yellow";       break;
            case 1: colorSpec = "gold";         break;
            case 0: colorSpec = "dark orange";  break;
            default:
                colorSpec = ndays < 0 ? "red" : "deep sky blue";
                break;
        }
    }
    return adjBrightness(colorSpec);
}

static const GdkRGBA *type_to_color(const reminderer_event *re)
{
    const char *colorSpec;

    if( re->evDeleted ) {
        colorSpec = "gray";
    }else if( re->evType == RET_BYDATE ) {
        /* designer color scheme id 2q409w0vWmpF3 */
        if( re->evByDateMonths == 0 || re->evByDateDays == 0 ||
                re->evByDateWeekdays == 0 )
        {
            colorSpec = "light gray";
        }else if( re->evByDateYear ) {
            colorSpec = "#9498E4";
        }else if( re->evByDateMonths != 0xFFF ) {
            colorSpec = "#D1F893";
        }else if( re->evByDateDays != 0x7FFFFFFF ) {
            colorSpec = "#FFE497";
        }else if( re->evByDateWeekdays != 0x7F ) {
            colorSpec = "#FEBC00";
        }else{
            colorSpec = "#F08EBE";
        }
    }else{
        colorSpec = "#D30067";
    }
    return adjBrightness(colorSpec);
}

static const GdkRGBA *todo_to_color(gboolean deleted, gboolean keep)
{
    return adjBrightness(deleted ? "gray" : keep ? "goldenrod" : "gold");
}

static const char *event_type_short(const reminderer_event *re)
{
    if( re->evType == RET_BYDATE ) {
        if( re->evByDateMonths == 0 || re->evByDateDays == 0 ||
                re->evByDateWeekdays == 0 )
            return "-";
        if( re->evByDateYear )
            return _("S");  /* Single */
        if( re->evByDateMonths != 0xFFF )
            return _("A");  /* Annual */
        if( re->evByDateDays != 0x7FFFFFFF )
            return _("M");  /* Monthly */
        if( re->evByDateWeekdays != 0x7F )
            return _("W");  /* Weekly */
        return _("D");      /* Daily */
    }
    return _("P");
}

static void updateRemindCols(evl_remind_id rmd)
{
	GDate date;
	gint daysTo;
	gchar fmtDays[20], fmtDate[50];
    gboolean showOnRmdList, showOnPvwList, showAsDeleted;
    const reminderer_remind *rr;
    const reminderer_event *re;

    if( rmd == NULL )
        return;
    rr = &rmd->rr;
    re = &rmd->ev->re;
    if( rr->day != 0 ) {
        g_date_clear(&date, 1);
        g_date_set_dmy(&date, rr->day, rr->month, rr->year);
        daysTo = g_date_days_between(evs_get_cur_date(), &date);
        g_sprintf(fmtDays, "%d", daysTo);
        if( daysTo >= 365 )
            g_date_strftime(fmtDate, sizeof(fmtDate), "%a %d %b %Y", &date);
        else
            g_date_strftime(fmtDate, sizeof(fmtDate), "%a %d %b", &date);
        showOnPvwList = !re->evDeleted && (g_showDeleted || ! rr->rrDeleted);
        showOnRmdList = showOnPvwList && daysTo <= re->remind_days;
    }else{
        daysTo = 0;
        showOnPvwList = g_showDeleted || (!re->evDeleted && ! rr->rrDeleted);
        showOnRmdList = showOnPvwList;
    }
    showAsDeleted = rr->rrDeleted || re->evDeleted;
    if( showOnRmdList ) {
        if( ! rmd->isOnRmdList ) {
            gtk_list_store_append(g_remindList, &rmd->iterRmd);
            gtk_list_store_set(g_remindList, &rmd->iterRmd, EVL_RMDCOL_PTR,
                    rmd, -1);
            rmd->isOnRmdList = TRUE;
        }
        if( rr->day != 0 ) {
            gtk_list_store_set(g_remindList, &rmd->iterRmd,
                    EVL_RMDCOL_COLOR,   ndays_to_color(showAsDeleted, daysTo),
                    EVL_RMDCOL_DAYS,    fmtDays,
                    EVL_RMDCOL_DATE,    fmtDate,
                    EVL_RMDCOL_TEXT,    re->text,
                    EVL_RMDCOL_DELETED, showAsDeleted, -1);
        }else{
            gtk_list_store_set(g_remindList, &rmd->iterRmd,
                    EVL_RMDCOL_COLOR,   todo_to_color(showAsDeleted, re->keep),
                    EVL_RMDCOL_DAYS,    "*",
                    EVL_RMDCOL_DATE,    "",
                    EVL_RMDCOL_TEXT,    re->text,
                    EVL_RMDCOL_DELETED, showAsDeleted, -1);
        }
    }else{
        if( rmd->isOnRmdList ) {
            gtk_list_store_remove(g_remindList, &rmd->iterRmd);
            rmd->isOnRmdList = FALSE;
        }
    }
    if( showOnPvwList ) {
        if( ! rmd->isOnPvwList ) {
            gtk_list_store_append(g_previewList, &rmd->iterPvw);
            gtk_list_store_set(g_previewList, &rmd->iterPvw, EVL_RMDCOL_PTR,
                    rmd, -1);
            rmd->isOnPvwList = TRUE;
        }
        if( rr->day != 0 ) {
            gtk_list_store_set(g_previewList, &rmd->iterPvw,
                    EVL_RMDCOL_COLOR,   ndays_to_color(showAsDeleted, daysTo),
                    EVL_RMDCOL_DAYS,    fmtDays,
                    EVL_RMDCOL_DATE,    fmtDate,
                    EVL_RMDCOL_TEXT,    re->text,
                    EVL_RMDCOL_DELETED, showAsDeleted, -1);
        }else{
            gtk_list_store_set(g_previewList, &rmd->iterPvw,
                    EVL_RMDCOL_COLOR,   todo_to_color(showAsDeleted, re->keep),
                    EVL_RMDCOL_DAYS,    "*",
                    EVL_RMDCOL_DATE,    "",
                    EVL_RMDCOL_TEXT,    re->text,
                    EVL_RMDCOL_DELETED, showAsDeleted, -1);
        }
    }else{
        if( rmd->isOnPvwList ) {
            gtk_list_store_remove(g_previewList, &rmd->iterPvw);
            rmd->isOnPvwList = FALSE;
        }
    }
}

/* Stores event on appropriate list and updates list columns with fresh values;
 * note that event may change list which belongs to
 */
static void updateEventCols(evl_event_id ev)
{
	gchar fmtbuf[50], *formatted;
	GDate date;
    int i;
    const reminderer_event *re;
    gboolean isMulti;

    if( ev == NULL )
        return;
    re = &ev->re;
    if( g_showDeleted || !re->evDeleted ) {
        if( re->evType == RET_TODO ) {
            if( ev->isOnEventList ) {
                gtk_list_store_remove(g_eventList, &ev->iterEv);
                ev->isOnEventList = FALSE;
            }
            if( ! ev->isOnTodoList ) {
                gtk_list_store_append(g_todoList, &ev->iterTodo);
                gtk_list_store_set(g_todoList, &ev->iterTodo,
                        EVL_TODOCOL_PTR, ev, -1);
                ev->isOnTodoList = TRUE;
            }
            gtk_list_store_set(g_todoList, &ev->iterTodo,
                    EVL_TODOCOL_COLOR, todo_to_color(re->evDeleted, re->keep),
                    EVL_TODOCOL_MARK, re->keep ? "*" : "",
                    EVL_TODOCOL_TEXT, re->text,
                    EVL_TODOCOL_DELETED, re->evDeleted, -1);
        }else{
            if( ev->isOnTodoList ) {
                gtk_list_store_remove(g_todoList, &ev->iterTodo);
                ev->isOnTodoList = FALSE;
            }
            if( !ev->isOnEventList ) {
                gtk_list_store_append(g_eventList, &ev->iterEv);
                gtk_list_store_set(g_eventList, &ev->iterEv,
                        EVL_EVCOL_PTR, ev, -1);
                ev->isOnEventList = TRUE;
            }
            switch( re->evType ) {
            case RET_BYDATE:
                if( re->evByDateMonths == 0 || re->evByDateDays == 0 ||
                        re->evByDateWeekdays == 0 )
                {
                    g_sprintf(fmtbuf, _("Off"));
                }else{
                    isMulti = FALSE;
                    formatted = fmtbuf;
                    *formatted = '\0';
                    if( re->evByDateWeekdays != 0x7F ) {
                        int wdayNum = g_bit_nth_lsf(re->evByDateWeekdays, -1);
                        const char *wday;
                        switch( wdayNum ) {
                        case 0: wday = _("Mon"); break;
                        case 1: wday = _("Tue"); break;
                        case 2: wday = _("Wed"); break;
                        case 3: wday = _("Thu"); break;
                        case 4: wday = _("Fri"); break;
                        case 5: wday = _("Sat"); break;
                        case 6: wday = _("Sun"); break;
                        default: wday = ""; break;
                        }
                        formatted += g_sprintf(formatted, "%s ", wday);
                        isMulti = isMulti ||
                            re->evByDateWeekdays & (re->evByDateWeekdays-1);
                    }
                    if( re->evByDateDays != 0x7FFFFFFF ) {
                        int day = g_bit_nth_lsf(re->evByDateDays, -1) + 1;
                        formatted += g_sprintf(formatted, "%02d ", day);
                        isMulti = isMulti ||
                            re->evByDateDays & (re->evByDateDays-1);
                    }
                    if( re->evByDateMonths != 0xFFF ) {
                        int month = g_bit_nth_lsf(re->evByDateMonths, -1) + 1;
                        g_date_clear(&date, 1);
                        g_date_set_dmy(&date, 1, month, 2014);
                        formatted += g_date_strftime(formatted,
                                sizeof(fmtbuf) + fmtbuf - formatted, "%b ",
                                &date);
                        isMulti = isMulti ||
                            re->evByDateMonths & (re->evByDateMonths-1);
                    }
                    if( re->evByDateYear ) {
                        formatted += g_sprintf(formatted, "%d ",
                                re->evByDateYear);
                    }
                    if( isMulti )
                        strcpy(formatted, "+");
                }
                break;
            default:    /* RET_PERIODIC */
                g_sprintf(fmtbuf, "%d", re->evPeriodicDays);
                break;
            }
            gtk_list_store_set(g_eventList, &ev->iterEv,
                    EVL_EVCOL_COLOR, type_to_color(re),
                    EVL_EVCOL_TYPE, event_type_short(re),
                    EVL_EVCOL_TEXT, re->text,
                    EVL_EVCOL_DATE, fmtbuf,
                    EVL_EVCOL_TEXT, re->text,
                    EVL_EVCOL_DELETED, re->evDeleted, -1);
        }
    }else{
        if( ev->isOnEventList ) {
            gtk_list_store_remove(g_eventList, &ev->iterEv);
            ev->isOnEventList = FALSE;
        }
        if( ev->isOnTodoList ) {
            gtk_list_store_remove(g_todoList, &ev->iterTodo);
            ev->isOnTodoList = FALSE;
        }
    }
    for(i = 0; i < ev->remind_count; ++i)
        updateRemindCols(ev->reminds[i]);
}

static gint remindersSortFunc(GtkTreeModel *model, GtkTreeIter *a,
                            GtkTreeIter *b, gpointer user_data)
{
    struct evl_remind *rmd_a, *rmd_b;
    const struct reminderer_remind *rr_a, *rr_b;
    int ret;

    gtk_tree_model_get(model, a, EVL_RMDCOL_PTR, &rmd_a, -1);
    gtk_tree_model_get(model, b, EVL_RMDCOL_PTR, &rmd_b, -1);
    rr_a = &rmd_a->rr;
    rr_b = &rmd_b->rr;
    if( (rr_a->day != 0) != (rr_b->day != 0) )
        ret = (rr_b->day != 0) - (rr_a->day != 0); // todos at end
    else if( rr_a->year != rr_b->year )
        ret = rr_a->year - rr_b->year;
    else if( rr_a->month != rr_b->month )
        ret = rr_a->month - rr_b->month;
    else
        ret = rr_a->day - rr_b->day;
    return ret;
}

static gint eventsSortFunc(GtkTreeModel *model, GtkTreeIter *a,
                            GtkTreeIter *b, gpointer user_data)
{
    struct evl_event *ev_a, *ev_b;
    const reminderer_event *re_a, *re_b;
    gint val_a, val_b;
    gboolean isOff_a, isOff_b;

    gtk_tree_model_get(model, a, EVL_EVCOL_PTR, &ev_a, -1);
    gtk_tree_model_get(model, b, EVL_EVCOL_PTR, &ev_b, -1);
    re_a = &ev_a->re;
    re_b = &ev_b->re;
    if( re_a->evType != re_b->evType )
        return re_a->evType - re_b->evType;
    switch( re_a->evType ) {
    case RET_BYDATE:
        isOff_a = re_a->evByDateMonths == 0 || re_a->evByDateDays == 0 ||
                re_a->evByDateWeekdays == 0;
        isOff_b = re_b->evByDateMonths == 0 || re_b->evByDateDays == 0 ||
                re_b->evByDateWeekdays == 0;
        if( isOff_a != isOff_b )
            return isOff_a ? 1 : -1;
        if( re_a->evByDateYear != re_b->evByDateYear ) {
            if( re_a->evByDateYear == 0 )
                return 1;
            if( re_b->evByDateYear == 0 )
                return -1;
            return re_a->evByDateYear - re_b->evByDateYear;
        }
        if( re_a->evByDateMonths != re_b->evByDateMonths ) {
            if( re_a->evByDateMonths == 0xFFF )
                return 1;
            if( re_b->evByDateMonths == 0xFFF )
                return -1;
            val_a = g_bit_nth_lsf(re_a->evByDateMonths, -1);
            val_b = g_bit_nth_lsf(re_b->evByDateMonths, -1);
            if( val_a != val_b )
                return val_a - val_b;
        }
        if( re_a->evByDateDays != re_b->evByDateDays ) {
            if( re_a->evByDateDays == 0x7FFFFFFF )
                return 1;
            if( re_b->evByDateDays == 0x7FFFFFFF )
                return -1;
            val_a = g_bit_nth_lsf(re_a->evByDateDays, -1);
            val_b = g_bit_nth_lsf(re_b->evByDateDays, -1);
            if( val_a != val_b )
                return val_a - val_b;
        }
        if( re_a->evByDateWeekdays != re_b->evByDateWeekdays ) {
            if( re_a->evByDateWeekdays == 0x7F )
                return 1;
            if( re_b->evByDateWeekdays == 0x7F )
                return -1;
            val_a = g_bit_nth_lsf(re_a->evByDateWeekdays, -1);
            val_b = g_bit_nth_lsf(re_b->evByDateWeekdays, -1);
            if( val_a != val_b )
                return val_a - val_b;
        }
        break;
    case RET_PERIODIC:
        return re_a->evPeriodicDays - re_b->evPeriodicDays;
    default:
        break;
    }
    return 0;
}

static gint todosSortFunc(GtkTreeModel *model, GtkTreeIter *a,
                            GtkTreeIter *b, gpointer user_data)
{
    struct evl_event *ev_a, *ev_b;
    const reminderer_event *re_a, *re_b;

    gtk_tree_model_get(model, a, EVL_TODOCOL_PTR, &ev_a, -1);
    gtk_tree_model_get(model, b, EVL_TODOCOL_PTR, &ev_b, -1);
    re_a = &ev_a->re;
    re_b = &ev_b->re;
    return re_b->keep - re_a->keep;
}

void evl_init(void)
{
    g_events = g_hash_table_new(NULL, NULL);
    g_eventList = gtk_list_store_new (EVL_EVCOL_COUNT,
                                   GDK_TYPE_RGBA,       /* color */
                                   G_TYPE_STRING,       /* type */
                                   G_TYPE_STRING,       /* date */
                                   G_TYPE_STRING,       /* text */
                                   G_TYPE_BOOLEAN,      /* deleted */
                                   G_TYPE_POINTER);     /* ptr */
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(g_eventList),
                                            eventsSortFunc, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(g_eventList),
            GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
    g_remindList = gtk_list_store_new (EVL_RMDCOL_COUNT,
                                   GDK_TYPE_RGBA,       /* color */
                                   G_TYPE_STRING,       /* days */
                                   G_TYPE_STRING,       /* date */
                                   G_TYPE_STRING,       /* text */
                                   G_TYPE_BOOLEAN,      /* deleted */
                                   G_TYPE_POINTER);     /* ptr */
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(g_remindList),
                                            remindersSortFunc, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(g_remindList),
            GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
    g_previewList = gtk_list_store_new (EVL_RMDCOL_COUNT,
                                   GDK_TYPE_RGBA,       /* color */
                                   G_TYPE_STRING,       /* days */
                                   G_TYPE_STRING,       /* date */
                                   G_TYPE_STRING,       /* text */
                                   G_TYPE_BOOLEAN,      /* deleted */
                                   G_TYPE_POINTER);     /* ptr */
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(g_previewList),
                                            remindersSortFunc, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(g_previewList),
            GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
    g_todoList = gtk_list_store_new (EVL_TODOCOL_COUNT,
                                  GDK_TYPE_RGBA,        /* color */
                                  G_TYPE_STRING,        /* mark */
                                  G_TYPE_STRING,        /* text */
                                  G_TYPE_BOOLEAN,       /* deleted */
                                  G_TYPE_POINTER);      /* ptr */
    gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(g_todoList),
                                            todosSortFunc, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(g_todoList),
            GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
}

/* Adds new remind at end of remind list for the event
 */
evl_remind_id evl_remind_new(evl_event_id eid, const reminderer_remind *rr)
{
    struct evl_remind *rmd = g_new(struct evl_remind, 1);

    rmd->rr = *rr;
    rmd->ev = eid;
    eid->reminds = g_renew(struct evl_remind*, eid->reminds,
            eid->remind_count+1);
    eid->reminds[eid->remind_count++] = rmd;
    rmd->isOnRmdList = FALSE;
    rmd->isOnPvwList = FALSE;
    updateRemindCols(rmd);
    return rmd;
}

gint evl_remind_count(evl_event_id eid)
{
    return eid->remind_count;
}

evl_remind_id evl_remind_get_at(evl_event_id eid, gint pos)
{
    return eid->reminds[pos];
}

evl_event_id evl_get_eid(evl_remind_id rid)
{
    return rid->ev;
}

const reminderer_remind *evl_get_rr(evl_remind_id rid)
{
    return &rid->rr;
}

void evl_set_rr(evl_remind_id rid, const reminderer_remind *rr)
{
    rid->rr = *rr;
    updateRemindCols(rid);
}

static void remindDestroy(struct evl_event *ev, int remindNo)
{
    int i;
    struct evl_remind *rmd = ev->reminds[remindNo];

    if( rmd->isOnRmdList )
        gtk_list_store_remove(g_remindList, &rmd->iterRmd);
    if( rmd->isOnPvwList )
        gtk_list_store_remove(g_previewList, &rmd->iterPvw);
    --ev->remind_count;
    for(i = remindNo; i < ev->remind_count; ++i)
        ev->reminds[i] = ev->reminds[i+1];
    g_free(rmd);
}

evl_event_id evl_event_new(const reminderer_event *re)
{
    struct evl_event *ev = g_new(struct evl_event, 1);
    reminderer_event_init(&ev->re);
    reminderer_event_set(&ev->re, re);
    ev->reminds = NULL;
    ev->remind_count = 0;
    ev->isOnEventList = FALSE;
    ev->isOnTodoList = FALSE;
    g_hash_table_insert(g_events, ev, ev);
	updateEventCols(ev);
    return ev;
}

const reminderer_event *evl_get_re(evl_event_id eid)
{
    return &eid->re;
}

void evl_set_re(evl_event_id eid, const reminderer_event *re)
{
    reminderer_event_set(&eid->re, re);
	updateEventCols(eid);
}

static void destroyEventNoRmHT(evl_event_id eid)
{
    while( eid->remind_count )
        remindDestroy(eid, 0);
    if( eid->isOnEventList )
        gtk_list_store_remove(g_eventList, &eid->iterEv);
    if( eid->isOnTodoList )
        gtk_list_store_remove(g_todoList, &eid->iterTodo);
    reminderer_event_destroy(&eid->re);
    g_free(eid);
}

void evl_event_destroy(evl_event_id eid)
{
    destroyEventNoRmHT(eid);
    g_hash_table_remove(g_events, eid);
}

evl_event_id evl_eidFromList(enum evl_iterlist listType, GtkTreeIter *iter)
{
	struct evl_event *ev;
	
    if( listType == EVL_EVENTS )
        gtk_tree_model_get(GTK_TREE_MODEL(g_eventList), iter,
                           EVL_EVCOL_PTR, &ev, -1);
    else
        gtk_tree_model_get(GTK_TREE_MODEL(g_todoList), iter,
                           EVL_TODOCOL_PTR, &ev, -1);
    return ev;
}

/* Derives reminds from the event; checks all days since dateDeriveFrom till
 * current day.
 */
void evl_derive_reminds(evl_event_id ev, const GDate *dateDeriveFrom)
{
	struct evl_remind *rmd;
	int i, j, assigned_cnt = 0;
	GDate dateChk, firstJan14;
    const GDate *curDate = evs_get_cur_date();
    reminderer_remind rr;

    if( ev->re.evType == RET_TODO ) {
        if( ev->re.keep ) {
            rr.year = 0;
            rr.month = 0;
            rr.day = 0;
            rr.rrDeleted = FALSE;
            if( ev->remind_count > 0 )
                evl_set_rr(ev->reminds[0], &rr);
            else
                evl_remind_new(ev, &rr);
            ++assigned_cnt;
        }
        while( ev->remind_count > assigned_cnt )
            remindDestroy(ev, assigned_cnt);
    }else{
        if( ev->re.keep ) {
            /* check past dates, too */
            dateChk = *dateDeriveFrom;
            i = g_date_days_between(curDate, &dateChk);
        }else{
            dateChk = *curDate;
            i = 0;
        }
        g_date_clear(&firstJan14, 1);
        g_date_set_dmy(&firstJan14, 1, 1, 2014);
        while(i <= ev->re.remind_days + 365) {
            gboolean isDateEv = FALSE;
            gint yearChk, monthChk, dayChk;
            gint from1stJan14;

            yearChk = g_date_year(&dateChk);
            monthChk = g_date_month(&dateChk);
            dayChk = g_date_day(&dateChk);
            switch( ev->re.evType ) {
            case RET_BYDATE:
                isDateEv = (ev->re.evByDateYear == 0 ||
                        ev->re.evByDateYear == yearChk) &&
                    ev->re.evByDateMonths & 1 << (monthChk-1) &&
                    ev->re.evByDateDays & 1 << (dayChk-1);
                if( isDateEv ) {
                    gint weekdayChk = g_date_get_weekday(&dateChk);
                    isDateEv = ev->re.evByDateWeekdays & 1 << (weekdayChk-1);
                }
                break;
            case RET_PERIODIC:
                from1stJan14 = g_date_days_between(&firstJan14, &dateChk);
                isDateEv = (from1stJan14 - ev->re.evPeriodicOffset) %
                    ev->re.evPeriodicDays == 0;
                break;
            default:
                break;
            }
            if( isDateEv ) {
                /* use matching remind if found, otherwise create new */
                for(j = assigned_cnt; j < ev->remind_count; ++j) {
                    rmd = ev->reminds[j];
                    if( rmd->rr.year == g_date_year(&dateChk) &&
                            rmd->rr.month == g_date_month(&dateChk) &&
                            rmd->rr.day == g_date_day(&dateChk) )
                        break;
                }
                if( j == ev->remind_count ) {
                    rr.year = g_date_year(&dateChk);
                    rr.month = g_date_month(&dateChk);
                    rr.day = g_date_day(&dateChk);
                    rr.rrDeleted = FALSE;
                    evl_remind_new(ev, &rr);
                }
                rmd = ev->reminds[j];
                ev->reminds[j] = ev->reminds[assigned_cnt];
                ev->reminds[assigned_cnt] = rmd;
                ++assigned_cnt;
                updateRemindCols(rmd);
            }
            g_date_add_days(&dateChk, 1);
            ++i;
        }
        while( assigned_cnt < ev->remind_count ) {
            rmd = ev->reminds[assigned_cnt];
            if( rmd->rr.day != 0 )
                g_date_set_dmy(&dateChk, rmd->rr.day, rmd->rr.month,
                        rmd->rr.year);
            if(ev->re.keep && rmd->rr.day != 0 &&
                        g_date_compare(&dateChk, curDate) < 0)
            {
                updateRemindCols(rmd);
                ++assigned_cnt;
            }else
                remindDestroy(ev, assigned_cnt);
        }
    }
}

GtkListStore *evl_events_get(void)
{
	return g_eventList;
}

GtkListStore *evl_reminds_get(void)
{
	return g_remindList;
}

GtkListStore *evl_preview_get(void)
{
	return g_previewList;
}

GtkListStore *evl_todos_get(void)
{
	return g_todoList;
}

struct foreachParam {
    void (*pf)(evl_event_id, gpointer);
    gpointer user_data;
};

static void foreachFun(gpointer key, gpointer val, gpointer user_data)
{
    struct evl_event *ev = (struct evl_event*)key;
    struct foreachParam *param = (struct foreachParam*)user_data;

    param->pf(ev, param->user_data);
}

void evl_events_foreach( void(*pf)(evl_event_id, gpointer), gpointer user_data)
{
    struct foreachParam param;

    param.pf = pf;
    param.user_data = user_data;
    g_hash_table_foreach(g_events, foreachFun, &param);
}

struct foreachRemoveParam {
    gboolean (*pf)(evl_event_id, gpointer);
    gpointer user_data;
};

static gboolean foreachRemoveFun(gpointer key, gpointer val, gpointer user_data)
{
    gboolean res;

    struct evl_event *ev = (struct evl_event*)key;
    struct foreachRemoveParam *param = (struct foreachRemoveParam*)user_data;
    res = param->pf == NULL ? TRUE : param->pf(ev, param->user_data);
    if( res ) {
        destroyEventNoRmHT(ev);
    }
    return res;
}

void evl_events_foreach_remove( gboolean(*pf)(evl_event_id, gpointer),
        gpointer user_data)
{
    struct foreachRemoveParam param;

    param.pf = pf;
    param.user_data = user_data;
    g_hash_table_foreach_remove(g_events, foreachRemoveFun, &param);
}

evl_event_id evl_eidFromPath(enum evl_iterlist listType, GtkTreePath *path)
{
    GtkTreeIter iter;

    GtkListStore *store = listType == EVL_EVENTS ? g_eventList : g_todoList;
    if( gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, path) )
        return evl_eidFromList(listType, &iter);
    return NULL;
}

evl_event_id evl_eidFromRemindPath(GtkTreePath *rmdListPath)
{
	GtkTreeIter remindIter;
	struct evl_remind *rmd;
	
    gboolean ret = gtk_tree_model_get_iter(GTK_TREE_MODEL(g_remindList),
                                           &remindIter, rmdListPath);
	if( ret ) {
        gtk_tree_model_get(GTK_TREE_MODEL(g_remindList), &remindIter,
                EVL_RMDCOL_PTR, &rmd, -1);
        return rmd->ev;
	}
    return NULL;
}

evl_event_id evl_eidFromPreviewPath(GtkTreePath *pvwListPath)
{
	GtkTreeIter previewIter;
	struct evl_remind *rmd;
	
    gboolean ret = gtk_tree_model_get_iter(GTK_TREE_MODEL(g_previewList),
                                           &previewIter, pvwListPath);
	if( ret ) {
        gtk_tree_model_get(GTK_TREE_MODEL(g_previewList), &previewIter,
                EVL_RMDCOL_PTR, &rmd, -1);
        return rmd->ev;
	}
    return NULL;
}

evl_event_id evl_eidFromRemindList(GtkTreeIter *remindIter)
{
	struct evl_remind *rmd;
	
 	gtk_tree_model_get(GTK_TREE_MODEL(g_remindList), remindIter,
            EVL_RMDCOL_PTR, &rmd, -1);
    return rmd->ev;
}

evl_event_id evl_eidFromPreviewList(GtkTreeIter *previewIter)
{
	struct evl_remind *rmd;
	
 	gtk_tree_model_get(GTK_TREE_MODEL(g_previewList), previewIter,
            EVL_RMDCOL_PTR, &rmd, -1);
    return rmd->ev;
}

evl_remind_id evl_ridFromRemindList(GtkTreeIter *iter)
{
	struct evl_remind *rmd;
	
	gtk_tree_model_get(GTK_TREE_MODEL(g_remindList), iter,
	                   EVL_RMDCOL_PTR, &rmd, -1);
    return rmd;
}

evl_remind_id evl_ridFromPreviewList(GtkTreeIter *iter)
{
	struct evl_remind *rmd;
	
	gtk_tree_model_get(GTK_TREE_MODEL(g_previewList), iter,
	                   EVL_RMDCOL_PTR, &rmd, -1);
    return rmd;
}

gboolean evl_showdeleted_get(void)
{
    return g_showDeleted;
}

static void showDelForeachFun(gpointer key, gpointer val, gpointer user_data)
{
    struct evl_event *ev = (struct evl_event*)key;
    struct evl_remind *rmd;
    int i;

    if( ev->re.evDeleted )
        updateEventCols(ev);
    for(i = 0; i < ev->remind_count; ++i) {
        rmd = ev->reminds[i];
        if( rmd->rr.rrDeleted )
            updateRemindCols(rmd);
    }
}

void evl_showdeleted_set(gboolean show)
{
    if( g_showDeleted != show ) {
        g_showDeleted = show;
        g_hash_table_foreach(g_events, showDelForeachFun, NULL);
    }
}

static void changedPrefsForeachFun(gpointer key, gpointer val,
        gpointer user_data)
{
    struct evl_event *ev = (struct evl_event*)key;

    updateEventCols(ev);
}

void evl_changed_disp_preferences(void)
{
    g_hash_table_foreach(g_events, changedPrefsForeachFun, NULL);
}

void evl_select_in_treeview(evl_event_id ev, GtkTreeView *treeView)
{
    GtkTreeModel *model;
    GtkListStore *store;
    GtkTreePath *path = NULL;

    model = gtk_tree_view_get_model(treeView);
    store = GTK_LIST_STORE(model);
    if( store == g_eventList ) {
        if( ev->isOnEventList )
            path = gtk_tree_model_get_path(model, &ev->iterEv);
    }else if( store == g_todoList ) {
        if( ev->isOnTodoList )
            path = gtk_tree_model_get_path(model, &ev->iterTodo);
    }else{
        int i;
        for(i = 0; path == NULL && i < ev->remind_count; ++i) {
            struct evl_remind *rmd = ev->reminds[i];
            if( store == g_previewList ) {
                if( rmd->isOnPvwList )
                    path = gtk_tree_model_get_path(model, &rmd->iterPvw);
            }else{
                if( rmd->isOnRmdList )
                    path = gtk_tree_model_get_path(model, &rmd->iterRmd);
            }
        }
    }
    if( path != NULL ) {
        gtk_tree_view_set_cursor(treeView, path, NULL, FALSE);
        gtk_tree_path_free(path);
    }
}

gboolean evl_remind_is_empty(void)
{
    GtkTreeIter iter;

    return !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(g_remindList), &iter);
}

