AC_INIT([ubavahid],[0.1.0],[schneider@blinkenlichts.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
CFLAGS="$CFLAGS -g -Wall -Wextra"

AC_CHECK_HEADER(avahi-core/core.h,LIBS="$LIBS -lavahi-core",
  AC_MSG_ERROR(avahi-core/core.h not found. Please install libavahi-dev.))

AC_CHECK_HEADER(avahi-common/address.h,LIBS="$LIBS -lavahi-common",
  AC_MSG_ERROR(avahi-core/core.h not found. Please install libavahi-dev.))

PKG_CHECK_MODULES(GLIB, glib-2.0 > 2.0.0,
  [LIBS="$LIBS $GLIB_LIBS" CFLAGS="$CFLAGS $GLIB_CFLAGS"],
    AC_MSG_ERROR(Cannot find glib: Is glib-config in path?))

PKG_CHECK_MODULES(GIO, gio-2.0 > 2.0.0,
  [LIBS="$LIBS $GIO_LIBS" CFLAGS="$CFLAGS $GIO_CFLAGS"],
    AC_MSG_ERROR(Cannot find gio: Is gio-config in path?))

PKG_CHECK_MODULES(JSON, json >= 0.9,
    [LIBS="$LIBS $JSON_LIBS" CFLAGS="$CFLAGS $JSON_CFLAGS"])

#PKG_CHECK_MODULES(GTHREAD, gthread-2.0,
#    [LIBS="$LIBS $GTHREAD_LIBS" CFLAGS="$CFLAGS $GTHREAD_CFLAGS"])
#PKG_CHECK_MODULES(AVAHIGLIB, avahi-glib >= 0.6.22,
#    [LIBS="$LIBS $AVAHIGLIB_LIBS" CFLAGS="$CFLAGS $AVAHIGLIB_CFLAGS"])
#PKG_CHECK_MODULES(AVAHICLIENT, avahi-client >= 0.6.22,
#    [LIBS="$LIBS $AVAHICLIENT_LIBS" CFLAGS="$CFLAGS $AVAHICLIENT_CFLAGS"])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
