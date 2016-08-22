#ifndef PREFERENCES_H
#define PREFERENCES_H

struct reminderer_prefs {
    const gchar *eventsFileName; /* full pathname of file containing events */

    guint coloredBgLighten;    /* in range 0 .. 100 */
    gboolean useBoldInColored; /* whether to use bold font
                                * in colored list cells */
};

const gchar *prefs_getEventsFileName(void);
guint prefs_coloredBgLighten(void);
gboolean prefs_useBoldInColored(void);

void prefs_setPreferences(const struct reminderer_prefs*);

#endif /* PREFERENCES_H */
