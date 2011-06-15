/*
 * "$Id: fl_utf.c 8585 2011-04-13 15:43:22Z ianmacarthur $"
 *
 * This is the utf.c file from fltk2 adapted for use in my fltk1.1 port
 */
/* Copyright 2006-2011 by Bill Spitzak and others.
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

/* Modified to obey rfc3629, which limits unicode to 0-0x10ffff */

#include <FL/fl_utf8.h>
#include <string.h>
#include <stdlib.h>

/** \addtogroup fl_unicode
    @{
*/


#if 0
  /**
   \defgroup fl_unichar Unicode Character Functions
   Global Functions Handling Single Unicode Characters
   @{ */

  /**
   Converts a Unicode character into a utf-8 sequence.
   \param[in] uc Unicode character
   \param[out] text utf-8 sequence will be written here; if this pointer is
   \c NULL, only the length of the utf-8 sequence is calculated
   \return length of the sequence in bytes
   */
  /* FL_EXPORT int fl_unichar_to_utf8(unsigned int uc, char *text); */

  /** @} */

  /**
   \defgroup fl_utf8 Unicode String Functions
   Global Functions Handling Unicode Text
   @{ */

  /**
   Calculate the size of a utf-8 sequence for a Unicode character.
   \param[in] uc Unicode character
   \return length of the sequence in bytes
   */
  /* FL_EXPORT int fl_utf8_size(unsigned int uc); */

  /** @} */
#endif /* 0 */

/*!Set to 1 to turn bad UTF8 bytes into ISO-8859-1. If this is to zero
   they are instead turned into the Unicode REPLACEMENT CHARACTER, of
   value 0xfffd.
   If this is on fl_utf8decode() will correctly map most (perhaps all)
   human-readable text that is in ISO-8859-1. This may allow you
   to completely ignore character sets in your code because virtually
   everything is either ISO-8859-1 or UTF-8.
*/
#define ERRORS_TO_ISO8859_1 1

/*!Set to 1 to turn bad UTF8 bytes in the 0x80-0x9f range into the
   Unicode index for Microsoft's CP1252 character set. You should
   also set ERRORS_TO_ISO8859_1. With this a huge amount of more
   available text (such as all web pages) are correctly converted
   to Unicode.
*/
#define ERRORS_TO_CP1252 1

/*!A number of Unicode code points are in fact illegal and should not
   be produced by a UTF-8 converter. Turn this on will replace the
   bytes in those encodings with errors. If you do this then converting
   arbitrary 16-bit data to UTF-8 and then back is not an identity,
   which will probably break a lot of software.
*/
#define STRICT_RFC3629 0

#if ERRORS_TO_CP1252
/* Codes 0x80..0x9f from the Microsoft CP1252 character set, translated
 * to Unicode:
 */
static unsigned short cp1252[32] = {
  0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
  0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
  0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
  0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
};
#endif

/*! Decode a single UTF-8 encoded character starting at \e p. The
    resulting Unicode value (in the range 0-0x10ffff) is returned,
    and \e len is set to the number of bytes in the UTF-8 encoding
    (adding \e len to \e p will point at the next character).

    If \p p points at an illegal UTF-8 encoding, including one that
    would go past \e end, or where a code is uses more bytes than
    necessary, then *(unsigned char*)p is translated as though it is
    in the Microsoft CP1252 character set and \e len is set to 1.
    Treating errors this way allows this to decode almost any
    ISO-8859-1 or CP1252 text that has been mistakenly placed where
    UTF-8 is expected, and has proven very useful.

    If you want errors to be converted to error characters (as the
    standards recommend), adding a test to see if the length is
    unexpectedly 1 will work:

    \code
    if (*p & 0x80) {              // what should be a multibyte encoding
      code = fl_utf8decode(p,end,&len);
      if (len<2) code = 0xFFFD;   // Turn errors into REPLACEMENT CHARACTER
    } else {                      // handle the 1-byte utf8 encoding:
      code = *p;
      len = 1;
    }
    \endcode

    Direct testing for the 1-byte case (as shown above) will also
    speed up the scanning of strings where the majority of characters
    are ASCII.
*/
unsigned fl_utf8decode(const char* p, const char* end, int* len)
{
  unsigned char c = *(unsigned char*)p;
  if (c < 0x80) {
    if (len) *len = 1;
    return c;
#if ERRORS_TO_CP1252
  } else if (c < 0xa0) {
    if (len) *len = 1;
    return cp1252[c-0x80];
#endif
  } else if (c < 0xc2) {
    goto FAIL;
  }
  if ( (end && p+1 >= end) || (p[1]&0xc0) != 0x80) goto FAIL;
  if (c < 0xe0) {
    if (len) *len = 2;
    return
      ((p[0] & 0x1f) << 6) +
      ((p[1] & 0x3f));
  } else if (c == 0xe0) {
    if (((unsigned char*)p)[1] < 0xa0) goto FAIL;
    goto UTF8_3;
#if STRICT_RFC3629
  } else if (c == 0xed) {
    /* RFC 3629 says surrogate chars are illegal. */
    if (((unsigned char*)p)[1] >= 0xa0) goto FAIL;
    goto UTF8_3;
  } else if (c == 0xef) {
    /* 0xfffe and 0xffff are also illegal characters */
    if (((unsigned char*)p)[1]==0xbf &&
	((unsigned char*)p)[2]>=0xbe) goto FAIL;
    goto UTF8_3;
#endif
  } else if (c < 0xf0) {
  UTF8_3:
    if ( (end && p+2 >= end) || (p[2]&0xc0) != 0x80) goto FAIL;
    if (len) *len = 3;
    return
      ((p[0] & 0x0f) << 12) +
      ((p[1] & 0x3f) << 6) +
      ((p[2] & 0x3f));
  } else if (c == 0xf0) {
    if (((unsigned char*)p)[1] < 0x90) goto FAIL;
    goto UTF8_4;
  } else if (c < 0xf4) {
  UTF8_4:
    if ( (end && p+3 >= end) || (p[2]&0xc0) != 0x80 || (p[3]&0xc0) != 0x80) goto FAIL;
    if (len) *len = 4;
#if STRICT_RFC3629
    /* RFC 3629 says all codes ending in fffe or ffff are illegal: */
    if ((p[1]&0xf)==0xf &&
	((unsigned char*)p)[2] == 0xbf &&
	((unsigned char*)p)[3] >= 0xbe) goto FAIL;
#endif
    return
      ((p[0] & 0x07) << 18) +
      ((p[1] & 0x3f) << 12) +
      ((p[2] & 0x3f) << 6) +
      ((p[3] & 0x3f));
  } else if (c == 0xf4) {
    if (((unsigned char*)p)[1] > 0x8f) goto FAIL; /* after 0x10ffff */
    goto UTF8_4;
  } else {
  FAIL:
    if (len) *len = 1;
#if ERRORS_TO_ISO8859_1
    return c;
#else
    return 0xfffd; /* Unicode REPLACEMENT CHARACTER */
#endif
  }
}

/*! Move \p p forward until it points to the start of a UTF-8
  character. If it already points at the start of one then it
  is returned unchanged. Any UTF-8 errors are treated as though each
  byte of the error is an individual character.

  \e start is the start of the string and is used to limit the
  backwards search for the start of a utf8 character.

  \e end is the end of the string and is assumed to be a break
  between characters. It is assumed to be greater than p.

  This function is for moving a pointer that was jumped to the
  middle of a string, such as when doing a binary search for
  a position. You should use either this or fl_utf8back() depending
  on which direction your algorithim can handle the pointer
  moving. Do not use this to scan strings, use fl_utf8decode()
  instead.
*/
const char* fl_utf8fwd(const char* p, const char* start, const char* end)
{
  const char* a;
  int len;
  /* if we are not pointing at a continuation character, we are done: */
  if ((*p&0xc0) != 0x80) return p;
  /* search backwards for a 0xc0 starting the character: */
  for (a = p-1; ; --a) {
    if (a < start) return p;
    if (!(a[0]&0x80)) return p;
    if ((a[0]&0x40)) break;
  }
  fl_utf8decode(a,end,&len);
  a += len;
  if (a > p) return a;
  return p;
}

/*! Move \p p backward until it points to the start of a UTF-8
  character. If it already points at the start of one then it
  is returned unchanged. Any UTF-8 errors are treated as though each
  byte of the error is an individual character.

  \e start is the start of the string and is used to limit the
  backwards search for the start of a UTF-8 character.

  \e end is the end of the string and is assumed to be a break
  between characters. It is assumed to be greater than p.

  If you wish to decrement a UTF-8 pointer, pass p-1 to this.
*/
const char* fl_utf8back(const char* p, const char* start, const char* end)
{
  const char* a;
  int len;
  /* if we are not pointing at a continuation character, we are done: */
  if ((*p&0xc0) != 0x80) return p;
  /* search backwards for a 0xc0 starting the character: */
  for (a = p-1; ; --a) {
    if (a < start) return p;
    if (!(a[0]&0x80)) return p;
    if ((a[0]&0x40)) break;
  }
  fl_utf8decode(a,end,&len);
  if (a+len > p) return a;
  return p;
}

/*! Returns number of bytes that utf8encode() will use to encode the
  character \p ucs. */
int fl_utf8bytes(unsigned ucs) {
  if (ucs < 0x000080U) {
    return 1;
  } else if (ucs < 0x000800U) {
    return 2;
  } else if (ucs < 0x010000U) {
    return 3;
  } else if (ucs <= 0x10ffffU) {
    return 4;
  } else {
    return 3; /* length of the illegal character encoding */
  }
}

/*! Write the UTF-8 encoding of \e ucs into \e buf and return the
    number of bytes written. Up to 4 bytes may be written. If you know
    that \p ucs is less than 0x10000 then at most 3 bytes will be written.
    If you wish to speed this up, remember that anything less than 0x80
    is written as a single byte.

    If ucs is greater than 0x10ffff this is an illegal character
    according to RFC 3629. These are converted as though they are
    0xFFFD (REPLACEMENT CHARACTER).

    RFC 3629 also says many other values for \p ucs are illegal (in
    the range 0xd800 to 0xdfff, or ending with 0xfffe or
    0xffff). However I encode these as though they are legal, so that
    utf8encode/fl_utf8decode will be the identity for all codes between 0
    and 0x10ffff.
*/
int fl_utf8encode(unsigned ucs, char* buf) {
  if (ucs < 0x000080U) {
    buf[0] = ucs;
    return 1;
  } else if (ucs < 0x000800U) {
    buf[0] = 0xc0 | (ucs >> 6);
    buf[1] = 0x80 | (ucs & 0x3F);
    return 2;
  } else if (ucs < 0x010000U) {
    buf[0] = 0xe0 | (ucs >> 12);
    buf[1] = 0x80 | ((ucs >> 6) & 0x3F);
    buf[2] = 0x80 | (ucs & 0x3F);
    return 3;
  } else if (ucs <= 0x0010ffffU) {
    buf[0] = 0xf0 | (ucs >> 18);
    buf[1] = 0x80 | ((ucs >> 12) & 0x3F);
    buf[2] = 0x80 | ((ucs >> 6) & 0x3F);
    buf[3] = 0x80 | (ucs & 0x3F);
    return 4;
  } else {
    /* encode 0xfffd: */
    buf[0] = 0xefU;
    buf[1] = 0xbfU;
    buf[2] = 0xbdU;
    return 3;
  }
}

/*! Convert a single 32-bit Unicode codepoint into an array of 16-bit
    characters. These are used by some system calls, especially on Windows.

    \p ucs is the value to convert.

    \p dst points at an array to write, and \p dstlen is the number of
    locations in this array. At most \p dstlen words will be
    written, and a 0 terminating word will be added if \p dstlen is
    large enough. Thus this function will never overwrite the buffer
    and will attempt return a zero-terminated string if space permits.
    If \p dstlen is zero then \p dst can be set to NULL and no data
    is written, but the length is returned.

    The return value is the number of 16-bit words that \e would be written
    to \p dst if it is large enough, not counting any terminating
    zero.

    If the return value is greater than \p dstlen it indicates truncation,
    you should then allocate a new array of size return+1 and call this again.

    Unicode characters in the range 0x10000 to 0x10ffff are converted to
    "surrogate pairs" which take two words each (in UTF-16 encoding).
    Typically, setting \p dstlen to 2 will ensure that any valid Unicode
    value can be converted, and setting \p dstlen to 3 or more will allow
    a NULL terminated sequence to be returned.
*/
unsigned fl_ucs_to_Utf16(const unsigned ucs, unsigned short *dst, const unsigned dstlen)
{
  /* The rule for direct conversion from UCS to UTF16 is:
   * - if UCS >  0x0010FFFF then UCS is invalid
   * - if UCS >= 0xD800 && UCS <= 0xDFFF UCS is invalid
   * - if UCS <= 0x0000FFFF then U16 = UCS, len = 1
   * - else
   * -- U16[0] = ((UCS - 0x00010000) >> 10) & 0x3FF + 0xD800
   * -- U16[1] = (UCS & 0x3FF) + 0xDC00
   * -- len = 2;
   */
  unsigned count;        /* Count of converted UTF16 cells */
  unsigned short u16[4]; /* Alternate buffer if dst is not set */
  unsigned short *out;   /* points to the active buffer */
  /* Ensure we have a valid buffer to write to */
  if((!dstlen) || (!dst)) {
    out = u16;
  } else {
    out = dst;
  }
  /* Convert from UCS to UTF16 */
  if((ucs > 0x0010FFFF) || /* UCS is too large */
  ((ucs > 0xD7FF) && (ucs < 0xE000))) { /* UCS in invalid range */
    out[0] = 0xFFFD; /* REPLACEMENT CHARACTER */
    count = 1;
  } else if(ucs < 0x00010000) {
    out[0] = (unsigned short)ucs;
    count = 1;
  } else if(dstlen < 2) { /* dst is too small for the result */
    out[0] = 0xFFFD; /* REPLACEMENT CHARACTER */
    count = 2;
  } else {
    out[0] = (((ucs - 0x00010000) >> 10) & 0x3FF) + 0xD800;
    out[1] = (ucs & 0x3FF) + 0xDC00;
    count = 2;
  }
  /* NULL terminate the output, if there is space */
  if(count < dstlen) { out[count] = 0; }
  return count;
} /* fl_ucs_to_Utf16 */

/*! Convert a UTF-8 sequence into an array of 16-bit characters. These
    are used by some system calls, especially on Windows.

    \p src points at the UTF-8, and \p srclen is the number of bytes to
    convert.

    \p dst points at an array to write, and \p dstlen is the number of
    locations in this array. At most \p dstlen-1 words will be
    written there, plus a 0 terminating word. Thus this function
    will never overwrite the buffer and will always return a
    zero-terminated string. If \p dstlen is zero then \p dst can be
    null and no data is written, but the length is returned.

    The return value is the number of 16-bit words that \e would be written
    to \p dst if it were long enough, not counting the terminating
    zero. If the return value is greater or equal to \p dstlen it
    indicates truncation, you can then allocate a new array of size
    return+1 and call this again.

    Errors in the UTF-8 are converted as though each byte in the
    erroneous string is in the Microsoft CP1252 encoding. This allows
    ISO-8859-1 text mistakenly identified as UTF-8 to be printed
    correctly.

    Unicode characters in the range 0x10000 to 0x10ffff are converted to
    "surrogate pairs" which take two words each (this is called UTF-16
    encoding).
*/
unsigned fl_utf8toUtf16(const char* src, unsigned srclen,
		  unsigned short* dst, unsigned dstlen)
{
  const char* p = src;
  const char* e = src+srclen;
  unsigned count = 0;
  if (dstlen) for (;;) {
    if (p >= e) {dst[count] = 0; return count;}
    if (!(*p & 0x80)) { /* ascii */
      dst[count] = *p++;
    } else {
      int len; unsigned ucs = fl_utf8decode(p,e,&len);
      p += len;
      if (ucs < 0x10000) {
	dst[count] = ucs;
      } else {
	/* make a surrogate pair: */
	if (count+2 >= dstlen) {dst[count] = 0; count += 2; break;}
	dst[count] = (((ucs-0x10000u)>>10)&0x3ff) | 0xd800;
	dst[++count] = (ucs&0x3ff) | 0xdc00;
      }
    }
    if (++count == dstlen) {dst[count-1] = 0; break;}
  }
  /* we filled dst, measure the rest: */
  while (p < e) {
    if (!(*p & 0x80)) p++;
    else {
      int len; unsigned ucs = fl_utf8decode(p,e,&len);
      p += len;
      if (ucs >= 0x10000) ++count;
    }
    ++count;
  }
  return count;
}


/**
  Converts a UTF-8 string into a wide character string.

  This function generates 32-bit wchar_t (e.g. "ucs4" as it were) except
  on Windows where it is equivalent to fl_utf8toUtf16 and returns
  UTF-16.

  \p src points at the UTF-8, and \p srclen is the number of bytes to
  convert.

  \p dst points at an array to write, and \p dstlen is the number of
  locations in this array. At most \p dstlen-1 wchar_t will be
  written there, plus a 0 terminating wchar_t.

  The return value is the number of wchar_t that \e would be written
  to \p dst if it were long enough, not counting the terminating
  zero. If the return value is greater or equal to \p dstlen it
  indicates truncation, you can then allocate a new array of size
  return+1 and call this again.

  Notice that sizeof(wchar_t) is 2 on Windows and is 4 on Linux
  and most other systems. Where wchar_t is 16 bits, Unicode
  characters in the range 0x10000 to 0x10ffff are converted to
  "surrogate pairs" which take two words each (this is called UTF-16
  encoding). If wchar_t is 32 bits this rather nasty problem is
  avoided.

  Note that Windows includes Cygwin, i.e. compiled with Cygwin's POSIX
  layer (cygwin1.dll, --enable-cygwin), either native (GDI) or X11.
  */
unsigned fl_utf8towc(const char* src, unsigned srclen,
		  wchar_t* dst, unsigned dstlen)
{
#if defined(WIN32) || defined(__CYGWIN__)
  return fl_utf8toUtf16(src, srclen, (unsigned short*)dst, dstlen);
#else
  const char* p = src;
  const char* e = src+srclen;
  unsigned count = 0;
  if (dstlen) for (;;) {
    if (p >= e) {
      dst[count] = 0;
      return count;
    }
    if (!(*p & 0x80)) { /* ascii */
      dst[count] = *p++;
    } else {
      int len; unsigned ucs = fl_utf8decode(p,e,&len);
      p += len;
      dst[count] = (wchar_t)ucs;
    }
    if (++count == dstlen) {dst[count-1] = 0; break;}
  }
  /* we filled dst, measure the rest: */
  while (p < e) {
    if (!(*p & 0x80)) p++;
    else {
      int len; fl_utf8decode(p,e,&len);
      p += len;
    }
    ++count;
  }
  return count;
#endif
}

/*! Convert a UTF-8 sequence into an array of 1-byte characters.

    If the UTF-8 decodes to a character greater than 0xff then it is
    replaced with '?'.

    Errors in the UTF-8 are converted as individual bytes, same as
    fl_utf8decode() does. This allows ISO-8859-1 text mistakenly identified
    as UTF-8 to be printed correctly (and possibly CP1512 on Windows).

    \p src points at the UTF-8, and \p srclen is the number of bytes to
    convert.

    Up to \p dstlen bytes are written to \p dst, including a null
    terminator. The return value is the number of bytes that would be
    written, not counting the null terminator. If greater or equal to
    \p dstlen then if you malloc a new array of size n+1 you will have
    the space needed for the entire string. If \p dstlen is zero then
    nothing is written and this call just measures the storage space
    needed.
*/
unsigned fl_utf8toa(const char* src, unsigned srclen,
		 char* dst, unsigned dstlen)
{
  const char* p = src;
  const char* e = src+srclen;
  unsigned count = 0;
  if (dstlen) for (;;) {
    unsigned char c;
    if (p >= e) {dst[count] = 0; return count;}
    c = *(unsigned char*)p;
    if (c < 0xC2) { /* ascii or bad code */
      dst[count] = c;
      p++;
    } else {
      int len; unsigned ucs = fl_utf8decode(p,e,&len);
      p += len;
      if (ucs < 0x100) dst[count] = ucs;
      else dst[count] = '?';
    }
    if (++count >= dstlen) {dst[count-1] = 0; break;}
  }
  /* we filled dst, measure the rest: */
  while (p < e) {
    if (!(*p & 0x80)) p++;
    else {
      int len;
      fl_utf8decode(p,e,&len);
      p += len;
    }
    ++count;
  }
  return count;
}

/*! Turn "wide characters" as returned by some system calls
    (especially on Windows) into UTF-8.

    Up to \p dstlen bytes are written to \p dst, including a null
    terminator. The return value is the number of bytes that would be
    written, not counting the null terminator. If greater or equal to
    \p dstlen then if you malloc a new array of size n+1 you will have
    the space needed for the entire string. If \p dstlen is zero then
    nothing is written and this call just measures the storage space
    needed.

    \p srclen is the number of words in \p src to convert. On Windows
    this is not necessarily the number of characters, due to there
    possibly being "surrogate pairs" in the UTF-16 encoding used.
    On Unix wchar_t is 32 bits and each location is a character.

    On Unix if a \p src word is greater than 0x10ffff then this is an
    illegal character according to RFC 3629. These are converted as
    though they are 0xFFFD (REPLACEMENT CHARACTER). Characters in the
    range 0xd800 to 0xdfff, or ending with 0xfffe or 0xffff are also
    illegal according to RFC 3629. However I encode these as though
    they are legal, so that fl_utf8towc will return the original data.

    On Windows "surrogate pairs" are converted to a single character
    and UTF-8 encoded (as 4 bytes). Mismatched halves of surrogate
    pairs are converted as though they are individual characters.
*/
unsigned fl_utf8fromwc(char* dst, unsigned dstlen,
		    const wchar_t* src, unsigned srclen) {
  unsigned i = 0;
  unsigned count = 0;
  if (dstlen) for (;;) {
    unsigned ucs;
    if (i >= srclen) {dst[count] = 0; return count;}
    ucs = src[i++];
    if (ucs < 0x80U) {
      dst[count++] = ucs;
      if (count >= dstlen) {dst[count-1] = 0; break;}
    } else if (ucs < 0x800U) { /* 2 bytes */
      if (count+2 >= dstlen) {dst[count] = 0; count += 2; break;}
      dst[count++] = 0xc0 | (ucs >> 6);
      dst[count++] = 0x80 | (ucs & 0x3F);
#if defined(WIN32) || defined(__CYGWIN__)
    } else if (ucs >= 0xd800 && ucs <= 0xdbff && i < srclen &&
	       src[i] >= 0xdc00 && src[i] <= 0xdfff) {
      /* surrogate pair */
      unsigned ucs2 = src[i++];
      ucs = 0x10000U + ((ucs&0x3ff)<<10) + (ucs2&0x3ff);
      /* all surrogate pairs turn into 4-byte utf8 */
#else
    } else if (ucs >= 0x10000) {
      if (ucs > 0x10ffff) {
	ucs = 0xfffd;
	goto J1;
      }
#endif
      if (count+4 >= dstlen) {dst[count] = 0; count += 4; break;}
      dst[count++] = 0xf0 | (ucs >> 18);
      dst[count++] = 0x80 | ((ucs >> 12) & 0x3F);
      dst[count++] = 0x80 | ((ucs >> 6) & 0x3F);
      dst[count++] = 0x80 | (ucs & 0x3F);
    } else {
#if !(defined(WIN32) || defined(__CYGWIN__))
    J1:
#endif
      /* all others are 3 bytes: */
      if (count+3 >= dstlen) {dst[count] = 0; count += 3; break;}
      dst[count++] = 0xe0 | (ucs >> 12);
      dst[count++] = 0x80 | ((ucs >> 6) & 0x3F);
      dst[count++] = 0x80 | (ucs & 0x3F);
    }
  }
  /* we filled dst, measure the rest: */
  while (i < srclen) {
    unsigned ucs = src[i++];
    if (ucs < 0x80U) {
      count++;
    } else if (ucs < 0x800U) { /* 2 bytes */
      count += 2;
#if defined(WIN32) || defined(__CYGWIN__)
    } else if (ucs >= 0xd800 && ucs <= 0xdbff && i < srclen-1 &&
	       src[i+1] >= 0xdc00 && src[i+1] <= 0xdfff) {
      /* surrogate pair */
      ++i;
#else
    } else if (ucs >= 0x10000 && ucs <= 0x10ffff) {
#endif
      count += 4;
    } else {
      count += 3;
    }
  }
  return count;
}

/*! Convert an ISO-8859-1 (ie normal c-string) byte stream to UTF-8.

    It is possible this should convert Microsoft's CP1252 to UTF-8
    instead. This would translate the codes in the range 0x80-0x9f
    to different characters. Currently it does not do this.

    Up to \p dstlen bytes are written to \p dst, including a null
    terminator. The return value is the number of bytes that would be
    written, not counting the null terminator. If greater or equal to
    \p dstlen then if you malloc a new array of size n+1 you will have
    the space needed for the entire string. If \p dstlen is zero then
    nothing is written and this call just measures the storage space
    needed.

    \p srclen is the number of bytes in \p src to convert.

    If the return value equals \p srclen then this indicates that
    no conversion is necessary, as only ASCII characters are in the
    string.
*/
unsigned fl_utf8froma(char* dst, unsigned dstlen,
		   const char* src, unsigned srclen) {
  const char* p = src;
  const char* e = src+srclen;
  unsigned count = 0;
  if (dstlen) for (;;) {
    unsigned char ucs;
    if (p >= e) {dst[count] = 0; return count;}
    ucs = *(unsigned char*)p++;
    if (ucs < 0x80U) {
      dst[count++] = ucs;
      if (count >= dstlen) {dst[count-1] = 0; break;}
    } else { /* 2 bytes (note that CP1252 translate could make 3 bytes!) */
      if (count+2 >= dstlen) {dst[count] = 0; count += 2; break;}
      dst[count++] = 0xc0 | (ucs >> 6);
      dst[count++] = 0x80 | (ucs & 0x3F);
    }
  }
  /* we filled dst, measure the rest: */
  while (p < e) {
    unsigned char ucs = *(unsigned char*)p++;
    if (ucs < 0x80U) {
      count++;
    } else {
      count += 2;
    }
  }
  return count;
}

#ifdef WIN32
# include <windows.h>
#endif

/*! Return true if the "locale" seems to indicate that UTF-8 encoding
    is used. If true the fl_utf8to_mb and fl_utf8from_mb don't do anything
    useful.

    <i>It is highly recommended that you change your system so this
    does return true.</i> On Windows this is done by setting the
    "codepage" to CP_UTF8.  On Unix this is done by setting $LC_CTYPE
    to a string containing the letters "utf" or "UTF" in it, or by
    deleting all $LC* and $LANG environment variables. In the future
    it is likely that all non-Asian Unix systems will return true,
    due to the compatibility of UTF-8 with ISO-8859-1.
*/
int fl_utf8locale(void) {
  static int ret = 2;
  if (ret == 2) {
#ifdef WIN32
    ret = GetACP() == CP_UTF8;
#else
    char* s;
    ret = 1; /* assume UTF-8 if no locale */
    if (((s = getenv("LC_CTYPE")) && *s) ||
	((s = getenv("LC_ALL"))   && *s) ||
	((s = getenv("LANG"))     && *s)) {
      ret = (strstr(s,"utf") || strstr(s,"UTF"));
    }
#endif
  }
  return ret;
}

/*! Convert the UTF-8 used by FLTK to the locale-specific encoding
    used for filenames (and sometimes used for data in files).
    Unfortunately due to stupid design you will have to do this as
    needed for filenames. This is a bug on both Unix and Windows.

    Up to \p dstlen bytes are written to \p dst, including a null
    terminator. The return value is the number of bytes that would be
    written, not counting the null terminator. If greater or equal to
    \p dstlen then if you malloc a new array of size n+1 you will have
    the space needed for the entire string. If \p dstlen is zero then
    nothing is written and this call just measures the storage space
    needed.

    If fl_utf8locale() returns true then this does not change the data.
*/
unsigned fl_utf8to_mb(const char* src, unsigned srclen,
		  char* dst, unsigned dstlen)
{
  if (!fl_utf8locale()) {
#ifdef WIN32
    wchar_t lbuf[1024];
    wchar_t* buf = lbuf;
    unsigned length = fl_utf8towc(src, srclen, buf, 1024);
    unsigned ret;
    if (length >= 1024) {
      buf = (wchar_t*)(malloc((length+1)*sizeof(wchar_t)));
      fl_utf8towc(src, srclen, buf, length+1);
    }
    if (dstlen) {
      /* apparently this does not null-terminate, even though msdn
       * documentation claims it does:
       */
      ret =
        WideCharToMultiByte(GetACP(), 0, buf, length, dst, dstlen, 0, 0);
      dst[ret] = 0;
    }
    /* if it overflows or measuring length, get the actual length: */
    if (dstlen==0 || ret >= dstlen-1)
      ret =
	WideCharToMultiByte(GetACP(), 0, buf, length, 0, 0, 0, 0);
    if (buf != lbuf) free((void*)buf);
    return ret;
#else
    wchar_t lbuf[1024];
    wchar_t* buf = lbuf;
    unsigned length = fl_utf8towc(src, srclen, buf, 1024);
    int ret;
    if (length >= 1024) {
      buf = (wchar_t*)(malloc((length+1)*sizeof(wchar_t)));
      fl_utf8towc(src, srclen, buf, length+1);
    }
    if (dstlen) {
      ret = wcstombs(dst, buf, dstlen);
      if (ret >= dstlen-1) ret = wcstombs(0,buf,0);
    } else {
      ret = wcstombs(0,buf,0);
    }
    if (buf != lbuf) free((void*)buf);
    if (ret >= 0) return (unsigned)ret;
    /* on any errors we return the UTF-8 as raw text...*/
#endif
  }
  /* identity transform: */
  if (srclen < dstlen) {
    memcpy(dst, src, srclen);
    dst[srclen] = 0;
  } else {
    /* Buffer insufficent or buffer query */
  }
  return srclen;
}

/*! Convert a filename from the locale-specific multibyte encoding
    used by Windows to UTF-8 as used by FLTK.

    Up to \p dstlen bytes are written to \p dst, including a null
    terminator. The return value is the number of bytes that would be
    written, not counting the null terminator. If greater or equal to
    \p dstlen then if you malloc a new array of size n+1 you will have
    the space needed for the entire string. If \p dstlen is zero then
    nothing is written and this call just measures the storage space
    needed.

    On Unix or on Windows when a UTF-8 locale is in effect, this
    does not change the data.
    You may also want to check if fl_utf8test() returns non-zero, so that
    the filesystem can store filenames in UTF-8 encoding regardless of
    the locale.
*/
unsigned fl_utf8from_mb(char* dst, unsigned dstlen,
		    const char* src, unsigned srclen)
{
  if (!fl_utf8locale()) {
#ifdef WIN32
    wchar_t lbuf[1024];
    wchar_t* buf = lbuf;
    unsigned length;
    unsigned ret;
    length = MultiByteToWideChar(GetACP(), 0, src, srclen, buf, 1024);
    if ((length == 0)&&(GetLastError()==ERROR_INSUFFICIENT_BUFFER)) {
      length = MultiByteToWideChar(GetACP(), 0, src, srclen, 0, 0);
      buf = (wchar_t*)(malloc(length*sizeof(wchar_t)));
      MultiByteToWideChar(GetACP(), 0, src, srclen, buf, length);
    }
    ret = fl_utf8fromwc(dst, dstlen, buf, length);
    if (buf != lbuf) free((void*)buf);
    return ret;
#else
    wchar_t lbuf[1024];
    wchar_t* buf = lbuf;
    int length;
    unsigned ret;
    length = mbstowcs(buf, src, 1024);
    if (length >= 1024) {
      length = mbstowcs(0, src, 0)+1;
      buf = (wchar_t*)(malloc(length*sizeof(wchar_t)));
      mbstowcs(buf, src, length);
    }
    if (length >= 0) {
      ret = fl_utf8fromwc(dst, dstlen, buf, length);
      if (buf != lbuf) free((void*)buf);
      return ret;
    }
    /* errors in conversion return the UTF-8 unchanged */
#endif
  }
  /* identity transform: */
  if (srclen < dstlen) {
    memcpy(dst, src, srclen);
    dst[srclen] = 0;
  } else {
    /* Buffer insufficent or buffer query */
  }
  return srclen;
}

/*! Examines the first \p srclen bytes in \p src and returns a verdict
    on whether it is UTF-8 or not.
    - Returns 0 if there is any illegal UTF-8 sequences, using the
      same rules as fl_utf8decode(). Note that some UCS values considered
      illegal by RFC 3629, such as 0xffff, are considered legal by this.
    - Returns 1 if there are only single-byte characters (ie no bytes
      have the high bit set). This is legal UTF-8, but also indicates
      plain ASCII. It also returns 1 if \p srclen is zero.
    - Returns 2 if there are only characters less than 0x800.
    - Returns 3 if there are only characters less than 0x10000.
    - Returns 4 if there are characters in the 0x10000 to 0x10ffff range.

    Because there are many illegal sequences in UTF-8, it is almost
    impossible for a string in another encoding to be confused with
    UTF-8. This is very useful for transitioning Unix to UTF-8
    filenames, you can simply test each filename with this to decide
    if it is UTF-8 or in the locale encoding. My hope is that if
    this is done we will be able to cleanly transition to a locale-less
    encoding.
*/
int fl_utf8test(const char* src, unsigned srclen) {
  int ret = 1;
  const char* p = src;
  const char* e = src+srclen;
  while (p < e) {
    if (*p & 0x80) {
      int len; fl_utf8decode(p,e,&len);
      if (len < 2) return 0;
      if (len > ret) ret = len;
      p += len;
    } else {
      p++;
    }
  }
  return ret;
}

/* forward declare mk_wcwidth() as static so the name is not visible.
 */
 static int mk_wcwidth(unsigned int ucs);

 /* include the c source directly so it's contents are only visible here
  */
#include "xutf8/mk_wcwidth.c"

/** wrapper to adapt Markus Kuhn's implementation of wcwidth() for FLTK
    \param [in] ucs Unicode character value
    \returns width of character in columns

    See http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c for Markus Kuhn's
    original implementation of wcwidth() and wcswidth()
    (defined in IEEE Std 1002.1-2001) for Unicode.

    \b WARNING: this function returns widths for "raw" Unicode characters.
    It does not even try to map C1 control characters (0x80 to 0x9F) to
    CP1252, and C0/C1 control characters and DEL will return -1.
    You are advised to use fl_width(const char* src) instead.
 */
int fl_wcwidth_(unsigned int ucs) {
  return mk_wcwidth(ucs);
}

/** extended wrapper around  fl_wcwidth_(unsigned int ucs) function.
    \param[in] src pointer to start of UTF-8 byte sequence
    \returns width of character in columns

    Depending on build options, this function may map C1 control
    characters (0x80 to 0x9f) to CP1252, and return the width of
    that character instead. This is not the same behaviour as
    fl_wcwidth_(unsigned int ucs) .

    Note that other control characters and DEL will still return -1,
    so if you want different behaviour, you need to test for those
    characters before calling fl_wcwidth(), and handle them separately.
 */
int fl_wcwidth(const char* src) {
  int len = fl_utf8len(*src);
  int ret = 0;
  unsigned int ucs = fl_utf8decode(src, src+len, &ret);
  int width = fl_wcwidth_(ucs);
  return width;
}

/** @} */

/*
 * End of "$Id: fl_utf.c 8585 2011-04-13 15:43:22Z ianmacarthur $".
 */
