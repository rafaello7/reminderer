/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include "reminderer.h"
#include "window_prefs.h"
#include "preferences.h"
#include <string.h>


void G_MODULE_EXPORT
on_eventFileEntry_icon_release(GtkEntry *ent,
        GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
    GtkWidget *fileChooser;
    char *dirname, *basename;

    fileChooser = gtk_file_chooser_dialog_new(_("Reminderer - events file"),
            GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(ent))),
            GTK_FILE_CHOOSER_ACTION_SAVE, _("OK"), GTK_RESPONSE_OK,
            _("Cancel"), GTK_RESPONSE_CANCEL, NULL);
    dirname = g_path_get_dirname(gtk_entry_get_text(ent));
    basename = g_path_get_basename(gtk_entry_get_text(ent));
    if( g_strcmp0(dirname, ".") ) {
        gtk_file_chooser_set_current_folder(
                GTK_FILE_CHOOSER(fileChooser), dirname);
    }
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fileChooser), basename);
    g_free(dirname);
    g_free(basename);
    if( gtk_dialog_run(GTK_DIALOG(fileChooser)) == GTK_RESPONSE_OK ) {
        gchar *fname = gtk_file_chooser_get_filename(
                GTK_FILE_CHOOSER(fileChooser));
        if( fname != NULL ) {
            gtk_entry_set_text(ent, fname);
            g_free(fname);
        }
    }
    gtk_widget_destroy(fileChooser);
}

gboolean showWindowPreferences(GtkWidget *owner)
{
    GtkBuilder *builder;
	GObject *window, *eventFileEntry, *adjLighten, *chkUseBoldFont;
    struct reminderer_prefs prefs;
    gboolean res;

	builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, GLADE_FILE("window_prefs.ui"), NULL);
	gtk_builder_connect_signals(builder, NULL);
    eventFileEntry = gtk_builder_get_object(builder, "eventFileEntry");
    adjLighten = gtk_builder_get_object(builder, "adjLighten");
    chkUseBoldFont = gtk_builder_get_object(builder, "chkUseBoldFont");
    gtk_entry_set_text(GTK_ENTRY(eventFileEntry), prefs_getEventsFileName());
    gtk_adjustment_set_value(GTK_ADJUSTMENT(adjLighten),
            prefs_coloredBgLighten());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chkUseBoldFont),
            prefs_useBoldInColored());
	window = gtk_builder_get_object(builder, "preferencesDialog");
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(owner));
	res = gtk_dialog_run(GTK_DIALOG(window)) == 0;
	if( res ) {
        prefs.eventsFileName = gtk_entry_get_text(GTK_ENTRY(eventFileEntry));
        prefs.coloredBgLighten = gtk_adjustment_get_value(
                GTK_ADJUSTMENT(adjLighten));
        prefs.useBoldInColored = gtk_toggle_button_get_active(
                GTK_TOGGLE_BUTTON(chkUseBoldFont));
        prefs_setPreferences(&prefs);
    }
	gtk_widget_destroy(GTK_WIDGET(window));
    g_object_unref(G_OBJECT(builder));
    return res;
}

