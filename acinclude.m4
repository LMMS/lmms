# Check for Qt compiler flags, linker flags, and binary packages
AC_DEFUN([gw_CHECK_QT],
[
AC_REQUIRE([AC_PROG_CXX])
AC_REQUIRE([AC_PATH_X])

case "${prefix}" in
	/opt/mingw*)
		;;
	*)
		AC_PATH_PROG([PKGCONFIG], [pkg-config])
		;;
esac

# Only search manually if no pkgconfig
if test -z "$PKGCONFIG" ; then
	AC_MSG_CHECKING([QTDIR])
	AC_ARG_WITH([qtdir], [  --with-qtdir=DIR        Qt installation directory [default=$QTDIR]], QTDIR=$withval)
	# Check that QTDIR is defined or that --with-qtdir given
	if test x"$QTDIR" = x ; then
		# some usual Qt-locations
		QT_SEARCH="/usr /usr/lib/qt4 /usr/share/qt4 /usr/local/Trolltech/Qt-4.3.0 /usr/local/Trolltech/Qt-4.3.1 /usr/local/Trolltech/Qt-4.3.2 /usr/local/Trolltech/Qt-4.3.3 /usr/local/Trolltech/Qt-4.3.4"
	else
		QT_SEARCH=$QTDIR
		QTDIR=""
	fi
	for i in $QT_SEARCH ; do
		QT_INCLUDE_SEARCH="include/qt4 include"
		for j in $QT_INCLUDE_SEARCH ; do
			if test -f $i/$j/Qt/qglobal.h -a x$QTDIR = x ; then
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
fi

# Search for available Qt translations
AH_TEMPLATE(QT_TRANSLATIONS_DIR, [Define to Qt translations directory])
AC_MSG_CHECKING([Qt translations])
QT_TRANSLATIONS_SEARCH="$QTDIR /usr/share/qt4 /usr/local/qt /usr/local/Trolltech/Qt-4.3.0 /usr/local/Trolltech/Qt-4.3.1"
for i in $QT_TRANSLATIONS_SEARCH ; do
    if test -d $i/translations -a x$QT_TRANSLATIONS = x ; then
        QT_TRANSLATIONS=$i/translations
    fi
done
if test x"$QT_TRANSLATIONS" = x ; then
    AC_MSG_ERROR([*** not found! Either install Qt i18n files or define QT_TRANSLATIONS.])
fi
AC_DEFINE_UNQUOTED(QT_TRANSLATIONS_DIR, "$QT_TRANSLATIONS")
AC_MSG_RESULT([$QT_TRANSLATIONS])

# First try to set QTHOSTDIR according to pkg-config
#
if test -n "$PKGCONFIG" ; then
	QTHOSTDIR=$(pkg-config QtCore --variable=prefix)
fi

if test -z "$QTHOSTDIR" ; then
	case "${prefix}" in
		/opt/mingw*)
			QTHOSTDIR=/usr
			;;
		*)
			if test -n "$QTDIR" ; then
				QTHOSTDIR="$QTDIR"
			else
				QTHOSTDIR=/usr
			fi
			;;
	esac
fi


# Check that moc is in path
AC_CHECK_PROG(MOC, moc-qt4, $QTHOSTDIR/bin/moc-qt4,,$QTHOSTDIR/bin/)
if test x$MOC = x ; then
	AC_CHECK_PROG(MOC, moc, $QTHOSTDIR/bin/moc,,$QTHOSTDIR/bin/)
	if test x$MOC = x ; then
        	AC_MSG_ERROR([*** not found! Make sure you have Qt-devel-tools installed!])
	fi
fi

# Check that uic is in path
AC_CHECK_PROG(UIC, uic-qt4, $QTHOSTDIR/bin/uic-qt4,,$QTHOSTDIR/bin/)
if test x$UIC = x ; then
	AC_CHECK_PROG(UIC, uic, $QTHOSTDIR/bin/uic,,$QTHOSTDIR/bin/)
	if test x$UIC = x ; then
        	AC_MSG_ERROR([*** not found! Make sure you have Qt-devel-tools installed!])
	fi
fi

# Check that rcc is in path
AC_CHECK_PROG(RCC, rcc-qt4, $QTHOSTDIR/bin/rcc-qt4,,$QTHOSTDIR/bin/)
if test x$RCC = x ; then
	AC_CHECK_PROG(RCC, rcc, $QTHOSTDIR/bin/rcc,,$QTHOSTDIR/bin/)
	if test x$RCC = x ; then
        	AC_MSG_ERROR([*** not found! Make sure you have Qt-devel-tools installed!])
	fi
fi

# lupdate is the Qt translation-update utility.
AC_CHECK_PROG(LUPDATE, lupdate-qt4, $QTHOSTDIR/bin/lupdate-qt4,,$QTHOSTDIR/bin/)
if test x$LUPDATE = x ; then
	AC_CHECK_PROG(LUPDATE, lupdate, $QTHOSTDIR/bin/lupdate,,$QTHOSTDIR/bin/)
	if test x$LUPDATE = x ; then
	        AC_MSG_WARN([*** not found! It's not needed just for compiling but should be part of a proper Qt-devel-tools-installation!])
	fi
fi

# lrelease is the Qt translation-release utility.
AC_CHECK_PROG(LRELEASE, lrelease-qt4, $QTHOSTDIR/bin/lrelease-qt4,,$QTHOSTDIR/bin/)
if test x$LRELEASE = x ; then
	AC_CHECK_PROG(LRELEASE, lrelease, $QTHOSTDIR/bin/lrelease,,$QTHOSTDIR/bin/)
	if test x$LRELEASE = x ; then
	        AC_MSG_WARN([*** not found! It's not needed just for compiling but should be part of a proper Qt-devel-tools-installation!])
	fi
fi

# construct CXXFLAGS
if test -n "$PKGCONFIG" ; then
	QT_CXXFLAGS=$(pkg-config --cflags QtCore QtGui QtXml)
fi

if test -z "$QT_CXXFLAGS" ; then
	QT_CXXFLAGS="-I$QT_INCLUDES -I$QT_INCLUDES/Qt -D_REENTRANT -DQT_NO_DEBUG -DQT_CORE_LIB -DQT_XML_LIB -DQT_THREAD_SUPPORT"
fi

AC_MSG_CHECKING([QT_CXXFLAGS])
AC_MSG_RESULT([$QT_CXXFLAGS])


# check libraries
AC_MSG_CHECKING([Qt4 libraries])

if test -n "$PKGCONFIG" ; then
	QT_LIB=$(pkg-config --libs QtCore QtGui QtXml)
fi

if test -z "$QT_LIB" ; then
	case "${host}" in
	      *mingw32)
		QT_LIBS=`ls $QTDIR/lib/libQt*.a 2> /dev/null` 
		if test "x$QT_LIBS" = x;  then
		    AC_MSG_ERROR([*** Couldn't find any Qt4 libraries])
		fi
		QT_LIB="-L$QTDIR/bin -lQtCore4 -lQtXml4 -lQtNetwork4 -lQtGui4"
		# Check that windres is in path
		AC_PATH_PROGS([WINDRES],[i586-mingw32-windres windres],,[${prefix}/bin:$PATH])
		if test x$WINDRES = x ; then
			AC_MSG_ERROR([*** not found! Make sure you have mingw32 binutils installed!])
		fi
		;;
	    *)
		QT_LIBS=`ls $QTDIR/lib64/libQt*.so 2> /dev/null` 
		if test "x$QT_LIBS" = x;  then
			QT_LIBS=`ls $QTDIR/lib/libQt*.so 2> /dev/null` 
			if test "x$QT_LIBS" = x;  then
				AC_MSG_ERROR([*** Couldn't find any Qt4 libraries])
			fi
			QT_LIB="-L$QTDIR/lib -L$QTDIR/lib/qt4"
		else
			QT_LIB="-L$QTDIR/lib64 -L$QTDIR/lib64/qt4"
		fi
		QT_LIB="$QT_LIB -lQtCore -lQtXml -lQtNetwork -lQtGui"
		;;
	esac
fi
AC_MSG_RESULT([found: $QT_LIB])

QT_LDADD="$QT_LIB"




AC_SUBST(QT_CXXFLAGS)
AC_SUBST(QT_LDADD)


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


dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ax_prefix_config_h.html
dnl
AC_DEFUN([AX_PREFIX_CONFIG_H],[AC_REQUIRE([AC_CONFIG_HEADER])
AC_CONFIG_COMMANDS([ifelse($1,,$PACKAGE-config.h,$1)],[dnl
AS_VAR_PUSHDEF([_OUT],[ac_prefix_conf_OUT])dnl
AS_VAR_PUSHDEF([_DEF],[ac_prefix_conf_DEF])dnl
AS_VAR_PUSHDEF([_PKG],[ac_prefix_conf_PKG])dnl
AS_VAR_PUSHDEF([_LOW],[ac_prefix_conf_LOW])dnl
AS_VAR_PUSHDEF([_UPP],[ac_prefix_conf_UPP])dnl
AS_VAR_PUSHDEF([_INP],[ac_prefix_conf_INP])dnl
m4_pushdef([_script],[conftest.prefix])dnl
m4_pushdef([_symbol],[m4_cr_Letters[]m4_cr_digits[]_])dnl
_OUT=`echo ifelse($1, , $PACKAGE-config.h, $1)`
_DEF=`echo _$_OUT | sed -e "y:m4_cr_letters:m4_cr_LETTERS[]:" -e "s/@<:@^m4_cr_Letters@:>@/_/g"`
_PKG=`echo ifelse($2, , LMMS, $2)`
_LOW=`echo _$_PKG | sed -e "y:m4_cr_LETTERS-:m4_cr_letters[]_:"`
_UPP=`echo $_PKG | sed -e "y:m4_cr_letters-:m4_cr_LETTERS[]_:"  -e "/^@<:@m4_cr_digits@:>@/s/^/_/"`
_INP=`echo ifelse($3, , _, $3)`
if test "$ac_prefix_conf_INP" = "_"; then
   for ac_file in : $CONFIG_HEADERS; do test "_$ac_file" = _: && continue
     test -f "$ac_prefix_conf_INP" && continue
     case $ac_file in
        *.h) test -f $ac_file && _INP=$ac_file ;;
        *)
     esac
   done
fi
if test "$_INP" = "_"; then
   case "$_OUT" in
      */*) _INP=`basename "$_OUT"`
      ;;
      *-*) _INP=`echo "$_OUT" | sed -e "s/@<:@_symbol@:>@*-//"`
      ;;
      *) _INP=config.h
      ;;
   esac
fi
if test -z "$_PKG" ; then
   AC_MSG_ERROR([no prefix for _PREFIX_PKG_CONFIG_H])
else
  if test ! -f "$_INP" ; then if test -f "$srcdir/$_INP" ; then
     _INP="$srcdir/$_INP"
  fi fi
  AC_MSG_NOTICE(creating $_OUT - prefix $_UPP for $_INP defines)
  if test -f $_INP ; then
    echo "s/@%:@undef  *\\(@<:@m4_cr_LETTERS[]_@:>@\\)/@%:@undef $_UPP""_\\1/" > _script
    # no! these are things like socklen_t, const, vfork
    # echo "s/@%:@undef  *\\(@<:@m4_cr_letters@:>@\\)/@%:@undef $_LOW""_\\1/" >> _script
    echo "s/@%:@def[]ine  *\\(@<:@m4_cr_LETTERS[]_@:>@@<:@_symbol@:>@*\\)\\(.*\\)/@%:@ifndef $_UPP""_\\1 \\" >> _script
    echo "@%:@def[]ine $_UPP""_\\1 \\2 \\" >> _script
    echo "@%:@endif/" >>_script
    # no! these are things like socklen_t, const, vfork
    # echo "s/@%:@def[]ine  *\\(@<:@m4_cr_letters@:>@@<:@_symbol@:>@*\\)\\(.*\\)/@%:@ifndef $_LOW""_\\1 \\" >> _script
    # echo "@%:@define $_LOW""_\\1 \\2 \\" >> _script
    # echo "@%:@endif/" >> _script
    # now executing _script on _DEF input to create _OUT output file
    echo "@%:@ifndef $_DEF"      >$tmp/pconfig.h
    echo "@%:@def[]ine $_DEF 1" >>$tmp/pconfig.h
    echo ' ' >>$tmp/pconfig.h
    echo /'*' $_OUT. Generated automatically at end of configure. '*'/ >>$tmp/pconfig.h

    sed -f _script $_INP >>$tmp/pconfig.h
    echo ' ' >>$tmp/pconfig.h
    echo '/* once:' $_DEF '*/' >>$tmp/pconfig.h
    echo "@%:@endif" >>$tmp/pconfig.h
    if cmp -s $_OUT $tmp/pconfig.h 2>/dev/null; then
      AC_MSG_NOTICE([$_OUT is unchanged])
    else
      ac_dir=`AS_DIRNAME(["$_OUT"])`
      AS_MKDIR_P(["$ac_dir"])
      rm -f "$_OUT"
      mv $tmp/pconfig.h "$_OUT"
    fi
#    cp _script _configs.sed
  else
    AC_MSG_ERROR([input file $_INP does not exist - skip generating $_OUT])
  fi
  rm -f conftest.*
fi
m4_popdef([_symbol])dnl
m4_popdef([_script])dnl
AS_VAR_POPDEF([_INP])dnl
AS_VAR_POPDEF([_UPP])dnl
AS_VAR_POPDEF([_LOW])dnl
AS_VAR_POPDEF([_PKG])dnl
AS_VAR_POPDEF([_DEF])dnl
AS_VAR_POPDEF([_OUT])dnl
],[PACKAGE="$PACKAGE"])])


