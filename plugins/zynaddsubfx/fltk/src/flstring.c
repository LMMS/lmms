/*
 * "$Id: flstring.c 4288 2005-04-16 00:13:17Z mike $"
 *
 * BSD string functions for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 1998-2005 by Bill Spitzak and others.
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

#include "flstring.h"


/*
 * 'fl_strlcat()' - Safely concatenate two strings.
 */

size_t				/* O - Length of string */
fl_strlcat(char       *dst,	/* O - Destination string */
           const char *src,	/* I - Source string */
	   size_t     size) {	/* I - Size of destination string buffer */
  size_t	srclen;		/* Length of source string */
  size_t	dstlen;		/* Length of destination string */


 /*
  * Figure out how much room is left...
  */

  dstlen = strlen(dst);
  size   -= dstlen + 1;

  if (!size) return (dstlen);	/* No room, return immediately... */

 /*
  * Figure out how much room is needed...
  */

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size) srclen = size;

  memcpy(dst + dstlen, src, srclen);
  dst[dstlen + srclen] = '\0';

  return (dstlen + srclen);
}


/*
 * 'fl_strlcpy()' - Safely copy two strings.
 */

size_t				/* O - Length of string */
fl_strlcpy(char       *dst,	/* O - Destination string */
           const char *src,	/* I - Source string */
	   size_t      size) {	/* I - Size of destination string buffer */
  size_t	srclen;		/* Length of source string */


 /*
  * Figure out how much room is needed...
  */

  size --;

  srclen = strlen(src);

 /*
  * Copy the appropriate amount...
  */

  if (srclen > size) srclen = size;

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';

  return (srclen);
}


/*
 * End of "$Id: flstring.c 4288 2005-04-16 00:13:17Z mike $".
 */
