UI = reminderer.ui \
     event_edit.ui \
     window_about.ui\
     window_help.ui \
     window_prefs.ui

bin_PROGRAMS = reminderer

reminderer_SOURCES = \
	event_edit.c\
	event_edit.h \
	eventstore.c \
	eventstore.h \
    eventlists.c \
    eventlists.h \
	reminderer.c \
	reminderer.h \
	reminderer_event.c \
	reminderer_event.h \
	window_about.c \
	window_about.h \
	window_help.c \
	window_help.h \
    preferences.c \
    preferences.h \
    window_prefs.c \
    window_prefs.h \
    $(UI)

nodist_reminderer_SOURCES = resources.c

reminderer_CFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(LIBGTK_CFLAGS)

reminderer_LDADD = $(LIBGTK_LIBS)
reminderer_LDFLAGS = -rdynamic

EXTRA_DIST = reminderer.gresource.xml

resources.c: reminderer.gresource.xml $(UI)
	glib-compile-resources --sourcedir=$(srcdir) $< --target=$@ \
		--generate-source

CLEANFILES = resources.c

