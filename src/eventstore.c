#include "reminderer.h"
#include <glib/gstdio.h>
#include "eventstore.h"
#include "eventlists.h"
#include "preferences.h"
#include <gio/gio.h>
#include <string.h>
#include <errno.h>


static GDate g_curDate;
static int gTimeOffset;

static void saveEventFun(evl_event_id eid, gpointer user_data)
{
    FILE *fp = (FILE*)user_data;
    gchar *text;
    int i, savedCount;
    const reminderer_event *re;
    const reminderer_remind *rr;
    const char *nodeName = NULL;
    GDate rmdDate;

    re = evl_get_re(eid);
    switch( re->evType ) {
    case RET_BYDATE:
        fprintf(fp, "    <eventByDate");
        if( re->evByDateYear != 0 )
            fprintf(fp, " year=\"%d\"", re->evByDateYear);
        fprintf(fp, " weekdays=\"%X\"", re->evByDateWeekdays);
        fprintf(fp, " months=\"%X\"", re->evByDateMonths);
        fprintf(fp, " days=\"%X\"", re->evByDateDays);
        nodeName = "eventByDate";
        break;
    case RET_PERIODIC:
        fprintf(fp, "    <eventPeriodic");
        fprintf(fp, " period=\"%d\"", re->evPeriodicDays);
        fprintf(fp, " offset=\"%d\"", re->evPeriodicOffset);
        nodeName = "eventPeriodic";
        break;
    case RET_TODO:
        fprintf(fp, "    <todo");
        break;
    }
    if( re->evDeleted )
        fprintf(fp, " deleted=\"1\"");
    text = g_markup_escape_text(re->text, -1);
    fprintf(fp, " text=\"%s\"", text);
    g_free(text);
    if( re->evType == RET_TODO ) {
        if( re->keep )
            fprintf(fp, " remind=\"1\"");
        fprintf(fp, "/>\n");
    }else{
        if( re->remind_days != 7 )
            fprintf(fp, " remind=\"%d\"", re->remind_days);
        if( re->keep )
            fprintf(fp, " keep=\"1\"");
        savedCount = 0;
        for(i = 0; i < evl_remind_count(eid); ++i) {
            rr = evl_get_rr(evl_remind_get_at(eid, i));
            g_date_clear(&rmdDate, 1);
            g_date_set_dmy(&rmdDate, rr->day, rr->month, rr->year);
            if( rr->rrDeleted || g_date_compare(&rmdDate, &g_curDate) < 0) {
                if( ++savedCount == 1 )
                    fprintf(fp, ">\n");
                fprintf(fp, "        <remind");
                fprintf(fp, " day=\"%d\"", rr->day);
                fprintf(fp, " month=\"%d\"", rr->month);
                fprintf(fp, " year=\"%d\"", rr->year);
                if( rr->rrDeleted )
                    fprintf(fp, " deleted=\"1\"");
                fprintf(fp, "/>\n");
            }
        }
        if( savedCount == 0 )
            fprintf(fp, "/>\n");
        else
            fprintf(fp, "    </%s>\n", nodeName);
    }
}

static void saveAll(void)
{
    static gboolean didSave = FALSE;
    const gchar *eventsFile;
    gchar *fname_tmp = NULL, *fname_bak = NULL;
    FILE *fp = NULL;

    if((eventsFile = prefs_getEventsFileName()) == NULL)
        return;
    fname_tmp = g_strconcat(eventsFile, ".tmp", NULL);
    if( (fp = fopen(fname_tmp, "w")) == NULL ) {
        rmdr_showMessage(_("Unable to create file %s\n%s"), fname_tmp,
                strerror(errno));
        goto save_error;
    }
    fprintf(fp, "<reminders derived=\"%d%02d%02d\">\n",
            g_date_year(&g_curDate),
            g_date_month(&g_curDate),
            g_date_day(&g_curDate));
    evl_events_foreach(saveEventFun, fp);
    if( fprintf(fp, "</reminders>\n") < 0 ) {
        rmdr_showMessage(_("Unable to write to %s"), fname_tmp);
        goto save_error;
    }
    if( fclose(fp) != 0 ) {
        rmdr_showMessage(_("Unable to write to %s"), fname_tmp);
        fp = NULL;
        goto save_error;
    }
    fp = NULL;
    if( ! didSave ) {
        if( g_file_test(eventsFile, G_FILE_TEST_EXISTS) ) {
            fname_bak = g_strconcat(eventsFile, "~", NULL);
            if( g_rename(eventsFile, fname_bak) == -1 ) {
                rmdr_showMessage(_("Unable to rename %s to %s\n%s"),
                        eventsFile, fname_bak, strerror(errno));
                goto save_error;
            }
        }
        didSave = TRUE;
    }
    if( g_rename(fname_tmp, eventsFile) == -1 ) {
        rmdr_showMessage(_("Unable to rename %s to %s"),
                fname_tmp, eventsFile);
        goto save_error;
    }
save_error:
    if( fp != NULL )
        fclose(fp);
    g_free(fname_tmp);
    g_free(fname_bak);
}

static void checkDerivesForeachFun(evl_event_id eid, gpointer user_data)
{
    const reminderer_event *re = evl_get_re(eid);
    const GDate *dateDeriveFrom = user_data;
    if( re->evType != RET_TODO )
        evl_derive_reminds(eid, dateDeriveFrom);
}

static void checkEventDerives(const GDate *deriveStartDate)
{
    GDate deriveDate = *deriveStartDate;
    evl_events_foreach(checkDerivesForeachFun, &deriveDate);
}

static gboolean timeout_fun(gpointer data)
{
	GDate curDate;
    GDateTime *curDateTime;
    gint toSleep;
	GTimeVal timeVal;
	
	g_get_current_time(&timeVal);
    timeVal.tv_sec += gTimeOffset;
	g_date_clear(&curDate, 1);
	g_date_set_time_val(&curDate, &timeVal);
	if( g_date_compare(&curDate, &g_curDate) != 0 ) {
		g_curDate = curDate;
		checkEventDerives(&curDate);
	}
    curDateTime = g_date_time_new_from_timeval_local(&timeVal);
    toSleep = 86400 - 3600 * g_date_time_get_hour(curDateTime) +
            - 60 * g_date_time_get_minute(curDateTime) +
            - g_date_time_get_second(curDateTime);
    g_date_time_unref(curDateTime);
	g_timeout_add(toSleep * 1000, timeout_fun, NULL);
	return FALSE;
}

static const gchar *attrVal(const gchar **attribute_names,
                            const gchar *attr_name,
                            const gchar **attribute_values)
{
	int i;
	
	for(i = 0; attribute_names[i]; ++i) {
		if( !g_strcmp0(attribute_names[i], attr_name) )
			return attribute_values[i];
	}
	return NULL;
}

static gint attrValNum(const gchar **attribute_names,
                       const gchar *attr_name,
                       const gchar **attribute_values, int defaultVal)
{
	const char *attr_val = attrVal(attribute_names, attr_name, attribute_values);
	if( attr_val == NULL )
		return defaultVal;
	return g_ascii_strtoll(attr_val, NULL, 10);
}

static gint attrValNumHex(const gchar **attribute_names,
                       const gchar *attr_name,
                       const gchar **attribute_values, int defaultVal)
{
	const char *attr_val = attrVal(attribute_names, attr_name, attribute_values);
	if( attr_val == NULL )
		return defaultVal;
	return g_ascii_strtoll(attr_val, NULL, 16);
}

struct parse_param {
    GDate deriveDate;
	int nestLvl;
    evl_event_id curEv;
    int curEvMonth, curEvDay;
    GString *parseErrors;
};

static void startElem(GMarkupParseContext *context,
                      const gchar         *element_name,
                      const gchar        **attribute_names,
                      const gchar        **attribute_values,
                      gpointer             user_data,
                      GError             **error)
{
	struct parse_param *pp = (struct parse_param*)user_data;
	evl_event_id eid;
	const char *txt;
	guint date;
    gboolean add;
    GDate rmdDate;
    reminderer_event re;
    const reminderer_event *pre;
    reminderer_remind rr;
    gint linenum;

	++pp->nestLvl;
    switch( pp->nestLvl ) {
    case 1:
        if( !g_strcmp0(element_name, "reminders") ) {
            date = attrValNum(attribute_names, "derived", attribute_values, 0);
            if( date != 0 )
                g_date_set_dmy(&pp->deriveDate, date % 100, date / 100 % 100,
                        date / 10000);
        }else{
            *error = g_error_new(G_MARKUP_ERROR,
                    G_MARKUP_ERROR_UNKNOWN_ELEMENT, _("Expected <reminders> "
                    "root element, is <%s>"), element_name);
        }
        break;
    case 2:
        eid = NULL;
        if( !g_strcmp0(element_name, "event") ) {
            /* backward compatibility */
            reminderer_event_init(&re);
            re.evType = RET_BYDATE;
            date = attrValNum(attribute_names, "date", attribute_values, 1);
            re.evByDateYear = date / 10000;
            re.evByDateWeekdays = 0x7F;
            pp->curEvMonth = date % 10000 / 100;
            pp->curEvDay = date % 100;
            re.evByDateMonths = pp->curEvMonth ?
                1 << (pp->curEvMonth - 1) : 0xFFF;
            re.evByDateDays = 1 << (pp->curEvDay - 1);
            txt = attrVal(attribute_names, "text", attribute_values);
            reminderer_event_set_text (&re, txt);
            re.remind_days = attrValNum(attribute_names, "days",
                                            attribute_values, 7);
            re.keep = attrValNum(attribute_names, "keep", attribute_values, 0);
            re.evDeleted = attrValNum(attribute_names, "deleted",
                    attribute_values, 0) != 0;
            eid = evl_event_new(&re);
            reminderer_event_destroy(&re);
        }else if( !g_strcmp0(element_name, "eventByDate") ) {
            reminderer_event_init(&re);
            re.evType = RET_BYDATE;
            re.evByDateYear = attrValNum(attribute_names, "year",
                    attribute_values, 0);
            re.evByDateWeekdays = attrValNumHex(attribute_names, "weekdays",
                    attribute_values, 0x7F);
            re.evByDateMonths = attrValNumHex(attribute_names, "months",
                    attribute_values, 0xFFF);
            re.evByDateDays = attrValNumHex(attribute_names, "days",
                    attribute_values, 0x7FFFFFFF);
            txt = attrVal(attribute_names, "text", attribute_values);
            reminderer_event_set_text (&re, txt);
            re.remind_days = attrValNum(attribute_names, "remind",
                                            attribute_values, 7);
            re.keep = attrValNum(attribute_names, "keep", attribute_values, 0);
            re.evDeleted = attrValNum(attribute_names, "deleted",
                    attribute_values, 0) != 0;
            eid = evl_event_new(&re);
            reminderer_event_destroy(&re);
            pp->curEvMonth = pp->curEvDay = 0;
        }else if( !g_strcmp0(element_name, "eventPeriodic") ) {
            reminderer_event_init(&re);
            re.evType = RET_PERIODIC;
            re.evPeriodicDays = attrValNum(attribute_names, "period",
                    attribute_values, 2);
            re.evPeriodicOffset = attrValNum(attribute_names, "offset",
                    attribute_values, 2);
            txt = attrVal(attribute_names, "text", attribute_values);
            reminderer_event_set_text (&re, txt);
            re.remind_days = attrValNum(attribute_names, "remind",
                                            attribute_values, 7);
            re.keep = attrValNum(attribute_names, "keep", attribute_values, 0);
            re.evDeleted = attrValNum(attribute_names, "deleted",
                    attribute_values, 0) != 0;
            eid = evl_event_new(&re);
            reminderer_event_destroy(&re);
            pp->curEvMonth = pp->curEvDay = 0;
        }else if( !g_strcmp0(element_name, "todo") ) {
            reminderer_event_init(&re);
            re.evType = RET_TODO;
            txt = attrVal(attribute_names, "text", attribute_values);
            reminderer_event_set_text (&re, txt);
            re.remind_days = 7;
            re.keep = attrValNum(attribute_names, "remind",
                    attribute_values, 0);
            re.evDeleted = attrValNum(attribute_names, "deleted",
                    attribute_values, 0) != 0;
            eid = evl_event_new(&re);
            reminderer_event_destroy(&re);
            evl_derive_reminds(eid, NULL);
        }else{
            g_markup_parse_context_get_position(context, &linenum, NULL);
            g_string_append_printf(pp->parseErrors,
                    _("\nunrecognized element \"%s\" at line %d"),
                    element_name, linenum);
        }
        pp->curEv = eid;
        break;
    case 3:
        if( !g_strcmp0(element_name, "remind") ) {
            eid = pp->curEv;
            if( eid != NULL ) {
                pre = evl_get_re(eid);
                /* year/month/day may not appear on old-type <event> elements */
                rr.year = attrValNum(attribute_names, "year",
                        attribute_values, pre->evByDateYear);
                rr.month = attrValNum(attribute_names, "month",
                        attribute_values, pp->curEvMonth);
                rr.day = attrValNum(attribute_names, "day",
                        attribute_values, pp->curEvDay);
                rr.rrDeleted = attrValNum(attribute_names, "deleted",
                        attribute_values, 0);
                add = TRUE;
                if( rr.rrDeleted ) {
                    if( rr.day == 0 )
                        add = FALSE;
                    else{
                        g_date_clear(&rmdDate, 1);
                        g_date_set_dmy(&rmdDate, rr.day, rr.month, rr.year);
                        add = g_date_compare(&rmdDate,
                               pre->keep ? &pp->deriveDate : &g_curDate) >= 0;
                    }
                }
                if( add )
                    evl_remind_new(eid, &rr);
            }
        }else{
            g_markup_parse_context_get_position(context, &linenum, NULL);
            g_string_append_printf(pp->parseErrors,
                    _("\nunrecognized element \"%s\" at line %d"),
                    element_name, linenum);
        }
        break;
    default:
        break;
	}
}

static void endElem(GMarkupParseContext *context,
             const gchar         *element_name,
             gpointer             user_data,
             GError             **error)
{
	struct parse_param *pp = (struct parse_param*)user_data;
	if( --pp->nestLvl == 1 )
        pp->curEv = NULL;
}

void evs_load_file(const gchar *fname, gchar **parseErrors)
{
	GMarkupParseContext *context;
	GMarkupParser parser;
	gchar *docStr;
	struct parse_param pp;
    GError *err = NULL;

    evl_events_foreach_remove(NULL, NULL);
    *parseErrors = NULL;
    if( g_file_test(fname, G_FILE_TEST_EXISTS) ) {
        /* parse events file */
        parser.start_element = startElem;
        parser.end_element = endElem;
        parser.text = NULL;
        parser.passthrough = NULL;
        parser.error = NULL;
        pp.nestLvl = 0;
        pp.deriveDate = g_curDate;
        pp.parseErrors = g_string_new(NULL);
        if( g_file_get_contents(fname, &docStr, NULL, &err) ) {
            context = g_markup_parse_context_new(&parser, 0, &pp, NULL);
            if(g_markup_parse_context_parse(context, docStr, strlen(docStr),
                        &err) &&
                    g_markup_parse_context_end_parse(context, &err) )
            {
                if( pp.parseErrors->len > 0 ) {
                    /* note: order is reversed due to "prepend" */
                    g_string_prepend(pp.parseErrors, "\n");
                    g_string_prepend(pp.parseErrors, fname);
                    g_string_prepend(pp.parseErrors,
                            _("Errors encountered in events file\n"));
                }
            }else{
                g_string_printf(pp.parseErrors,
                        _("Unable to parse events file\n%s\n%s"),
                        fname, err->message);
                g_error_free(err);
            }
            g_markup_parse_context_free(context);
            g_free(docStr);
            checkEventDerives(&pp.deriveDate);
        }else{
            g_string_assign(pp.parseErrors, err->message);
            g_error_free(err);
        }
        *parseErrors = g_string_free(pp.parseErrors,
                    pp.parseErrors->len == 0);
    }
}

evl_event_id evs_event_add(const reminderer_event *re)
{
	evl_event_id eid;

    eid = evl_event_new(re);
    evl_derive_reminds(eid, &g_curDate);
	saveAll();
    return eid;
}

void evs_event_set(evl_event_id eid, const reminderer_event *reSet)
{
    evl_set_re(eid, reSet);
    evl_derive_reminds(eid, &g_curDate);
	saveAll();
}

static void setEventDeleteState(evl_event_id eid, gboolean deleted)
{
    const reminderer_event *pre = evl_get_re(eid);
    reminderer_event re;

    if( pre->evDeleted != deleted ) {
        reminderer_event_init(&re);
        reminderer_event_set(&re, pre);
        re.evDeleted = deleted;
        evl_set_re(eid, &re);
        reminderer_event_destroy(&re);
        saveAll();
    }
}

void evs_event_del(evl_event_id eid)
{
    setEventDeleteState(eid, TRUE);
}

void evs_event_undel(evl_event_id eid)
{
    setEventDeleteState(eid, FALSE);
}

void evs_remind_del(evl_remind_id rid)
{
    const reminderer_remind *prr = evl_get_rr(rid);
    reminderer_remind rr;

    if( prr->day == 0 ) { // a todo
        setEventDeleteState(evl_get_eid(rid), TRUE);
    }else if( ! prr->rrDeleted ) {
        rr = *prr;
        rr.rrDeleted = TRUE;
        evl_set_rr(rid, &rr);
        saveAll();
    }
}

void evs_remind_undel(evl_remind_id rid)
{
    const reminderer_remind *prr = evl_get_rr(rid);
    reminderer_remind rr;

    if( prr->day == 0 ) { // a todo
        setEventDeleteState(evl_get_eid(rid), FALSE);
    }else{
        if( prr->rrDeleted ) {
            rr = *prr;
            rr.rrDeleted = FALSE;
            evl_set_rr(rid, &rr);
            saveAll();
        }
    }
}

static gboolean purgeDelForeachFun(evl_event_id eid, gpointer user_data)
{
    gboolean *anyDeleted = (gboolean*)user_data;
    const reminderer_event *re = evl_get_re(eid);
    gboolean isDeleted = re->evDeleted;
    GDate date;

    if( ! isDeleted && re->evType == RET_BYDATE && re->evByDateYear != 0 &&
        re->evByDateMonths != 0 && re->evByDateDays != 0 &&
        re->evByDateWeekdays != 0 )
    {
        /* delete when from past and their remind is deleted */
        gint month = g_bit_nth_msf(re->evByDateMonths, 12)+1;
        gint day = g_bit_nth_msf(re->evByDateDays, 31)+1;
        gint lastDayInMonth;

        lastDayInMonth = g_date_days_in_month(month, re->evByDateYear);
        if( day > lastDayInMonth )
            day = lastDayInMonth;
        g_date_set_dmy(&date, day, month, re->evByDateYear);
        isDeleted = g_date_compare(&date, &g_curDate) < 0 &&
            evl_remind_count(eid) == 0;

    }
    if( isDeleted ) {
        *anyDeleted = TRUE;
    }
    return isDeleted;
}

void evs_purge_deleted_events(void)
{
    gboolean anyDeleted = FALSE;

    evl_events_foreach_remove(purgeDelForeachFun, &anyDeleted);
    if( anyDeleted )
        saveAll();
}

const GDate *evs_get_cur_date(void)
{
    return &g_curDate;
}

void evs_init(int timeOffset)
{
	GTimeVal timeVal;

    evl_init();
    gTimeOffset = timeOffset;
	/* initialize current date */
	g_get_current_time(&timeVal);
    timeVal.tv_sec += timeOffset;
	g_date_clear(&g_curDate, 1);
	g_date_set_time_val(&g_curDate, &timeVal);
    timeout_fun(NULL);
}

