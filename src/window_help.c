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
#include "window_help.h"


void showWindowHelp(GtkWidget *owner)
{
    GtkBuilder *builder;
	GObject *window;

	builder = gtk_builder_new();
    gtk_builder_add_from_file(builder,
            "/org/rafaello7/reminderer/window_help.ui", NULL);
	gtk_builder_connect_signals(builder, NULL);
	window = gtk_builder_get_object (builder, "dialogHelp");
	gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(owner));
	gtk_dialog_run(GTK_DIALOG(window));
	gtk_widget_destroy(GTK_WIDGET(window));
    g_object_unref(G_OBJECT(builder));
}
