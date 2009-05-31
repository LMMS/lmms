/* "$Id: $"
 *
 * Author: Jean-Marc Lienher ( http://oksid.ch )
 * Copyright 2000-2003 by O'ksi'D.
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

/*
 * Unicode to UTF-8 conversion functions.
 */

#if !defined(WIN32) && !defined(__APPLE__)

#include "../../FL/Xutf8.h"

/*** NOTE : all functions are LIMITED to 24 bits Unicode values !!! ***/

/* 
 * Converts the first char of the UTF-8 string to an Unicode value 
 * Returns the byte length of the converted UTF-8 char 
 * Returns -1 if the UTF-8 string is not valid 
 */
int
XConvertUtf8ToUcs(const unsigned char     *buf,
		  int                     len,
		  unsigned int         	  *ucs) {

  if (buf[0] & 0x80) {
    if (buf[0] & 0x40) {
      if (buf[0] & 0x20) {
	if (buf[0] & 0x10) {
	  if (buf[0] & 0x08) {
	    if (buf[0] & 0x04) {
	      if (buf[0] & 0x02) {
		/* bad UTF-8 string */
	      } else {
		/* 0x04000000 - 0x7FFFFFFF */
	      }	
	    } else if (len > 4 
		       && (buf[1] & 0xC0) == 0x80
		       && (buf[2] & 0xC0) == 0x80
		       && (buf[3] & 0xC0) == 0x80
		       && (buf[4] & 0xC0) == 0x80) {
	      /* 0x00200000 - 0x03FFFFFF */
	      *ucs =  ((buf[0] & ~0xF8) << 24) +
		      ((buf[1] & ~0x80) << 18) +
		      ((buf[2] & ~0x80) << 12) +
		      ((buf[3] & ~0x80) << 6) +
		       (buf[4] & ~0x80);
	      if (*ucs > 0x001FFFFF && *ucs < 0x01000000) return 5;
	    }
	  } else if (len > 3 
		     && (buf[1] & 0xC0) == 0x80
		     && (buf[2] & 0xC0) == 0x80
		     && (buf[3] & 0xC0) == 0x80) {
	    /* 0x00010000 - 0x001FFFFF */
	    *ucs =  ((buf[0] & ~0xF0) << 18) +
		    ((buf[1] & ~0x80) << 12) +
		    ((buf[2] & ~0x80) << 6) +
		     (buf[3] & ~0x80);
	    if (*ucs > 0x0000FFFF) return 4;
	  }
	} else if (len > 2
	           && (buf[1] & 0xC0) == 0x80 
		   && (buf[2] & 0xC0) == 0x80) {
	  /* 0x00000800 - 0x0000FFFF */
	  *ucs =  ((buf[0] & ~0xE0) << 12) +
		  ((buf[1] & ~0x80) << 6) +
		   (buf[2] & ~0x80);
	  if (*ucs > 0x000007FF) return 3;
	}	
      } else if (len > 1 && (buf[1] & 0xC0) == 0x80) {
	/* 0x00000080 - 0x000007FF */
	*ucs = ((buf[0] & ~0xC0) << 6) +
		(buf[1] & ~0x80);
	if (*ucs > 0x0000007F) return 2;
      }
    }
  } else if (len > 0) {
    /* 0x00000000 - 0x0000007F */
    *ucs = buf[0];
    return 1;
  } 

  *ucs = (unsigned int) '?'; /* bad utf-8 string */
  return -1;
}

/* 
 * Converts an Unicode value to an UTF-8 string 
 * NOTE : the buffer (buf) must be at least 5 bytes long !!!  
 */
int 
XConvertUcsToUtf8(unsigned int 	ucs, 
		  char 		*buf) {

  if (ucs < 0x000080) {
    buf[0] = ucs;
    return 1;
  } else if (ucs < 0x000800) {
    buf[0] = 0xC0 | (ucs >> 6);
    buf[1] = 0x80 | (ucs & 0x3F);
    return 2;
  } else if (ucs < 0x010000) { 
    buf[0] = 0xE0 | (ucs >> 12);
    buf[1] = 0x80 | ((ucs >> 6) & 0x3F);
    buf[2] = 0x80 | (ucs & 0x3F);
    return 3;
  } else if (ucs < 0x00200000) {
    buf[0] = 0xF0 | (ucs >> 18);
    buf[1] = 0x80 | ((ucs >> 12) & 0x3F);
    buf[2] = 0x80 | ((ucs >> 6) & 0x3F);
    buf[3] = 0x80 | (ucs & 0x3F);
    return 4;
  } else if (ucs < 0x01000000) {
    buf[0] = 0xF8 | (ucs >> 24);
    buf[1] = 0x80 | ((ucs >> 18) & 0x3F);
    buf[2] = 0x80 | ((ucs >> 12) & 0x3F);
    buf[3] = 0x80 | ((ucs >> 6) & 0x3F);
    buf[4] = 0x80 | (ucs & 0x3F);
    return 5;
  }
  buf[0] = '?';
  return -1;
}

/* 
 * returns the byte length of the first UTF-8 char 
 * (returns -1 if not valid) 
 */
int
XUtf8CharByteLen(const unsigned char     *buf,
		 int                     len) {
  unsigned int ucs;
  return XConvertUtf8ToUcs(buf, len, &ucs);
}

/*
 * returns the quantity of Unicode chars in the UTF-8 string 
 */
int 
XCountUtf8Char(const unsigned char 	*buf, 
	       int 			len) {

  int i = 0;
  int nbc = 0;
  while (i < len) {
    int cl = XUtf8CharByteLen(buf + i, len - i);
    if (cl < 1) cl = 1;
    nbc++;
    i += cl;
  }
  return nbc;
}

/* 
 * Same as XConvertUtf8ToUcs but no sanity check is done.
 */
int
XFastConvertUtf8ToUcs(const unsigned char     *buf,
		      int                     len,
		      unsigned int            *ucs) {

  if (buf[0] & 0x80) {
    if (buf[0] & 0x40) {
      if (buf[0] & 0x20) {
	if (buf[0] & 0x10) {
	  if (buf[0] & 0x08) {
	    if (buf[0] & 0x04) {
	      if (buf[0] & 0x02) {
		/* bad UTF-8 string */
	      } else {
		/* 0x04000000 - 0x7FFFFFFF */
	      }	
	    } else if (len > 4) {
	      /* 0x00200000 - 0x03FFFFFF */
	      *ucs =  ((buf[0] & ~0xF8) << 24) +
		      ((buf[1] & ~0x80) << 18) +
		      ((buf[2] & ~0x80) << 12) +
		      ((buf[3] & ~0x80) << 6) +
		       (buf[4] & ~0x80);
	      return 5;
	    }
	  } else if (len > 3) {
	    /* 0x00010000 - 0x001FFFFF */
	    *ucs =  ((buf[0] & ~0xF0) << 18) +
		    ((buf[1] & ~0x80) << 12) +
		    ((buf[2] & ~0x80) << 6) +
		     (buf[3] & ~0x80);
	    return 4;
	  }
	} else if (len > 2) {
	  /* 0x00000800 - 0x0000FFFF */
	  *ucs =  ((buf[0] & ~0xE0) << 12) +
		  ((buf[1] & ~0x80) << 6) +
		   (buf[2] & ~0x80);
	  return 3;
	}	
      } else if (len > 1) {
	/* 0x00000080 - 0x000007FF */
	*ucs = ((buf[0] & ~0xC0) << 6) +
		(buf[1] & ~0x80);
	return 2;
      }
    }
  } else if (len > 0) {
    /* 0x00000000 - 0x0000007F */
    *ucs = buf[0];
    return 1;
  } 

  *ucs = (unsigned int) '?'; /* bad utf-8 string */
  return -1;
}

#endif /* X11 only */

/*
 * End of "$Id: $".
 */
