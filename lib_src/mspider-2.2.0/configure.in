dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)

AC_INIT(src/emspider.c)

mSpider_VERSION=2.2.0

dnl ========================================================================
dnl needed for cross-compiling
AC_CANONICAL_SYSTEM

dnl ========================================================================
dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LD
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AM_INIT_AUTOMAKE(mspider,$mSpider_VERSION,-)

dnl ========================================================================
dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

dnl ========================================================================
dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(sys/time.h unistd.h)

build_pthread="yes"

build_gif="yes"
build_animation="yes"
build_jpg="yes"
build_png="yes"
build_bmp="yes"
build_js="no"
build_flash="no"
build_ipv6="no"
build_ssl="no"
build_cookie="yes"
build_autofold="no"
build_tooltip="no"
build_findtext="no"
build_tvoutput="no"
build_linktrave="no"

AC_ARG_ENABLE(pthread,
[  --enable-pthread         enable PThread support <default=yes>],
build_pthread=$enableval)

AC_ARG_ENABLE(gif,
[  --enable-gif             enable GIF image support <default=yes>],
build_gif=$enableval)

AC_ARG_ENABLE(flash,
[  --enable-flash           enable FLASH image support <default=no>],
build_flash=$enableval)

AC_ARG_ENABLE(jpeg,
[  --enable-jpeg            enable JPEG image support <default=yes>],
build_jpg=$enableval)

AC_ARG_ENABLE(png,
[  --enable-png             enable PNG image support <default=yes>],
build_png=$enableval)

AC_ARG_ENABLE(bmp,
[  --enable-bmp             enable BMP image support <default=yes>],
build_bmp=$enableval)

AC_ARG_ENABLE(js,
[  --enable-js              enable JS engine  <default=no>],
build_js=$enableval)

AC_ARG_ENABLE(ipv6,
[  --enable-ipv6            enable IPv6 support <default=no>],
build_ipv6=$enableval)

AC_ARG_ENABLE(ssl,
[  --enable-ssl             enable SSL features (eg. https) <default=no>], 
build_ssl=$enableval)

AC_ARG_ENABLE(animation,
[  --enable-animation       enable GIF Animation support <default=yes>],
build_animation=$enableval)

AC_ARG_ENABLE(tvoutput,
[  --enable-tvoutput        rendering text for TV <default=no>],
build_tvoutput=$enableval)

AC_ARG_ENABLE(cookies,
[  --enable-cookies         include support for cookies <default=yes>],
build_cookie=$enableval)

AC_ARG_ENABLE(autofold,
[  --enable-autofold        enable autofold when you provide this option <default=no>],
build_autofold=$enableval)

AC_ARG_ENABLE(tooltip,
[  --enable-tooltip         enable tooltip when you provide this option <default=no>],
build_tooltip=$enableval)

AC_ARG_ENABLE(findtext,
[  --enable-findtext        enable findtext when you provide this option <default=no>],
build_findtext=$enableval)

AC_ARG_ENABLE(linktrave,
[  --enable-linktrave       enable linktrave when you provide this option <default=no>],
build_linktrave=$enableval)

dnl Check for JPEG library.
if test "x$build_jpg" != "xno"; then
  AC_CHECK_LIB(jpeg, jpeg_std_error,
  SYSTEM_LIBS="$SYSTEM_LIBS -ljpeg",
  build_jpg=no, -lm)
fi

dnl Check for PNG library.
if test "x$build_png" != "xno"; then
  AC_CHECK_LIB(png, png_check_sig,
  SYSTEM_LIBS="$SYSTEM_LIBS -lpng -lz",
  build_png=no, -lm)
fi

dnl Check for SpiderMonkey JS library.
if test "x$build_js" != "xno"; then
dnl  AC_CHECK_LIB(mozjs185, JS_Init, SYSTEM_LIBS="$SYSTEM_LIBS -lmozjs185", build_js=no, -lm)
AC_CHECK_LIB(js, JS_Init, SYSTEM_LIBS="$SYSTEM_LIBS -ljs", build_js=no, -lm)
fi

dnl Check for SSL Library

if test "x$build_ssl" = "xyes"; then
  AC_CHECK_HEADER(openssl/ssl.h, ssl_ok=yes, ssl_ok=no)

  if test "x$ssl_ok" = "xyes"; then
    old_libs="$LIBS"
    AC_CHECK_LIB(ssl, SSL_library_init, ssl_ok=yes, ssl_ok=no, -lcrypto)
    LIBS="$old_libs"
  fi

  if test "x$ssl_ok" = "xyes"; then
    SYSTEM_LIBS="-lcrypto -lssl"
  else
    AC_MSG_WARN([*** No libssl found. Disabling ssl support.***])
  fi
fi

if test "x$ssl_ok" = "xyes"; then
  AC_DEFINE(ENABLE_SSL, 1, [Enable SSL support])
fi

AM_CONFIG_HEADER(mgdconfig.h)

AC_DEFINE(EMSPIDER_VERSION, "2.2.0", [version of mSpider])

if test "x$build_pthread" = "xyes"; then
  AC_DEFINE(ENABLE_PTHREADS, 1, [Define if using PThreads])
fi

if test "x$build_gif" = "xyes"; then
  AC_DEFINE(ENABLE_GIF, 1, [Define if support GIF image format])
fi

if test "x$build_jpg" = "xyes"; then
  AC_DEFINE(ENABLE_JPEG, 1, [Define if support JPEG image format])
  SYSTEM_LIBS="$SYSTEM_LIBS -ljpeg"
fi

if test "x$build_png" = "xyes"; then
  AC_DEFINE(ENABLE_PNG, 1, [Define if support PNG image format])
  SYSTEM_LIBS="$SYSTEM_LIBS -lpng"
fi

if test "x$build_bmp" = "xyes"; then
  AC_DEFINE(ENABLE_BMP, 1, [Define if support BMP image format])
fi

if test "x$build_flash" = "xyes"; then
  AC_DEFINE(ENABLE_FLASH, 1, [Define if support FLASH image format])
fi

if test "x$build_js" = "xyes"; then
  AC_DEFINE(JS_SUPPORT, 1, [Define if support JS engine])
fi

if test "x$build_ipv6" = "xyes"; then
  AC_DEFINE(ENABLE_IPV6, 1, [Define if support IPv6])
fi

if test "x$build_animation" = "xyes"; then
  AC_DEFINE(ENABLE_ANIMATION, 1, [Define if support GIF Animation])
fi

if test "x$build_tvoutput" = "xyes" ; then
  AC_DEFINE(ENABLE_TVOUTPUT, 1, [Define if rendering text for TV])
fi

if test "x$build_cookie" = "xyes" ; then
  AC_DEFINE(ENABLE_COOKIES, 1, [Define if including support for cookies])
fi

if test "x$build_autofold" = "xyes" ; then
  AC_DEFINE(AUTOFOLD_SUPPORT, 1, [Define if enable autofold when you provide this option])
fi

if test "x$build_tooltip" = "xyes" ; then
  AC_DEFINE(TOOLTIP_SUPPORT, 1, [Define if enable tooltip when you provide this option])
fi

if test "x$build_findtext" = "xyes" ; then
  AC_DEFINE(ENABLE_FINDTEXT, 1, [Define if enable findtext when you provide this option])
fi

if test "x$build_linktrave" = "xyes" ; then
  AC_DEFINE(ENABLE_LINKTRAVE, 1, [Define if enable linktrave when you provide this option])
fi

AM_CONDITIONAL(BUILD_FLASH, test "x$build_flash" = "xyes")

AC_SUBST(SYSTEM_LIBS)

dnl ========================================================================
dnl check for MiniGUI
have_libminigui="no"
AC_CHECK_HEADERS(minigui/common.h, have_libminigui=yes, foo=bar)

PKG_CHECK_MODULES([MINIGUI], [minigui >= 3.0.10])
LIBS="$LIBS $MINIGUI_LIBS"

AC_OUTPUT(
Makefile
m4/Makefile
glib/Makefile
mpegsound/Makefile
flash/Makefile
include/Makefile
src/io/Makefile
src/js/Makefile
src/Makefile
test/Makefile
test/simple/Makefile
)

if test "x$have_libminigui" != "xyes"; then
    AC_MSG_WARN([
        MiniGUI is not properly installed on the system. You need 
        MiniGUI Ver 1.6.8 or later for building this package. 
        Please configure and install MiniGUI Ver 1.6.8 first.
    ])
fi

