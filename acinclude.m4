# Check for Qt compiler flags, linker flags, and binary packages
AC_DEFUN([gw_CHECK_QT],
[
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_PATH_X])

AC_MSG_CHECKING([QTDIR])
AC_ARG_WITH([qtdir], [  --with-qtdir=DIR        Qt installation directory [default=$QTDIR]], QTDIR=$withval)
# Check that QTDIR is defined or that --with-qtdir given
if test x"$QTDIR" = x ; then
	# some usual Qt-locations
	QT_SEARCH="/usr /usr/lib/qt /usr/lib/qt3 /usr/lib/qt31 /usr/lib/qt32 /usr/lib/qt33 /usr/lib/qt-3.0 /usr/lib/qt-3.1 /usr/lib/qt-3.2 /usr/lib/qt-3.3 /usr/local/qt /usr/local/qt3 /usr/local/qt31 /usr/local/qt32 /usr/local/qt33 /usr/share/qt3 /usr/X11R6" # /usr/share/qt4 /usr/local/Trolltech/Qt-4.0.0 /usr/local/Trolltech/Qt-4.0.1 /usr/local/Trolltech/Qt-4.1.0 /usr/local/Trolltech/Qt-4.1.0"
else
	QT_SEARCH=$QTDIR
	QTDIR=""
fi
for i in $QT_SEARCH ; do
	QT_INCLUDE_SEARCH="include include/qt include/qt3" # include/qt4/Qt include/Qt"
	for j in $QT_INCLUDE_SEARCH ; do
	        if test -f $i/$j/qglobal.h -a x$QTDIR = x ; then
			QTDIR=$i
			QT_INCLUDES=$i/$j
		fi
	done
done
if test x"$QTDIR" = x ; then
	AC_MSG_ERROR([*** QTDIR must be defined, or --with-qtdir option given])
fi
AC_MSG_RESULT([$QTDIR])

# Change backslashes in QTDIR to forward slashes to prevent escaping
# problems later on in the build process, mainly for Cygwin build
# environment using MSVC as the compiler
# TODO: Use sed instead of perl
QTDIR=`echo $QTDIR | perl -p -e 's/\\\\/\\//g'`

AC_MSG_CHECKING([Qt includes])
# Check where includes are located
if test x"$QT_INCLUDES" = x ; then
	AC_MSG_ERROR([*** could not find Qt-includes! Make sure you have the Qt-devel-files installed!])
fi
AC_MSG_RESULT([$QT_INCLUDES])

# Figure out which version of Qt we are using
AC_MSG_CHECKING([Qt version])
QT_VER=`grep 'define.*QT_VERSION_STR\W' $QT_INCLUDES/qglobal.h | perl -p -e 's/\D//g'`
case "${QT_VER}" in
    2*)
	AC_MSG_ERROR([*** Qt 2 is not supported by iTALC. Please upgrade to Qt3!])
    ;;
    3*)
        QT_MAJOR="3"
    ;;
#    4*)
#	QT_MAJOR="4"
#    ;;
    *)
        AC_MSG_ERROR([*** Don't know how to handle this Qt major version])
    ;;
esac
AC_MSG_RESULT([$QT_VER ($QT_MAJOR)])

# Search for available Qt translations
AH_TEMPLATE(QT_TRANSLATIONS_DIR, [Define to Qt translations directory])
AC_MSG_CHECKING([Qt translations])
case "${QT_VER}" in
    3*)
        QT_TRANSLATIONS_SEARCH="/usr/share/qt3 /usr/local/qt3 /usr/local/qt31 /usr/local/qt32 /usr/local/qt33 /usr/local/qt"
    ;;
    4*)
        QT_TRANSLATIONS_SEARCH="/usr/share/qt4 /usr/local/qt /usr/local/Trolltech/Qt-4.0.0 /usr/local/Trolltech/Qt-4.0.1 /usr/local/Trolltech/Qt-4.1.0 /usr/local/Trolltech/Qt-4.1.0"
    ;;
esac
for i in $QT_TRANSLATIONS_SEARCH ; do
    if test -d $i/translations -a x$QT_TRANSLATIONS = x ; then
        QT_TRANSLATIONS=$i/translations
    fi
done
if test x"$QT_TRANSLATIONS" = x ; then
    AC_MSG_WARN([*** not found! You may want to install a Qt i18n package])
else
    AC_DEFINE_UNQUOTED(QT_TRANSLATIONS_DIR, "$QT_TRANSLATIONS")
fi
AC_MSG_RESULT([$QT_TRANSLATIONS])

QTHOSTDIR=/usr

# Check that moc is in path
AC_CHECK_PROG(MOC, moc, $QTDIR/bin/moc,,$QTDIR/bin/)
if test x$MOC = x ; then
	AC_CHECK_PROG(MOC, moc-qt3, $QTDIR/bin/moc-qt3,,$QTDIR/bin/)
	if test x$MOC = x ; then
		AC_CHECK_PROG(MOC, moc-qt4, $QTHOSTDIR/bin/moc-qt4,,$QTHOSTDIR/bin/)
		if test x$MOC = x ; then
        		AC_MSG_ERROR([*** not found! Make sure you have Qt-devel-tools installed!])
		fi
	fi
fi

# uic is the Qt user interface compiler
AC_CHECK_PROG(UIC, uic, $QTDIR/bin/uic,,$QTDIR/bin/)
if test x$UIC = x ; then
        AC_MSG_WARN([*** not found! It's currently not needed but should be part of a proper Qt-devel-tools-installation!])
fi

# lupdate is the Qt translation-update utility.
AC_CHECK_PROG(LUPDATE, lupdate, $QTDIR/bin/lupdate,,$QTDIR/bin/)
if test x$LUPDATE = x ; then
        AC_MSG_WARN([*** not found! It's not needed just for compiling but should be part of a proper Qt-devel-tools-installation!])
fi

# lrelease is the Qt translation-release utility.
AC_CHECK_PROG(LRELEASE, lrelease, $QTDIR/bin/lrelease,,$QTDIR/bin/)
if test x$LRELEASE = x ; then
        AC_MSG_WARN([*** not found! It's not needed just for compiling but should be part of a proper Qt-devel-tools-installation!])
fi

# Calculate Qt include path
QT_CXXFLAGS="-I$QT_INCLUDES"
if test "$QT_MAJOR" = "4" ; then
	QT_CXXFLAGS="$QT_CXXFLAGS -I$QTDIR/include/qt4 -I$QTDIR/include"
fi


QT_IS_EMBEDDED="no"
# On unix, figure out if we're doing a static or dynamic link
case "${host}" in
    *-cygwin)
	AC_DEFINE_UNQUOTED(WIN32, "", Defined if on Win32 platform)
        if test -f "$QTDIR/lib/qt.lib" ; then
            QT_LIB="qt.lib"
            QT_IS_STATIC="yes"
            QT_IS_MT="no"
        elif test -f "$QTDIR/lib/qt-mt.lib" ; then
            QT_LIB="qt-mt.lib" 
            QT_IS_STATIC="yes"
            QT_IS_MT="yes"
        elif test -f "$QTDIR/lib/qt$QT_VER.lib" ; then
            QT_LIB="qt$QT_VER.lib"
            QT_IS_STATIC="no"
            QT_IS_MT="no"
        elif test -f "$QTDIR/lib/qt-mt$QT_VER.lib" ; then
            QT_LIB="qt-mt$QT_VER.lib"
            QT_IS_STATIC="no"
            QT_IS_MT="yes"
        fi
        ;;

      *mingw32)
	QT_IS_MT="yes"
	QT_LIB="-L$QTDIR/bin -lQtCore4 -lQtGui4 -lQtXml4 -lQt3Support4"
        ;;
    *)
        QT_IS_STATIC=`ls $QTDIR/lib/*.a 2> /dev/null`
       	if test "x$QT_IS_STATIC" = x; then
            QT_IS_STATIC="no"
       	else
            QT_IS_STATIC="yes"
        fi
        if test x$QT_IS_STATIC = xno ; then
            QT_IS_DYNAMIC=`ls $QTDIR/lib/*.so 2> /dev/null` 
            if test "x$QT_IS_DYNAMIC" = x;  then
                QT_IS_DYNAMIC=`ls /usr/lib/libQt*so.4 2> /dev/null` 
                if test "x$QT_IS_DYNAMIC" = x;  then
                    AC_MSG_ERROR([*** Couldn't find any Qt libraries])
		fi
            fi
        fi
	if test "$QT_MAJOR" = "4" ; then
		QT_IS_MT="yes"
		QT_LIB="-lQtCore -lQtGui -lQtXml -lQt3Support"
		MOC="$MOC -DLADSPA_SUPPORT"
	else
        	QT_CXXFLAGS="-DQT3 $QT_CXXFLAGS"
	        if test "x`ls $QTDIR/lib/libqt-mt.* 2> /dev/null`" != x ; then
	            QT_LIB="-lqt-mt"
	            QT_IS_MT="yes"
	        elif test "x`ls $QTDIR/lib/libqt.* 2> /dev/null`" != x ; then
	            QT_LIB="-lqt"
	            QT_IS_MT="no"
	        elif test "x`ls $QTDIR/lib/libqte.* 2> /dev/null`" != x ; then
	            QT_LIB="-lqte"
	            QT_IS_MT="no"
	            QT_IS_EMBEDDED="yes"
	        elif test "x`ls $QTDIR/lib/libqte-mt.* 2> /dev/null`" != x ; then
	            QT_LIB="-lqte-mt"
	            QT_IS_MT="yes"
	            QT_IS_EMBEDDED="yes"
	        fi
	fi
        ;;
esac
AC_MSG_CHECKING([if Qt is static])
AC_MSG_RESULT([$QT_IS_STATIC])
AC_MSG_CHECKING([if Qt is multithreaded])
if test "$QT_IS_MT" = "no"; then
	AC_MSG_ERROR([*** your Qt is not multithreaded. That's bad, because multithreading is required for compiling... Please install Qt-mt!])
fi
AC_MSG_RESULT([$QT_IS_MT])
AC_MSG_CHECKING([if Qt is embedded])
AC_MSG_RESULT([$QT_IS_EMBEDDED])

QT_GUILINK=""
QASSISTANTCLIENT_LDADD="-lqassistantclient"
QT_LIBS="$QT_LIB"
x_libraries="$x_libraries -L/usr/X11R6/lib"

case "${host}" in
    *irix*)
        QT_LIBS="$QT_LIB"
        if test $QT_IS_STATIC = yes ; then
            QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE"
        fi
        ;;

    *linux*)
        QT_LIBS="$QT_LIB"
        if test $QT_IS_STATIC = yes && test $QT_IS_EMBEDDED = no; then
            QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -ljpeg"
        fi
        ;;


    *osf*) 
        # Digital Unix (aka DGUX aka Tru64)
        QT_LIBS="$QT_LIB"
        if test $QT_IS_STATIC = yes ; then
            QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE"
        fi
        ;;

    *solaris*)
        QT_LIBS="$QT_LIB"
        if test $QT_IS_STATIC = yes ; then
            QT_LIBS="$QT_LIBS -L$x_libraries -lXext -lX11 -lm -lSM -lICE -lresolv -lsocket -lnsl"
        fi
        ;;


    *win*)
        # linker flag to suppress console when linking a GUI app on Win32
        QT_GUILINK="/subsystem:windows"

	if test $QT_MAJOR = "3" ; then
	    if test $QT_IS_MT = yes ; then
        	QT_LIBS="/nodefaultlib:libcmt"
            else
            	QT_LIBS="/nodefaultlib:libc"
            fi
        fi

        if test $QT_IS_STATIC = yes ; then
            QT_LIBS="$QT_LIBS $QT_LIB kernel32.lib user32.lib gdi32.lib comdlg32.lib ole32.lib shell32.lib imm32.lib advapi32.lib wsock32.lib winspool.lib winmm.lib netapi32.lib"
            if test $QT_MAJOR = "3" ; then
                QT_LIBS="$QT_LIBS qtmain.lib"
            fi
        else
            QT_LIBS="$QT_LIBS $QT_LIB"        
            if test $QT_MAJOR = "3" ; then
                QT_CXXFLAGS="$QT_CXXFLAGS -DQT_DLL"
                QT_LIBS="$QT_LIBS qtmain.lib qui.lib user32.lib netapi32.lib"
            fi
        fi
        QASSISTANTCLIENT_LDADD="qassistantclient.lib"
        ;;

esac


if test x"$QT_IS_EMBEDDED" = "xyes" ; then
        QT_CXXFLAGS="-DQWS $QT_CXXFLAGS"
fi

if test x"$QT_IS_MT" = "xyes" ; then
        QT_CXXFLAGS="$QT_CXXFLAGS -D_REENTRANT -DQT_THREAD_SUPPORT"
	QT_LIBS="$QT_LIBS"
fi

QT_LDADD="-L$QTDIR/lib $QT_LIBS"

#if test x$QT_IS_STATIC = xyes ; then
#    OLDLIBS="$LIBS"
#    LIBS="$QT_LDADD"
#    AC_CHECK_LIB(Xft, XftFontOpen, QT_LDADD="$QT_LDADD -lXft")
#    LIBS="$LIBS"
#fi

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])
AC_MSG_CHECKING([QT_LDADD])
AC_MSG_RESULT([$QT_LDADD])

AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDADD)
AC_SUBST(QT_GUILINK)
AC_SUBST(QASSISTANTCLIENT_LDADD)

])



dnl @synopsis AC_C_FIND_ENDIAN
dnl
dnl Determine endian-ness of target processor.
dnl @version 1.1	Mar 03 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Majority written from scratch to replace the standard autoconf macro 
dnl AC_C_BIGENDIAN. Only part remaining from the original it the invocation
dnl of the AC_TRY_RUN macro.
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.

dnl Find endian-ness in the following way:
dnl    1) Look in <endian.h>.
dnl    2) If 1) fails, look in <sys/types.h> and <sys/param.h>.
dnl    3) If 1) and 2) fails and not cross compiling run a test program.
dnl    4) If 1) and 2) fails and cross compiling then guess based on target.

AC_DEFUN([AC_C_FIND_ENDIAN],
[AC_CACHE_CHECK(processor byte ordering, 
	ac_cv_c_byte_order,

# Initialize to unknown
ac_cv_c_byte_order=unknown

if test x$ac_cv_header_endian_h = xyes ; then

	# First try <endian.h> which should set BYTE_ORDER.

	[AC_TRY_LINK([
		#include <endian.h>
		#if BYTE_ORDER != LITTLE_ENDIAN
			not big endian
		#endif
		], return 0 ;, 
			ac_cv_c_byte_order=little
		)]
				
	[AC_TRY_LINK([
		#include <endian.h>
		#if BYTE_ORDER != BIG_ENDIAN
			not big endian
		#endif
		], return 0 ;, 
			ac_cv_c_byte_order=big
		)]

	fi

if test $ac_cv_c_byte_order = unknown ; then

	[AC_TRY_LINK([
		#include <sys/types.h>
		#include <sys/param.h>
		#if !BYTE_ORDER || !BIG_ENDIAN || !LITTLE_ENDIAN
			bogus endian macros
		#endif
		], return 0 ;, 

		[AC_TRY_LINK([
			#include <sys/types.h>
			#include <sys/param.h>
			#if BYTE_ORDER != LITTLE_ENDIAN
				not big endian
			#endif
			], return 0 ;, 
				ac_cv_c_byte_order=little
			)]
				
		[AC_TRY_LINK([
			#include <sys/types.h>
			#include <sys/param.h>
			#if BYTE_ORDER != LITTLE_ENDIAN
				not big endian
			#endif
			], return 0 ;, 
				ac_cv_c_byte_order=little
			)]

		)]

 	fi

if test $ac_cv_c_byte_order = unknown ; then
	if test $cross_compiling = yes ; then
		# This is the last resort. Try to guess the target processor endian-ness
		# by looking at the target CPU type.	
		[
		case "$target_cpu" in
			alpha* | i?86* | mipsel* | ia64*)
				ac_cv_c_big_endian=0
				ac_cv_c_little_endian=1
				;;
			
			m68* | mips* | powerpc* | hppa* | sparc*)
				ac_cv_c_big_endian=1
				ac_cv_c_little_endian=0
				;;
	
			esac
		]
	else
		AC_TRY_RUN(
		[[
		int main (void) 
		{	/* Are we little or big endian?  From Harbison&Steele.  */
			union
			{	long l ;
				char c [sizeof (long)] ;
			} u ;
			u.l = 1 ;
			return (u.c [sizeof (long) - 1] == 1);
			}
			]], , ac_cv_c_byte_order=big, 
			ac_cv_c_byte_order=unknown
			)

		AC_TRY_RUN(
		[[int main (void) 
		{	/* Are we little or big endian?  From Harbison&Steele.  */
			union
			{	long l ;
				char c [sizeof (long)] ;
			} u ;
			u.l = 1 ;
			return (u.c [0] == 1);
			}]], , ac_cv_c_byte_order=little, 
			ac_cv_c_byte_order=unknown
			)
		fi	
	fi

)
]

if test $ac_cv_c_byte_order = big ; then
	ac_cv_c_big_endian=1
	ac_cv_c_little_endian=0
elif test $ac_cv_c_byte_order = little ; then
	ac_cv_c_big_endian=0
	ac_cv_c_little_endian=1
else
	ac_cv_c_big_endian=0
	ac_cv_c_little_endian=0

	AC_MSG_WARN([[*****************************************************************]])
	AC_MSG_WARN([[*** Not able to determine endian-ness of target processor.       ]])
	AC_MSG_WARN([[*** The constants CPU_IS_BIG_ENDIAN and CPU_IS_LITTLE_ENDIAN in  ]])
	AC_MSG_WARN([[*** src/config.h may need to be hand editied.                    ]])
	AC_MSG_WARN([[*****************************************************************]])
	fi

)# AC_C_FIND_ENDIAN

dnl @synopsis AC_C99_FUNC_LRINT
dnl
dnl Check whether C99's lrint function is available.
dnl @version 1.3	Feb 12 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LRINT],
[AC_CACHE_CHECK(for lrint,
  ac_cv_c99_lrint,
[
lrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
], if (!lrint(3.14159)) lrint(2.7183);, ac_cv_c99_lrint=yes, ac_cv_c99_lrint=no)

CFLAGS=$lrint_save_CFLAGS

])

if test "$ac_cv_c99_lrint" = yes; then
  AC_DEFINE(HAVE_LRINT, 1,
            [Define if you have C99's lrint function.])
fi
])# AC_C99_FUNC_LRINT
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
CFLAGS="-O2 -lm"
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
dnl @synopsis AC_C99_FUNC_LLRINT
dnl
dnl Check whether C99's llrint function is available.
dnl @version 1.1	Sep 30 2002
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.
dnl
AC_DEFUN([AC_C99_FUNC_LLRINT],
[AC_CACHE_CHECK(for llrint,
  ac_cv_c99_llrint,
[
llrint_save_CFLAGS=$CFLAGS
CFLAGS="-O2 -lm"
AC_TRY_LINK([
#define		_ISOC9X_SOURCE	1
#define 	_ISOC99_SOURCE	1
#define		__USE_ISOC99	1
#define 	__USE_ISOC9X	1

#include <math.h>
#include <stdint.h>
], int64_t	x ; x = llrint(3.14159) ;, ac_cv_c99_llrint=yes, ac_cv_c99_llrint=no)

CFLAGS=$llrint_save_CFLAGS

])

if test "$ac_cv_c99_llrint" = yes; then
  AC_DEFINE(HAVE_LLRINT, 1,
            [Define if you have C99's llrint function.])
fi
])# AC_C99_FUNC_LLRINT



dnl @synopsis AC_C_CLIP_MODE
dnl
dnl Determine the clipping mode when converting float to int.
dnl @version 1.0	May 17 2003
dnl @author Erik de Castro Lopo <erikd AT mega-nerd DOT com>
dnl
dnl Permission to use, copy, modify, distribute, and sell this file for any 
dnl purpose is hereby granted without fee, provided that the above copyright 
dnl and this permission notice appear in all copies.  No representations are
dnl made about the suitability of this software for any purpose.  It is 
dnl provided "as is" without express or implied warranty.



dnl Find the clipping mode in the following way:
dnl    1) If we are not cross compiling test it.
dnl    2) IF we are cross compiling, assume that clipping isn't done correctly.

AC_DEFUN([AC_C_CLIP_MODE],
[AC_CACHE_CHECK(processor clipping capabilities, 
	ac_cv_c_clip_type,

# Initialize to unknown
ac_cv_c_clip_positive=unknown
ac_cv_c_clip_negative=unknown

if test $ac_cv_c_clip_positive = unknown ; then
	AC_TRY_RUN(
	[[
	#define	_ISOC9X_SOURCE	1
	#define _ISOC99_SOURCE	1
	#define	__USE_ISOC99	1
	#define __USE_ISOC9X	1
	#include <math.h>
	int main (void)
	{	double	fval ;
		int k, ival ;

		fval = 1.0 * 0x7FFFFFFF ;
		for (k = 0 ; k < 100 ; k++)
		{	ival = (lrint (fval)) >> 24 ;
			if (ival != 127)
				return 1 ;
		
			fval *= 1.2499999 ;
			} ;
		
			return 0 ;
		}
		]],
		ac_cv_c_clip_positive=yes,
		ac_cv_c_clip_positive=no,
		ac_cv_c_clip_positive=unknown
		)

	AC_TRY_RUN(
	[[
	#define	_ISOC9X_SOURCE	1
	#define _ISOC99_SOURCE	1
	#define	__USE_ISOC99	1
	#define __USE_ISOC9X	1
	#include <math.h>
	int main (void)
	{	double	fval ;
		int k, ival ;

		fval = -8.0 * 0x10000000 ;
		for (k = 0 ; k < 100 ; k++)
		{	ival = (lrint (fval)) >> 24 ;
			if (ival != -128)
				return 1 ;
		
			fval *= 1.2499999 ;
			} ;
		
			return 0 ;
		}
		]],
		ac_cv_c_clip_negative=yes,
		ac_cv_c_clip_negative=no,
		ac_cv_c_clip_negative=unknown
		)

	fi

if test $ac_cv_c_clip_positive = yes ; then
	ac_cv_c_clip_positive=1
else
	ac_cv_c_clip_positive=0
	fi

if test $ac_cv_c_clip_negative = yes ; then
	ac_cv_c_clip_negative=1
else
	ac_cv_c_clip_negative=0
	fi

[[
case "$ac_cv_c_clip_positive$ac_cv_c_clip_negative" in
	"00")
		ac_cv_c_clip_type="none"
		;;
	"10")
		ac_cv_c_clip_type="positive"
		;;
	"01")
		ac_cv_c_clip_type="negative"
		;;
	"11")
		ac_cv_c_clip_type="both"
		;;
	esac
	]]

)
]

)# AC_C_CLIP_MODE



