#include "reminderer.h"
#include "eventstore.h"
#include "event_edit.h"
#include <glib/gprintf.h>
#include <stdlib.h>


/* glade XML containing the dialog windows */
static GtkBuilder *g_xml;

static void fillComboEvByDateYear(gint curYear, gint yearSetTo)
{
    GtkComboBoxText *combo;
    char entry_text[40];
    int i;

    combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(g_xml,
                "comboEvByDateYear"));
    if( yearSetTo != 0 && yearSetTo < curYear ) {
        g_sprintf(entry_text, "%d", yearSetTo);
        gtk_combo_box_text_append(combo, entry_text, entry_text);
    }
    for(i = 0; i < 16; ++i) {
        g_sprintf(entry_text, "%d", curYear+i);
        gtk_combo_box_text_append(combo, entry_text, entry_text);
    }
    g_sprintf(entry_text, "%d", yearSetTo);
    if( yearSetTo >= curYear + 16 ) {
        gtk_combo_box_text_append(combo, entry_text, entry_text);
    }
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo), entry_text);
}

static void setViewEvByDate(GtkIconView *view, gint activeIds)
{
    GtkTreeModel *store;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean found;
    gint i;

    store = gtk_icon_view_get_model(view);
    for(i = 1, found = gtk_tree_model_get_iter_first(store, &iter);
        found; i <<= 1, found = gtk_tree_model_iter_next(store, &iter))
    {
        if( activeIds & i ) {
            path = gtk_tree_model_get_path(store, &iter);
            gtk_icon_view_select_path(view, path);
            gtk_tree_path_free(path);
        }
    }
}

static gint getViewEvByDate(GtkIconView *view)
{
    GtkTreeModel *store;
    GtkTreeIter iter;
    GtkTreePath *path;
    gboolean found;
    gint i, activeIds = 0;

    store = gtk_icon_view_get_model(view);
    for(i = 1, found = gtk_tree_model_get_iter_first(store, &iter);
        found; i <<= 1, found = gtk_tree_model_iter_next(store, &iter))
    {
        path = gtk_tree_model_get_path(store, &iter);
        if( gtk_icon_view_path_is_selected(view, path) ) {
            activeIds |= i;
        }
        gtk_tree_path_free(path);
    }
    return activeIds;
}

static void adjustSensitivity(enum RemindererEventType et)
{
    GObject *comboEvByDateYear;
	GObject *listEvByDateWeekday, *listEvByDateMonth, *listEvByDateDay;
    GObject *spinEvPeriodicDays, *spinEvPeriodicNear;
	GObject *daysBefore, *chkKeep, *chkRemind;
	
	comboEvByDateYear = gtk_builder_get_object(g_xml, "comboEvByDateYear");
	listEvByDateWeekday = gtk_builder_get_object(g_xml,
            "listEvByDateWeekday");
	listEvByDateMonth = gtk_builder_get_object(g_xml, "listEvByDateMonth");
	listEvByDateDay = gtk_builder_get_object(g_xml, "listEvByDateDay");

	spinEvPeriodicDays = gtk_builder_get_object(g_xml, "spinEvPeriodicDays");
	spinEvPeriodicNear = gtk_builder_get_object(g_xml, "spinEvPeriodicNear");

	daysBefore = gtk_builder_get_object(g_xml, "daysBefore");
	chkKeep = gtk_builder_get_object(g_xml, "chkKeep");
	chkRemind = gtk_builder_get_object(g_xml, "chkRemind");

    g_object_set(comboEvByDateYear, "sensitive", et == RET_BYDATE, NULL);
    g_object_set(listEvByDateWeekday, "sensitive", et == RET_BYDATE, NULL);
    g_object_set(listEvByDateMonth, "sensitive", et == RET_BYDATE, NULL);
    g_object_set(listEvByDateDay, "sensitive", et == RET_BYDATE, NULL);

    g_object_set(spinEvPeriodicDays, "sensitive", et == RET_PERIODIC, NULL);
    g_object_set(spinEvPeriodicNear, "sensitive", et == RET_PERIODIC, NULL);

    g_object_set(daysBefore, "sensitive", et != RET_TODO, NULL);
    g_object_set(chkKeep, "sensitive", et != RET_TODO, NULL);
    g_object_set(chkRemind, "sensitive", et == RET_TODO, NULL);
}

gboolean run_EventEditDialog(GtkWidget *owner, struct reminderer_event *evbuf)
{
    GObject *window;
	GObject *radioEvByDate, *radioEvPeriodic, *radioTodo;
    GObject *comboEvByDateYear;
    GtkIconView *listEvByDateDay, *listEvByDateMonth, *listEvByDateWeekday;
    GtkSpinButton *spinEvPeriodicDays, *spinEvPeriodicNear;
    GObject *ctlText, *chkKeep;
	GObject *daysBefore, *chkRemind;
	gint status;
    GDate date;
    const GDate *curDate;
    gint curYear, curMonth, curDay, near, adjnear;
    GdkRGBA insensitive_color;

	g_xml = gtk_builder_new();
    gtk_builder_add_from_file(g_xml, GLADE_FILE("event_edit.ui"), NULL);
	radioEvByDate = gtk_builder_get_object(g_xml, "radioEvByDate");
	radioEvPeriodic = gtk_builder_get_object(g_xml, "radioEvPeriodic");
	radioTodo = gtk_builder_get_object(g_xml, "radioTodo");

	comboEvByDateYear = gtk_builder_get_object(g_xml, "comboEvByDateYear");
	listEvByDateDay = GTK_ICON_VIEW(gtk_builder_get_object(g_xml,
                "listEvByDateDay"));
	listEvByDateMonth = GTK_ICON_VIEW(gtk_builder_get_object(g_xml,
                "listEvByDateMonth"));
	listEvByDateWeekday = GTK_ICON_VIEW(gtk_builder_get_object(g_xml,
                "listEvByDateWeekday"));
	spinEvPeriodicDays = GTK_SPIN_BUTTON(gtk_builder_get_object(g_xml,
                "spinEvPeriodicDays"));
	spinEvPeriodicNear = GTK_SPIN_BUTTON(gtk_builder_get_object(g_xml,
                "spinEvPeriodicNear"));

	daysBefore = gtk_builder_get_object(g_xml, "daysBefore");
	chkKeep = gtk_builder_get_object(g_xml, "chkKeep");
	chkRemind = gtk_builder_get_object(g_xml, "chkRemind");
	ctlText = gtk_builder_get_object(g_xml, "text");
    curDate = evs_get_cur_date();
    curYear = g_date_get_year(curDate);
    curMonth = g_date_get_month(curDate);
    curDay = g_date_get_day(curDate);
    g_date_clear(&date, 1);
    g_date_set_dmy(&date, 1, 1, 2014);
    adjnear = g_date_days_between(&date, curDate);

    /* init RET_BYDATE elements */
    gdk_rgba_parse(&insensitive_color, "gray");
    gtk_widget_override_color(GTK_WIDGET(listEvByDateDay),
            GTK_STATE_FLAG_INSENSITIVE, &insensitive_color);
    gtk_widget_override_color(GTK_WIDGET(listEvByDateMonth),
            GTK_STATE_FLAG_INSENSITIVE, &insensitive_color);
    gtk_widget_override_color(GTK_WIDGET(listEvByDateWeekday),
            GTK_STATE_FLAG_INSENSITIVE, &insensitive_color);
    fillComboEvByDateYear(curYear,
            evbuf->evType == RET_BYDATE ? evbuf->evByDateYear : 0);
    setViewEvByDate(listEvByDateWeekday, evbuf->evType == RET_BYDATE ?
            evbuf->evByDateWeekdays : 0x7F);
    setViewEvByDate(listEvByDateDay, evbuf->evType == RET_BYDATE ?
            evbuf->evByDateDays : 1<<(curDay-1));
    setViewEvByDate(listEvByDateMonth, evbuf->evType == RET_BYDATE ?
            evbuf->evByDateMonths : 1<<(curMonth-1));

    switch( evbuf->evType ) {
    case RET_BYDATE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioEvByDate), TRUE);
        break;
    case RET_PERIODIC:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioEvPeriodic), TRUE);
        gtk_spin_button_set_value(spinEvPeriodicDays, evbuf->evPeriodicDays);
        gtk_adjustment_set_upper(
                gtk_spin_button_get_adjustment(spinEvPeriodicNear),
                evbuf->evPeriodicDays-1);
        near = (evbuf->evPeriodicOffset - adjnear) % evbuf->evPeriodicDays;
        if( near < 0 )
            near += evbuf->evPeriodicDays;
        gtk_spin_button_set_value(spinEvPeriodicNear, near);
        break;
    case RET_TODO:
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radioTodo), TRUE);
        break;
    }
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(daysBefore), evbuf->remind_days);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(chkKeep), evbuf->keep);
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(chkRemind), evbuf->keep);
	gtk_entry_set_text(GTK_ENTRY(ctlText), evbuf->text);
	adjustSensitivity(evbuf->evType);
	gtk_builder_connect_signals(g_xml, NULL);
	window = gtk_builder_get_object(g_xml, "eventEditDialog");
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(owner));
	status = gtk_dialog_run(GTK_DIALOG(window));
	if( status == 0 ) {
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radioEvByDate))) {
            evbuf->evType = RET_BYDATE;
            evbuf->evByDateYear = strtol(gtk_combo_box_get_active_id(
                    GTK_COMBO_BOX(comboEvByDateYear)), NULL, 10);
            evbuf->evByDateWeekdays = getViewEvByDate(listEvByDateWeekday);
            evbuf->evByDateMonths = getViewEvByDate(listEvByDateMonth);
            evbuf->evByDateDays = getViewEvByDate(listEvByDateDay);
        }else if(gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(radioEvPeriodic)))
        {
            evbuf->evType = RET_PERIODIC;
            evbuf->evPeriodicDays = gtk_spin_button_get_value(
                    spinEvPeriodicDays);
            evbuf->evPeriodicOffset =
                (gint)(gtk_spin_button_get_value(spinEvPeriodicNear) + adjnear)
                % evbuf->evPeriodicDays;
        }else{
            evbuf->evType = RET_TODO;
            evbuf->keep = gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(chkRemind));
        }
        if( evbuf->evType != RET_TODO ) {
            evbuf->keep = gtk_toggle_button_get_active(
                    GTK_TOGGLE_BUTTON(chkKeep));
        }
        evbuf->remind_days = gtk_spin_button_get_value(
                GTK_SPIN_BUTTON(daysBefore));
		reminderer_event_set_text(evbuf,
                gtk_entry_get_text(GTK_ENTRY(ctlText)));
	}
	gtk_widget_destroy(GTK_WIDGET(window));
    g_object_unref(G_OBJECT(g_xml));
	return status == 0;
}

void G_MODULE_EXPORT
on_radioByDate_toggled(GtkRadioButton *self, gpointer user_data)
{
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self)) ) {
		adjustSensitivity(RET_BYDATE);
    }
}

void G_MODULE_EXPORT
on_radioPeriodic_toggled (GtkRadioButton *self, gpointer user_data)
{
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self)) ) {
		adjustSensitivity(RET_PERIODIC);
    }
}

void G_MODULE_EXPORT
on_radioTodo_toggled (GtkRadioButton *self, gpointer user_data)
{
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self)) )
		adjustSensitivity(RET_TODO);
}

void G_MODULE_EXPORT
on_spinEvPeriodicDays_valueChanged(GtkSpinButton *self,
        gpointer user_data)
{
    GtkAdjustment *adjustNear;
    gint maxVal;

    adjustNear = GTK_ADJUSTMENT(gtk_builder_get_object(g_xml,
            "adjustEvPeriodicNear"));
    maxVal = gtk_spin_button_get_value(self) - 1;
    gtk_adjustment_set_upper(adjustNear, maxVal);
    if( gtk_adjustment_get_value(adjustNear) > maxVal )
        gtk_adjustment_set_value(adjustNear, maxVal);
}

