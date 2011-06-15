/* "$Id: fl_utf8.h 8585 2011-04-13 15:43:22Z ianmacarthur $"
 *
 * Author: Jean-Marc Lienher ( http://oksid.ch )
 * Copyright 2000-2010 by O'ksi'D.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

/* Merged in some functionality from the fltk-2 version. IMM.
 * The following code is an attempt to merge the functions incorporated in FLTK2
 * with the functions provided in OksiD's fltk-1.1.6-utf8 port
 */

/**
  \file fl_utf8.h
  \brief header for Unicode and UTF8 chracter handling
*/

#ifndef _HAVE_FL_UTF8_HDR_
#define _HAVE_FL_UTF8_HDR_

#include "Fl_Export.H"
#include "fl_types.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <locale.h>
#  include <ctype.h>
#  define xchar wchar_t
#  if !defined(FL_DLL) && !defined(__CYGWIN__)
#    undef strdup
#    define strdup _strdup
#    undef putenv
#    define putenv _putenv
#    undef stricmp
#    define stricmp _stricmp
#    undef strnicmp
#    define strnicmp _strnicmp
#    undef hypot
#    define hypot _hypot
#    undef chdir
#    define chdir _chdir
#  endif
#elif defined(__APPLE__)
#  include <wchar.h>
#  include <sys/stat.h>
#  define xchar wchar_t
#else /* X11 */
#  include <sys/types.h>
#  include <sys/stat.h>
#  include "Xutf8.h"
#  include <X11/Xlocale.h>
#  include <X11/Xlib.h>
#  include <locale.h>
#  define xchar unsigned short
#endif

#  ifdef __cplusplus
extern "C" {
#  endif

/** \addtogroup fl_unicode
    @{
*/

/* F2: comes from FLTK2 */
/* OD: comes from OksiD */

/**
  Return the number of bytes needed to encode the given UCS4 character in UTF8.
  \param [in] ucs UCS4 encoded character
  \return number of bytes required
 */
FL_EXPORT int fl_utf8bytes(unsigned ucs);

/* OD: returns the byte length of the first UTF-8 char sequence (returns -1 if not valid) */
FL_EXPORT int fl_utf8len(char c);

/* OD: returns the byte length of the first UTF-8 char sequence (returns +1 if not valid) */
FL_EXPORT int fl_utf8len1(char c);

/* OD: returns the number of Unicode chars in the UTF-8 string */
FL_EXPORT int fl_utf_nb_char(const unsigned char *buf, int len);

/* F2: Convert the next UTF8 char-sequence into a Unicode value (and say how many bytes were used) */
FL_EXPORT unsigned fl_utf8decode(const char* p, const char* end, int* len);

/* F2: Encode a Unicode value into a UTF8 sequence, return the number of bytes used */
FL_EXPORT int fl_utf8encode(unsigned ucs, char* buf);

/* F2: Move forward to the next valid UTF8 sequence start betwen start and end */
FL_EXPORT const char* fl_utf8fwd(const char* p, const char* start, const char* end);

/* F2: Move backward to the previous valid UTF8 sequence start */
FL_EXPORT const char* fl_utf8back(const char* p, const char* start, const char* end);

/* XX: Convert a single 32-bit Unicode value into UTF16 */
FL_EXPORT unsigned fl_ucs_to_Utf16(const unsigned ucs, unsigned short *dst, const unsigned dstlen);

/* F2: Convert a UTF8 string into UTF16 */
FL_EXPORT unsigned fl_utf8toUtf16(const char* src, unsigned srclen, unsigned short* dst, unsigned dstlen);

/* F2: Convert a UTF8 string into a wide character string - makes UTF16 on win32, "UCS4" elsewhere */
FL_EXPORT unsigned fl_utf8towc(const char *src, unsigned srclen, wchar_t *dst, unsigned dstlen);

/* F2: Convert a wide character string to UTF8 - takes in UTF16 on win32, "UCS4" elsewhere */
FL_EXPORT unsigned fl_utf8fromwc(char *dst, unsigned dstlen, const wchar_t *src, unsigned srclen);

/* F2: Convert a UTF8 string into ASCII, eliding untranslatable glyphs */
FL_EXPORT unsigned fl_utf8toa (const char *src, unsigned srclen, char *dst, unsigned dstlen);
/* OD: convert UTF-8 string to latin1 */
/* FL_EXPORT int fl_utf2latin1(const unsigned char *src, int srclen, char *dst); */

/* F2: Convert 8859-1 string to UTF8 */
FL_EXPORT unsigned fl_utf8froma (char *dst, unsigned dstlen, const char *src, unsigned srclen);
/* OD: convert latin1 str to UTF-8 */
/* FL_EXPORT int fl_latin12utf(const unsigned char *src, int srclen, char *dst); */

/* F2: Returns true if the current O/S locale is UTF8 */
FL_EXPORT int fl_utf8locale();

/* F2: Examine the first len characters of src, to determine if the input text is UTF8 or not
 * NOTE: The value returned is not simply boolean - it contains information about the probable
 * type of the src text. */
FL_EXPORT int fl_utf8test(const char *src, unsigned len);

/* XX: return width of "raw" ucs character in columns.
 * for internal use only */
FL_EXPORT int fl_wcwidth_(unsigned int ucs);

/* XX: return width of utf-8 character string in columns.
 * NOTE: this may also do C1 control character (0x80 to 0x9f) to CP1252 mapping,
 * depending on original build options */
FL_EXPORT int fl_wcwidth(const char *src);

/* OD: Return true if the character is non-spacing */
FL_EXPORT unsigned int fl_nonspacing(unsigned int ucs);

/* F2: Convert UTF8 to a local multi-byte encoding - mainly for win32? */
FL_EXPORT unsigned fl_utf8to_mb(const char *src, unsigned srclen, char *dst, unsigned dstlen);
/* OD: Convert UTF8 to a local multi-byte encoding */
FL_EXPORT char* fl_utf2mbcs(const char *src);

/* F2: Convert a local multi-byte encoding to UTF8 - mainly for win32? */
FL_EXPORT unsigned fl_utf8from_mb(char *dst, unsigned dstlen, const char *src, unsigned srclen);
/* OD: Convert a local multi-byte encoding to UTF8 */
/* FL_EXPORT char* fl_mbcs2utf(const char *src); */

/*****************************************************************************/
#ifdef WIN32
/* OD: Attempt to convert the UTF8 string to the current locale */
FL_EXPORT char *fl_utf8_to_locale(const char *s, int len, unsigned int codepage);

/* OD: Attempt to convert a string in the current locale to UTF8 */
FL_EXPORT char *fl_locale_to_utf8(const char *s, int len, unsigned int codepage);
#endif

/*****************************************************************************
 * The following functions are intended to provide portable, UTF8 aware
 * versions of standard functions
 */

/* OD: UTF8 aware strncasecmp - converts to lower case Unicode and tests */
FL_EXPORT int fl_utf_strncasecmp(const char *s1, const char *s2, int n);

/* OD: UTF8 aware strcasecmp - converts to Unicode and tests */
FL_EXPORT int fl_utf_strcasecmp(const char *s1, const char *s2);

/* OD: return the Unicode lower case value of ucs */
FL_EXPORT int fl_tolower(unsigned int ucs);

/* OD: return the Unicode upper case value of ucs */
FL_EXPORT int fl_toupper(unsigned int ucs);

/* OD: converts the UTF8 string to the lower case equivalent */
FL_EXPORT int fl_utf_tolower(const unsigned char *str, int len, char *buf);

/* OD: converts the UTF8 string to the upper case equivalent */
FL_EXPORT int fl_utf_toupper(const unsigned char *str, int len, char *buf);

/* OD: Portable UTF8 aware chmod wrapper */
FL_EXPORT int fl_chmod(const char* f, int mode);

/* OD: Portable UTF8 aware access wrapper */
FL_EXPORT int fl_access(const char* f, int mode);

/* OD: Portable UTF8 aware stat wrapper */
FL_EXPORT int fl_stat( const char *path, struct stat *buffer );

/* OD: Portable UTF8 aware getcwd wrapper */
FL_EXPORT char* fl_getcwd( char *buf, int maxlen);

/* OD: Portable UTF8 aware fopen wrapper */
FL_EXPORT FILE *fl_fopen(const char *f, const char *mode);

/* OD: Portable UTF8 aware system wrapper */
FL_EXPORT int fl_system(const char* f);

/* OD: Portable UTF8 aware execvp wrapper */
FL_EXPORT int fl_execvp(const char *file, char *const *argv);

/* OD: Portable UTF8 aware open wrapper */
FL_EXPORT int fl_open(const char* f, int o, ...);

/* OD: Portable UTF8 aware unlink wrapper */
FL_EXPORT int fl_unlink(const char *f);

/* OD: Portable UTF8 aware rmdir wrapper */
FL_EXPORT int fl_rmdir(const char *f);

/* OD: Portable UTF8 aware getenv wrapper */
FL_EXPORT char* fl_getenv(const char *name);

/* OD: Portable UTF8 aware execvp wrapper */
FL_EXPORT int fl_mkdir(const char* f, int mode);

/* OD: Portable UTF8 aware rename wrapper */
FL_EXPORT int fl_rename(const char* f, const char *t);


/* OD: Given a full pathname, this will create the directory path needed to hold the file named */
FL_EXPORT void fl_make_path_for_file( const char *path );

/* OD: recursively create a path in the file system */
FL_EXPORT char fl_make_path( const char *path );


/** @} */

/*****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _HAVE_FL_UTF8_HDR_ */

/*
 * End of "$Id: fl_utf8.h 8585 2011-04-13 15:43:22Z ianmacarthur $".
 */
