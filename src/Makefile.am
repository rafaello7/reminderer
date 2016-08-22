## Process this file with automake to produce Makefile.in

## Created by Anjuta

gladedir = $(datadir)/reminderer/gtkbuilder
glade_DATA = reminderer.ui \
             event_edit.ui \
             window_about.ui\
             window_help.ui \
             window_prefs.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(REMINDERER_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = reminderer

reminderer_SOURCES = \
	event_edit.c\
	event_edit.ui \
	event_edit.h \
	eventstore.c \
	eventstore.h \
    eventlists.c \
    eventlists.h \
	reminderer.c \
	reminderer.h \
	reminderer_event.c \
	reminderer_event.h \
	reminderer.ui \
	window_about.c \
	window_about.h \
	window_about.ui \
	window_help.c \
	window_help.h \
	window_help.ui \
    preferences.c \
    preferences.h \
    window_prefs.c \
    window_prefs.h \
    window_prefs.ui

reminderer_LDFLAGS = -export-dynamic

reminderer_LDADD = -lgtk-3 -lgdk-3 -lgio-2.0 -lgobject-2.0 -lglib-2.0 

EXTRA_DIST = $(glade_DATA)

# Remove glade directory on uninstall
uninstall-local:
	-rm -r $(gladedir)
	-rm -r $(datadir)/reminderer
