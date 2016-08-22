@INTLTOOL_DESKTOP_RULE@

dist_man_MANS = reminderer.1

appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = reminderer.png

desktopdir       = $(datadir)/applications
desktop_in_files = reminderer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = reminderer-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

EXTRA_DIST = $(appicon_DATA) \
             $(desktop_in_files) \
             $(autostart_in_files)

DISTCLEANFILES = $(desktop_DATA) \
                 $(autostart_DATA)
