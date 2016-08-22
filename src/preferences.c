#include "reminderer.h"
#include "preferences.h"


static const gchar *get_appdir(void)
{
    static gchar *appDir;
    const gchar *dataDir;

    if( appDir == NULL ) {
        dataDir = g_get_user_data_dir();
        appDir = g_strconcat(dataDir, "/reminderer", NULL);
        if( g_mkdir_with_parents(appDir, 0755) != 0 ) {
            rmdr_showMessage(_("Unable to create directory %s"), appDir);
            g_free(appDir);
            appDir = NULL;
        }
    }
    return appDir;
}

static const struct reminderer_prefs *initPrefs(
        const struct reminderer_prefs *prefs_set)
{
    static struct reminderer_prefs gPrefs;
    static gboolean isInitialized = FALSE;
    const gchar *appDir;
    gchar *fname;
    GKeyFile *kf;

    if( prefs_set != NULL ) {
        g_free((gpointer)gPrefs.eventsFileName);
        gPrefs.eventsFileName = g_strdup(prefs_set->eventsFileName);
        gPrefs.coloredBgLighten = prefs_set->coloredBgLighten;
        gPrefs.useBoldInColored = prefs_set->useBoldInColored;
        isInitialized = TRUE;
    }else if( ! isInitialized ) {
        kf = g_key_file_new();
        if( (appDir = get_appdir()) != NULL ) {
            fname = g_strconcat(appDir, "/preferences.ini", NULL);
            g_key_file_load_from_file(kf, fname, G_KEY_FILE_NONE, NULL);
            g_free(fname);
            gPrefs.eventsFileName = g_key_file_get_string(kf, "global",
                    "events_file", NULL);
            if( gPrefs.eventsFileName == NULL ) {
                gPrefs.eventsFileName = g_strconcat(appDir, "/events.xml",
                        NULL);
            }
            gPrefs.coloredBgLighten = g_key_file_get_integer(kf, "global",
                    "colored_bg_lighten", NULL);
            gPrefs.useBoldInColored = g_key_file_has_key(kf, "global",
                    "bold_colored", NULL);
        }
        g_key_file_free(kf);
        isInitialized = TRUE;
    }
    return &gPrefs;
}

const gchar *prefs_getEventsFileName(void)
{
    const struct reminderer_prefs *prefs;

    prefs = initPrefs(NULL);
    return prefs->eventsFileName;
}

guint prefs_coloredBgLighten(void)
{
    const struct reminderer_prefs *prefs;

    prefs = initPrefs(NULL);
    return prefs->coloredBgLighten;
}

gboolean prefs_useBoldInColored(void)
{
    const struct reminderer_prefs *prefs;

    prefs = initPrefs(NULL);
    return prefs->useBoldInColored;
}

void prefs_setPreferences(const struct reminderer_prefs *prefs)
{
    const gchar *appDir;
    gchar *fname, *data;
    gsize dataLen;
    GKeyFile *kf;

    prefs = initPrefs(prefs);
    if( (appDir = get_appdir()) != NULL ) {
        fname = g_strconcat(appDir, "/preferences.ini", NULL);
        kf = g_key_file_new();
        g_key_file_set_string(kf, "global", "events_file",
                prefs->eventsFileName);
        g_key_file_set_integer(kf, "global", "colored_bg_lighten",
                prefs->coloredBgLighten);
        if( prefs->useBoldInColored ) {
            g_key_file_set_boolean(kf, "global", "bold_colored", TRUE);
        }
        data = g_key_file_to_data(kf, &dataLen, NULL);
        g_file_set_contents(fname, data, dataLen, NULL);
        g_free(data);
        g_key_file_free(kf);
    }
}

