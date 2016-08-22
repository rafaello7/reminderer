#include "reminderer.h"
#include "eventstore.h"
#include "eventlists.h"
#include "event_edit.h"
#include "window_about.h"
#include "window_help.h"
#include "window_prefs.h"
#include "preferences.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>


static GtkBuilder *g_xml;

enum NotebookTab {
    NT_REMINDS,
    NT_EVENTS,
    NT_TODOS,
    NT_PREVIEW
};

void rmdr_showMessage(const char *message, ...)
{
    GtkWindow *mainWindow = NULL;
    GString *msgStr;
	GtkWidget *msgBox;
    va_list args;

    msgStr = g_string_new(NULL);
    va_start(args, message);
    g_string_vprintf(msgStr, message, args);
    va_end(args);
    if( g_xml != NULL ) {
        mainWindow = GTK_WINDOW(gtk_builder_get_object(g_xml, "mainWindow"));
    }
    msgBox = gtk_message_dialog_new(mainWindow, GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, NULL);
    gtk_window_set_title(GTK_WINDOW(msgBox), "Reminderer");
    g_object_set(G_OBJECT(msgBox), "text", msgStr->str, NULL);
    g_string_free(msgStr, TRUE);
    gtk_dialog_run(GTK_DIALOG(msgBox));
    gtk_widget_destroy(msgBox);
}

static enum NotebookTab getActiveTab(void)
{
	GObject *notebook = gtk_builder_get_object(g_xml, "notebook");
	return gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
}

void G_MODULE_EXPORT
on_events_toggled (GtkRadioMenuItem *self, gpointer user_data)
{
	if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self)) ) {
        GObject *notebook = gtk_builder_get_object(g_xml, "notebook");
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), NT_EVENTS);
    }
}

void G_MODULE_EXPORT
on_reminds_toggled (GtkRadioMenuItem *self, gpointer user_data)
{
	if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self)) ) {
        GObject *notebook = gtk_builder_get_object(g_xml, "notebook");
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), NT_REMINDS);
    }
}

void G_MODULE_EXPORT
on_todos_toggled (GtkRadioMenuItem *self, gpointer user_data)
{
	if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self)) ) {
        GObject *notebook = gtk_builder_get_object(g_xml, "notebook");
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), NT_TODOS);
    }
}

void G_MODULE_EXPORT
on_preview_toggled (GtkRadioMenuItem *self, gpointer user_data)
{
	if( gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self)) ) {
        GObject *notebook = gtk_builder_get_object(g_xml, "notebook");
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), NT_PREVIEW);
    }
}

static void runEvEditDialog(evl_event_id eid)
{
	struct reminderer_event ev;

	reminderer_event_init(&ev);
    reminderer_event_set(&ev, evl_get_re(eid));
	if( run_EventEditDialog(
                GTK_WIDGET(gtk_builder_get_object (g_xml, "mainWindow")), &ev) )
    {
		evs_event_set(eid, &ev);
        switch( getActiveTab() ) {
        case NT_REMINDS:
            evl_select_in_treeview(eid,
                   GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "remindList")));
            break;
        case NT_PREVIEW:
            evl_select_in_treeview(eid,
                   GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "previewList")));
            break;
        default:
            break;
        }
    }
	reminderer_event_destroy(&ev);
}

void G_MODULE_EXPORT
on_add_activate (GtkImageMenuItem *self, gpointer user_data)
{
	reminderer_event re;
    evl_event_id eid;
	const GDate *curDate;

	reminderer_event_init(&re);
	if( getActiveTab() == NT_TODOS ) {
		re.evType = RET_TODO;
	}else{
        re.evType = RET_BYDATE;
        curDate = evs_get_cur_date();
		re.evByDateYear = 0;
        re.evByDateMonths = 1 << (g_date_get_month(curDate) - 1);
        re.evByDateDays = 1 << (g_date_get_day(curDate) - 1);
        re.evByDateWeekdays = 0x7F;
	}
    re.remind_days = 7;
	re.keep = TRUE;
	if( run_EventEditDialog(
                GTK_WIDGET(gtk_builder_get_object (g_xml, "mainWindow")), &re))
    {
        enum NotebookTab curTab = getActiveTab();
		eid = evs_event_add(&re);
        if( curTab == NT_REMINDS ) {
            evl_select_in_treeview(eid,
                   GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "remindList")));
        }
        evl_select_in_treeview(eid,
               GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "eventList")));
        evl_select_in_treeview(eid,
               GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "todoList")));
        if( curTab == NT_PREVIEW ) {
            evl_select_in_treeview(eid,
                   GTK_TREE_VIEW(gtk_builder_get_object(g_xml, "previewList")));
        }
    }
	reminderer_event_destroy(&re);
}

void G_MODULE_EXPORT
on_eventList_row_activated (GtkTreeView *self, GtkTreePath *path,
                            GtkTreeViewColumn *col, gpointer user_data)
{
    evl_event_id eid = evl_eidFromPath(EVL_EVENTS, path);
	if( eid )
        runEvEditDialog(eid);
}

void G_MODULE_EXPORT
on_remindList_row_activated (GtkTreeView *self, GtkTreePath *path,
                             GtkTreeViewColumn *col, gpointer user_data)
{
    evl_event_id eid;

    eid = evl_eidFromRemindPath(path);
	if( eid )
        runEvEditDialog(eid);
}

void G_MODULE_EXPORT
on_previewList_row_activated (GtkTreeView *self, GtkTreePath *path,
                             GtkTreeViewColumn *col, gpointer user_data)
{
    evl_event_id eid;

    eid = evl_eidFromPreviewPath(path);
	if( eid )
        runEvEditDialog(eid);
}

void G_MODULE_EXPORT
on_todoList_row_activated (GtkTreeView *self, GtkTreePath *path,
                           GtkTreeViewColumn *col, gpointer user_data)
{
    evl_event_id eid = evl_eidFromPath(EVL_TODOS, path);
	if( eid )
        runEvEditDialog(eid);
}

void G_MODULE_EXPORT
on_menuItemEdit_activate (GtkImageMenuItem *self, gpointer user_data)
{
    evl_event_id eid = NULL;
    GObject *list;
	GtkTreeIter iter;
    GtkTreeSelection *sel;

    switch( getActiveTab() ) {
    case NT_EVENTS:
		list = gtk_builder_get_object(g_xml, "eventList");
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
		if( !gtk_tree_selection_get_selected(sel, NULL, &iter) )
			return;
        eid = evl_eidFromList(EVL_EVENTS, &iter);
        break;
    case NT_TODOS:
		list = gtk_builder_get_object(g_xml, "todoList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( !gtk_tree_selection_get_selected(sel, NULL, &iter) )
			return;
        eid = evl_eidFromList(EVL_TODOS, &iter);
        break;
    case NT_PREVIEW:
		list = gtk_builder_get_object(g_xml, "previewList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( !gtk_tree_selection_get_selected(sel, NULL, &iter) )
			return;
        eid = evl_eidFromPreviewList(&iter);
        break;
    case NT_REMINDS:
		list = gtk_builder_get_object(g_xml, "remindList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( !gtk_tree_selection_get_selected(sel, NULL, &iter) )
			return;
        eid = evl_eidFromRemindList(&iter);
        break;
	}
    if( eid )
        runEvEditDialog(eid);
}

void G_MODULE_EXPORT
on_remove_activate (GtkImageMenuItem *self, gpointer user_data)
{
	GtkTreeIter iter;
    evl_event_id eid;
    evl_remind_id rid;
    GObject *list;
    GtkTreeSelection *sel;

    switch( getActiveTab() ) {
    case NT_EVENTS:
		list = gtk_builder_get_object(g_xml, "eventList");
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
		if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            eid = evl_eidFromList(EVL_EVENTS, &iter);
			evs_event_del(eid);
        }
        break;
    case NT_TODOS:
		list = gtk_builder_get_object(g_xml, "todoList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            eid = evl_eidFromList(EVL_TODOS, &iter);
			evs_event_del(eid);
        }
        break;
    case NT_PREVIEW:
		list = gtk_builder_get_object(g_xml, "previewList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            rid = evl_ridFromPreviewList(&iter);
            evs_remind_del(rid);
        }
        break;
    case NT_REMINDS:
		list = gtk_builder_get_object(g_xml, "remindList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            rid = evl_ridFromRemindList(&iter);
            evs_remind_del(rid);
        }
        break;
	}
}

void G_MODULE_EXPORT
on_undelete_activate (GtkImageMenuItem *self, gpointer user_data)
{
	GtkTreeIter iter;
    evl_event_id eid;
    evl_remind_id rid;
    GObject *list;
    GtkTreeSelection *sel;

    switch( getActiveTab() ) {
    case NT_EVENTS:
		list = gtk_builder_get_object(g_xml, "eventList");
		sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
		if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            eid = evl_eidFromList(EVL_EVENTS, &iter);
			evs_event_undel(eid);
        }
        break;
    case NT_TODOS:
		list = gtk_builder_get_object(g_xml, "todoList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            eid = evl_eidFromList(EVL_TODOS, &iter);
			evs_event_undel(eid);
        }
        break;
    case NT_PREVIEW:
		list = gtk_builder_get_object(g_xml, "previewList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            rid = evl_ridFromPreviewList(&iter);
            evs_remind_undel(rid);
        }
        break;
    case NT_REMINDS:
		list = gtk_builder_get_object(g_xml, "remindList");
        sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(list));
        if( gtk_tree_selection_get_selected(sel, NULL, &iter) ) {
            rid = evl_ridFromRemindList(&iter);
            evs_remind_undel(rid);
        }
        break;
	}
}

static GtkCellRenderer *gColorRenderer;

void G_MODULE_EXPORT
on_preferences_activate (GtkImageMenuItem *self, gpointer user_data)
{
    gboolean oldUseBold, useBold;
    gchar *oldEventsFName;
    gint oldColoredBgLighten, coloredBgLighten;
    const gchar *eventsFName;
    gchar *parseErrors;
    GtkWidget *mainWindow;

    oldColoredBgLighten = prefs_coloredBgLighten();
    oldUseBold = prefs_useBoldInColored();
    oldEventsFName = g_strdup(prefs_getEventsFileName());
    mainWindow = GTK_WIDGET(gtk_builder_get_object(g_xml, "mainWindow"));
	if( showWindowPreferences(mainWindow) ) {
        useBold = prefs_useBoldInColored();
        if( useBold != oldUseBold ) {
            g_object_set(G_OBJECT(gColorRenderer), "weight", 
                    useBold ? 600 : 400, NULL);
        }
        eventsFName = prefs_getEventsFileName();
        if( g_strcmp0(oldEventsFName, eventsFName) ) {
            evs_load_file(eventsFName, &parseErrors);
            if( parseErrors != NULL ) {
                rmdr_showMessage(parseErrors);
                g_free(parseErrors);
            }
        }else{
            // list not recreated: update existing list display attributes
            coloredBgLighten = prefs_coloredBgLighten();
            if( coloredBgLighten != oldColoredBgLighten ) {
                evl_changed_disp_preferences();
            }
        }
    }
    g_free(oldEventsFName);
}

void G_MODULE_EXPORT
on_showDeleted_toggled (GtkCheckMenuItem *self, gpointer user_data)
{
	gboolean checked = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(self));
	evl_showdeleted_set(checked);
}

void G_MODULE_EXPORT
on_helpContents_activate (GtkImageMenuItem *self, gpointer user_data)
{
	showWindowHelp(GTK_WIDGET(gtk_builder_get_object (g_xml, "mainWindow")));
}

void G_MODULE_EXPORT
on_about_activate (GtkImageMenuItem *self, gpointer user_data)
{
	showWindowAbout(GTK_WIDGET(gtk_builder_get_object (g_xml, "mainWindow")));
}

static gboolean on_button_press_event (GtkTreeView *treeview, GdkEvent *event)
{
    GtkTreePath *path;
	GObject *menu;
	GdkEventButton *evButt = &event->button;

    if(evButt->type == GDK_BUTTON_PRESS  &&  evButt->button == 3) {
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                    (gint) evButt->x, (gint) evButt->y,
                    &path, NULL, NULL, NULL))
        {
            gtk_tree_view_set_cursor(treeview, path, NULL, FALSE);
			menu = gtk_builder_get_object(g_xml, "popup");
			gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			               evButt->button, gdk_event_get_time(event));
            gtk_tree_path_free(path);
        }
        return TRUE;
    }
    return FALSE;
}

gboolean G_MODULE_EXPORT
on_remindList_button_press_event (GtkTreeView *treeview, GdkEvent *event,
                                  gpointer user_data)
{
	return on_button_press_event(treeview, event);
}

gboolean G_MODULE_EXPORT
on_previewList_button_press_event (GtkTreeView *treeview, GdkEvent *event,
                                  gpointer user_data)
{
	return on_button_press_event(treeview, event);
}

gboolean G_MODULE_EXPORT
on_eventList_button_press_event (GtkTreeView *treeview, GdkEvent *event,
                                 gpointer user_data)
{
	return on_button_press_event(treeview, event);
}

gboolean G_MODULE_EXPORT
on_todoList_button_press_event (GtkTreeView *treeview, GdkEvent *event,
                                gpointer user_data)
{
	return on_button_press_event(treeview, event);
}

static void prepareTreeViewForEvents()
{
	GObject *view;
	GtkCellRenderer     *renderer;
	GtkListStore	*store;

	view = gtk_builder_get_object(g_xml, "eventList");
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("T"), gColorRenderer, "background-rgba", EVL_EVCOL_COLOR,
          "text", EVL_EVCOL_TYPE, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Date"), renderer, "text", EVL_EVCOL_DATE,
          "strikethrough", EVL_EVCOL_DELETED, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Event"), renderer, "text", EVL_EVCOL_TEXT,
          "strikethrough", EVL_EVCOL_DELETED, NULL);
	store = evl_events_get();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
}

static void prepareTreeViewForReminds()
{
	GObject *view;
	GtkCellRenderer     *renderer;
	GtkListStore	*store;

	view = gtk_builder_get_object(g_xml, "remindList");
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Ds"), gColorRenderer, "background-rgba", EVL_RMDCOL_COLOR,
	      "text", EVL_RMDCOL_DAYS, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Date"), renderer, "text", EVL_RMDCOL_DATE,
	      "strikethrough", EVL_RMDCOL_DELETED, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Event"), renderer, "text", EVL_RMDCOL_TEXT,
          "strikethrough", EVL_RMDCOL_DELETED, NULL);
	store = evl_reminds_get();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
}

static void prepareTreeViewForPreview()
{
	GObject *view;
	GtkCellRenderer     *renderer;
	GtkListStore	*store;

	view = gtk_builder_get_object(g_xml, "previewList");
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Ds"), gColorRenderer, "background-rgba", EVL_RMDCOL_COLOR,
	      "text", EVL_RMDCOL_DAYS, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Date"), renderer, "text", EVL_RMDCOL_DATE,
	      "strikethrough", EVL_RMDCOL_DELETED, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Event"), renderer, "text", EVL_RMDCOL_TEXT,
          "strikethrough", EVL_RMDCOL_DELETED, NULL);
	store = evl_preview_get();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
}

static void prepareTreeViewForTodos()
{
	GObject *view;
	GtkCellRenderer     *renderer;
	GtkListStore	*store;

	view = gtk_builder_get_object(g_xml, "todoList");
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("R"), gColorRenderer, "background-rgba", EVL_TODOCOL_COLOR,
          "text", EVL_TODOCOL_MARK, NULL);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view),
          -1, _("Todo"), renderer, "text", EVL_TODOCOL_TEXT,
          "strikethrough", EVL_TODOCOL_DELETED, NULL);
	store = evl_todos_get();
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(store));
}

static GtkWindow *prepareMainWindow(void)
{
	GtkWidget *window;
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	char *iconsDir;

	/* add search path in case when "datadir" is non-standard one */
	iconsDir = g_strconcat(PACKAGE_DATA_DIR, "/icons", NULL);
	gtk_icon_theme_append_search_path(theme, iconsDir);
	g_free(iconsDir);
	gtk_window_set_default_icon_name("reminderer");
	g_xml = gtk_builder_new();
    if( !gtk_builder_add_from_file(g_xml, GLADE_FILE("reminderer.ui"), NULL) )
	{
        g_object_unref(G_OBJECT(g_xml));
        g_xml = NULL;
        rmdr_showMessage(
            _("reminderer installation problem: unable to open config file"));
		return NULL;
	}
	gtk_builder_connect_signals(g_xml, NULL);
    gColorRenderer = gtk_cell_renderer_text_new ();
    g_object_set(G_OBJECT(gColorRenderer), "xalign", 0.5, NULL);
    g_object_set(G_OBJECT(gColorRenderer), "weight", 
                prefs_useBoldInColored() ? 600 : 400, NULL);
	prepareTreeViewForEvents();
	prepareTreeViewForReminds();
	prepareTreeViewForPreview();
	prepareTreeViewForTodos();
    window = GTK_WIDGET(gtk_builder_get_object(g_xml, "showDeleted"));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(window),
            evl_showdeleted_get());
	return GTK_WINDOW(gtk_builder_get_object (g_xml, "mainWindow"));
}

static gint calcTimeOffset(const gchar *optCurTimeStrParam)
{
    GDateTime *realCurTime, *optCurTime;
    GTimeSpan diff;
    gint res = 0;
    gint len, year, month, day, hour, min, sec;
    gchar *optCurTimeStr;

    if( optCurTimeStrParam != NULL && optCurTimeStrParam[0] ) {
        optCurTimeStr = g_strdup(optCurTimeStrParam);
        len = strlen(optCurTimeStr);
        realCurTime = g_date_time_new_now_local();
        if( optCurTimeStr[len-1] == 'X' ) {
            hour = 23;
            min = 59;
            sec = 55;
            optCurTimeStr[--len] = '\0';
        }else if( len >= 5 ) {
            min = atoi(optCurTimeStr + len - 2);
            optCurTimeStr[len-2] = '\0';
            hour = atoi(optCurTimeStr + len - 4);
            len -= 4;
            optCurTimeStr[len] = '\0';
            sec = 0;
        }else{
            hour = g_date_time_get_hour(realCurTime);
            min = g_date_time_get_minute(realCurTime);
            sec = g_date_time_get_second(realCurTime);
        }
        if( len >= 3 ) {
            day = atoi(optCurTimeStr + len - 2);
            optCurTimeStr[len-2] = '\0';
            month = atoi(optCurTimeStr);
        }else{
            day = atoi(optCurTimeStr);
            month = g_date_time_get_month(realCurTime);
        }
        year = g_date_time_get_year(realCurTime);
        optCurTime = g_date_time_new_local(year, month, day, hour, min, sec);
        if( optCurTime == NULL ) {
            g_print("invalid date parameter: %s\n", optCurTimeStrParam);
            exit(1);
        }
        g_free(optCurTimeStr);
        diff = g_date_time_difference(optCurTime, realCurTime);
        g_date_time_unref(realCurTime);
        g_date_time_unref(optCurTime);
        res = diff / 1000000;
    }
    return res;
}

static gint onAppCmdLine(GApplication *app, GApplicationCommandLine *cmdLine,
        gpointer user_data)
{
    static gboolean isInitialized = FALSE;
    GtkWindow *window = NULL;
    gchar *parseErrors = NULL;
    GOptionContext *optContext;
    GError *error = NULL;
    gboolean optIsSilent = FALSE;
    gchar *optCurTime = NULL;
    GOptionEntry optEntries[] = {
      { "silent", 'n', 0, G_OPTION_ARG_NONE, &optIsSilent,
          _("Don't start when reminder list is empty"), NULL},
      { "time", 't', 0, G_OPTION_ARG_STRING, &optCurTime,
          _("Time to use instead of current (X=23:59:55)"),
          "[MM]DD['X'|HHMI]" },
      { NULL }
    };
    gint argc;
    gchar **argv;

    argv = g_application_command_line_get_arguments(cmdLine, &argc);
    optContext = g_option_context_new (NULL);
    g_option_context_add_main_entries(optContext, optEntries, NULL);
    g_option_context_add_group(optContext, gtk_get_option_group(TRUE));
    if( !g_option_context_parse(optContext, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit(1);
    }
    g_option_context_free(optContext);
    if( ! isInitialized ) {
        evs_init(calcTimeOffset(optCurTime));
        evs_load_file(prefs_getEventsFileName(), &parseErrors);
        isInitialized = TRUE;
    }
    g_free(optCurTime);
    if( ! optIsSilent || !evl_remind_is_empty() ) {
        if( g_xml == NULL ) {
            if( (window = prepareMainWindow()) != NULL ) {
                gtk_application_add_window(GTK_APPLICATION(app), window);
                gtk_widget_show(GTK_WIDGET(window));
            }
        }else{
            window = GTK_WINDOW(gtk_builder_get_object(g_xml,
                        "mainWindow"));
            gtk_window_present(window);
        }
    }
    if( parseErrors != NULL ) {
        rmdr_showMessage(parseErrors);
        g_free(parseErrors);
    }
    return 0;
}

int main (int argc, char *argv[])
{
    int ret;
	GtkApplication *app;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
    setlocale(LC_ALL, "");
    app = gtk_application_new("net.sourceforge.reminderer",
		G_APPLICATION_HANDLES_COMMAND_LINE);
    g_signal_connect(G_OBJECT(app), "command-line",
            G_CALLBACK(onAppCmdLine), NULL);
    ret = g_application_run(G_APPLICATION(app), argc, argv);
    if( g_application_get_is_remote(G_APPLICATION(app)) ) {
        g_print(_("reminderer application is already running\n"));
    }else if( g_xml != NULL ) {
        g_object_unref(G_OBJECT(g_xml));
        g_xml = NULL;
        evs_purge_deleted_events();
    }
    return ret;
}

