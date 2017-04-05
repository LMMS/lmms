AC_INIT([swh-plugins], [0.4.17])
AC_CONFIG_SRCDIR([amp_1181.xml])
AC_CANONICAL_SYSTEM
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE

AM_DISABLE_STATIC

dnl @synopsis AC_C99_FUNC_LRINTF
dnl
dnl Check whether C99's lrintf function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINTF],
[AC_CACHE_CHECK(for lrintf,
  ac_cv_c99_lrintf,
[
lrintf_save_CFLAGS=$CFLAGS
CFLAGS="-O3 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrintf(3.14159)) lrintf(2.7183);, ac_cv_c99_lrintf=yes, ac_cv_c99_lrintf=no)

CFLAGS=$lrintf_save_CFLAGS

])

if test "$ac_cv_c99_lrintf" = yes; then
  AC_DEFINE(HAVE_LRINTF, 1,
            [Define if you have C99's lrintf function.])
fi
])# AC_C99_FUNC_LRINTF

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_LANG_C
AC_PROG_CC
AM_PROG_CC_C_O
AC_REQUIRE_CPP
ALL_LINGUAS="en_GB"
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.3])
AC_C_BIGENDIAN

LIBS="$LIBS -lm"
PKG_CHECK_MODULES(FFTW, fftw3f >= 3.0,
    [
	echo "Using FFTW 3";
	AC_DEFINE([FFTW3], [], [Wether were using FFTW version 3])
    ],
    [ AC_CHECK_LIB(sfftw,fftw_one,
        [
            FFTW_LIBS="-lsrfftw -lsfftw"
	    STATIC_FFTW_LIBS="$LIBS /usr/lib/libsfftw.a /usr/lib/libsrfftw.a /sw/lib/libsfftw.a"
	    AC_DEFINE_UNQUOTED(EXPLICIT_S, "")
        ],
        [ AC_CHECK_LIB(fftw,fftw_one,
            [
                FFTW_LIBS="-lrfftw -lfftw"
	        STATIC_FFTW_LIBS="$LIBS /usr/lib/libfftw.a /usr/lib/librfftw.a"
	    ],
	    [
	        AC_MSG_ERROR([Could not find working FFTW library (http://www.fftw.org/). If you have
installed FFTW3 check that you used the right build options, see the README.]);
	    ])
        ])
    ])

# On Mac OS X the float functions for <math.h> are in -lmx
AC_CHECK_LIB([m], [sqrt],, [AC_MSG_ERROR(Can't find libm)])
AC_CHECK_LIB(m, log10f,, [
        AC_CHECK_LIB(mx, log10f,,
                [AC_MSG_ERROR([Can't find float libm])])])

AC_CHECK_FUNC(shm_open,,[AC_CHECK_LIB(rt,shm_open)])
AC_C99_FUNC_LRINTF()

AC_ARG_ENABLE(3dnow, [  --enable-3dnow Enables 3DNow! acceleration], AS_IF([test "x$enableval" = "xyes"], [AC_DEFINE_UNQUOTED(ACCEL_3DNOW, "")]))
AC_ARG_ENABLE(sse, [  --enable-sse Uses SSE instructions where possible, requires gcc3 and a processor with SSE support], AS_IF([test "x$enableval" = "xyes"], [USE_SSE="-msse -mfpmath=sse -malign-double"]))
AC_ARG_ENABLE(darwin, [  --enable-darwin Builds plugins that will be shared object in the Darwin OS], AS_IF([test "x$enableval" = "xyes"], [DARWIN_CFLAGS="-fno-common -flat_namespace -bundle -undefined suppress -lbundle1.o"]))

CFLAGS="$lrintf_save_CFLAGS -I@top_srcdir@/intl -I@top_srcdir@"

check_cc_flag() {
  echo "int main(void){return 0;}" | $CC $CFLAGS $1 -o /dev/null -x c - 2> /dev/null
}

if [ echo ${CFLAGS} | grep "\-march=" ]; then
  AC_MSG_WARN([CFLAGS appears to allready contain architecture specifaction, using exiting one])
else
  AC_MSG_WARN([Can't find architecture specifaction in CFLAGS])

  CFLAGS="$CFLAGS -Wall -O3 -fomit-frame-pointer -funroll-loops -ffast-math -fPIC -DPIC ${USE_SSE} ${DARWIN_CFLAGS}"

  printf %s "checking if $CC supports -fstrength-reduce... "
  if check_cc_flag -fstrength-reduce; then
    echo yes
    CFLAGS="$CFLAGS -fstrength-reduce"
  else
    echo no
  fi

  dnl For Intel's C compiler use:
  dnl CC="icc"
  dnl CFLAGS="$CFLAGS -O2 -rcd -tpp6 -xiMK -KPIC -DPIC"
  
  dnl For debugging use:
  dnl CFLAGS="$CFLAGS -Wall -g -ffast-math -fPIC -DPIC"
fi

AC_PROG_LD
AM_PROG_LIBTOOL

dnl Set PACKAGE_LOCALE_DIR in config.h.
if test "x${prefix}" = xNONE; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${ac_default_prefix}/${DATADIRNAME}/locale")
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, "${prefix}/${DATADIRNAME}/locale")
fi

dnl Set PACKAGE_DATA_DIR in config.h.
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = xNONE; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}")
fi

subdirs="util gsm gverb metadata"

AC_SUBST(subdirs)
AC_SUBST(FFTW_LIBS)
AC_SUBST(STATIC_FFTW_LIBS)
AC_SUBST(FFTW_CFLAGS)
AC_SUBST(LIBTOOL)
AC_OUTPUT([
Makefile
util/Makefile
gsm/Makefile
gverb/Makefile
metadata/Makefile
po/Makefile.in
])
