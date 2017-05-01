AC_PREREQ([2.69])
AC_INIT(reminderer, 0.6.1, https://github.com/rafaello7/reminderer)
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/reminderer.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
IT_PROG_INTLTOOL

# Checks for libraries.
PKG_CHECK_MODULES(LIBGTK, [gtk+-3.0])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_CHECK_FUNCS([memset setlocale strerror strtol])

# internationalization
GETTEXT_PACKAGE=reminderer
AC_SUBST(GETTEXT_PACKAGE)

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
data/Makefile
debian/Makefile
])

AC_OUTPUT
