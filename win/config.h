#ifndef CONFIG_H
#define CONFIG_H

#define VERSION "0.5"

const char *getPackageDataDir();
const char *getGladeFilePathname(const char *fname);
#define PACKAGE_DATA_DIR    getPackageDataDir()
#define GLADE_FILE(fname)	getGladeFilePathname(fname)

#define _CRT_SECURE_NO_WARNINGS

#endif /* CONFIG_H */
