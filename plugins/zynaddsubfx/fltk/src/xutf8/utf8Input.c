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

#if !defined(WIN32) && !defined(__APPLE__)

#include "config.h"
#include "../../FL/Xutf8.h"
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <stdlib.h>

#if HAVE_LIBC_ICONV
#include <iconv.h>
#endif
/*
  I haven't found much doc on the web about EUC encodings, so I've used
  GNU libiconv source code as a reference.
  http://clisp.cons.org/~haible/packages-libiconv.html
*/

#define RET_ILSEQ -1
#define RET_TOOFEW(x) (-10 - x)
#define RET_TOOSMALL -2
#define conv_t void*
#define ucs4_t unsigned int
typedef struct {
  unsigned short indx;
  unsigned short used;
} Summary16;

#include "lcUniConv/big5.h"
#include "lcUniConv/gb2312.h"
#include "lcUniConv/jisx0201.h"
#include "lcUniConv/jisx0208.h"
#include "lcUniConv/jisx0212.h"
#include "lcUniConv/ksc5601.h"

int 
XConvertEucTwToUtf8(char* buffer_return, int len) {
  /* FIXME */
#if HAVE_LIBC_ICONV
  iconv_t cd;
  int cdl;
#else
  int i = 0;
#endif
  int l = 0;
  char *buf, *b;

  if (len < 1) return 0;
  b = buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned) len);

#if HAVE_LIBC_ICONV
  l = cdl = len;
  cd = iconv_open("EUC-TW", "UTF-8");
  iconv(cd, &b, &len, &buffer_return, &cdl);
  iconv_close(cd);
  l -= cdl;	
#else
  while (i < len) {
    unsigned int ucs;
    unsigned char c; 
    c = (unsigned char) buf[i];
    if (c < 0x80) {
      ucs = c;	
      i++;
    } else if (c >= 0xa1 && c < 0xff && len - i > 1 ) {
      unsigned char b[2];
      b[0] = (unsigned char) c - 0x80;
      b[1] = (unsigned char) buf[i + 1] - 0x80;
      ucs = ' '; i += 2;
    } else if (c == 0x8e &&  len - i > 3) {
      unsigned char b[2];
      unsigned char c1 =  buf[i + 1];
      unsigned char c2 =  buf[i + 2];
      unsigned char c3 =  buf[i + 3];
      b[0] = (unsigned char)  buf[i + 2] - 0x80;
      b[1] = (unsigned char)  buf[i + 3] - 0x80;
      if (c1 >= 0xa1 && c1 <= 0xb0) {
	if (c2 >= 0xa1 && c2 < 0xff && c3 >= 0xa1 && c3 < 0xff) {
	  ucs = ' '; i += 4;
	} else {
	  ucs = '?'; i++;
	}
      } else {
	ucs = '?'; i++;
      }
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
#endif
  free(buf);
  return l;
}

int 
XConvertEucKrToUtf8(char* buffer_return, int len) {
  int i = 0, l = 0;
  char *buf;

  if (len < 1) return 0;

  buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned)len);

  while (i < len) {
    unsigned int ucs;
    unsigned char c, c1;
    c = (unsigned char) buf[i];
    if (c < 0x80) {
      ucs = c;	
      i++;
    } else if (c >= 0xA1 && c < 0xFF && len - i > 1) {
      c1 = (unsigned char) buf[i + 1];
      if (c1 >= 0xa1 && c1 < 0xff) {
	unsigned char b[2];
	b[0] = c - 0x80;
	b[1] = c1 - 0x80;
	if (ksc5601_mbtowc(NULL, &ucs, b, 2) < 1) {
	  ucs = '?';
	}
      } else {
	ucs = '?';
      }
      i += 2;
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
  free(buf);
  return l;
}

int 
XConvertBig5ToUtf8(char* buffer_return, int len) {
  int i = 0, l = 0;
  char *buf;

  if (len < 1) return 0;
  buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned)len);

  if (len == 1) {
    l += XConvertUcsToUtf8((unsigned int)buf[i], buffer_return + l);
  }
  while (i + 1 < len) {
    unsigned int ucs;
    unsigned char b[2];
    b[0] = (unsigned char) buf[i];
    b[1] = (unsigned char) buf[i + 1];
    if (big5_mbtowc(NULL, &ucs, b, 2) == 2) {
      i += 2;
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
  free(buf);
  return l;
}

int 
XConvertGb2312ToUtf8(char* buffer_return, int len) {
  int i = 0, l = 0;
  char *buf;

  if (len < 1) return 0;
  buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned)len);

  if (len == 1) {
    l += XConvertUcsToUtf8((unsigned int)buf[i], buffer_return + l);
  }
  while (i + 1 < len) {
    unsigned int ucs;
    unsigned char b[2];
    b[0] = (unsigned char) buf[i];
    b[1] = (unsigned char) buf[i + 1];
    if (gb2312_mbtowc(NULL, &ucs, b, 2) == 2) {
      i += 2;
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
  free(buf);
  return l;
}

int 
XConvertEucCnToUtf8(char* buffer_return, int len) {
  int i = 0, l = 0;
  char *buf;

  if (len < 1) return 0;
  buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned)len);

  while (i < len) {
    unsigned int ucs;
    unsigned char c, c1;
    c = (unsigned char) buf[i];
    if (c < 0x80) {
      ucs = c;	
      i++;
    } else if (c >= 0xA1 && c < 0xFF && len - i > 1) {
      c1 = (unsigned char) buf[i + 1];
      if (c1 >= 0xa1 && c1 < 0xff) {	
	unsigned char b[2];
	b[0] = (unsigned char) c;
	b[1] = (unsigned char) c1;
	if (gb2312_mbtowc(NULL, &ucs, b, 2) < 1) {
	  ucs = '?';
	}	
      } else {
	ucs = '?';
      }
      i += 2;
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
  free(buf);
  return l;
}

int 
XConvertEucJpToUtf8(char* buffer_return, int len) {
  int i = 0, l = 0;
  char *buf;

  if (len < 1) return 0;
  buf = (char*) malloc((unsigned)len);
  memcpy(buf, buffer_return, (unsigned)len);

  while (i < len) {
    unsigned int ucs;
    unsigned char c, c1;
    c = (unsigned char) buf[i];
    if (c < 0x80) {
      ucs = c;	
      i++;
    } else if (c >= 0xA1 && c < 0xFF && len - i > 1) {
      c1 = (unsigned char) buf[i + 1];		
      if (c < 0xF5 && c1 >= 0xa1) {
	unsigned char b[2];
	b[0] = c - 0x80;
	b[1] = c1 - 0x80;
	if (jisx0208_mbtowc(NULL, &ucs, b, 2) < 1) { 
	  ucs = '?';
	}
      } else if (c1 >= 0xA1 && c1 < 0xFF) {
	ucs = 0xE000 + 94 * (c - 0xF5) + (c1 - 0xA1);
      } else {
	ucs = '?';
      }
      i += 2;
    } else if (c == 0x8E && len - i > 1) {
      c1 = (unsigned char) buf[i + 1];		
      if (c1 >= 0xa1 && c1 <= 0xe0) {
	if (jisx0201_mbtowc(NULL, &ucs, &c1, 1) != 1) {
	  ucs = '?';
	}
      } else {
	ucs = '?';
      }
      i += 2;
    } else if (c == 0x8F && len - i > 2) {
      c = (unsigned char) buf[i + 1];		
      c1 = (unsigned char) buf[i + 2];	
      if (c >= 0xa1 && c < 0xff) {
	if (c < 0xf5 && c1 >= 0xa1 && c1 < 0xff) {
	  unsigned char b[2];
	  b[0] = c - 0x80;
	  b[1] = c1 - 0x80;
	  if (jisx0212_mbtowc(NULL, &ucs, b, 2) < 1) {
	    ucs = '?';
	  }
	} else {
	  ucs = '?';
	}
      } else {
	if (c1 >= 0xa1 && c1 < 0xff) {
	  ucs = 0xe3ac + 94 * (c - 0xF5) + (c1 - 0xA1);
	} else {
	  ucs = '?';
	}
      }
      i += 3;
    } else {
      ucs = '?';
      i++;
    }
    l += XConvertUcsToUtf8(ucs, buffer_return + l);
  }
  free(buf);
  return l;
}

int
XConvertEucToUtf8(const char*	locale,
		  char*		buffer_return, 
		  int		len, 
		  int		bytes_buffer) {

  if (!locale/* || strstr(locale, "UTF") || strstr(locale, "utf")*/) {
    return len;
  }

  if (strstr(locale, "ja")) {	
    return XConvertEucJpToUtf8(buffer_return, len);
  } else if (strstr(locale, "Big5") || strstr(locale, "big5")) { /* BIG5 */
    return XConvertBig5ToUtf8(buffer_return, len);
  } else if (strstr(locale, "zh") || strstr(locale, "chinese-")) {
    if (strstr(locale, "TW") || strstr(locale, "chinese-t")) {
      if (strstr(locale, "EUC") || strstr(locale, "euc") || strstr(locale, "chinese-t")) {
	return XConvertEucTwToUtf8(buffer_return, len);
      }
      return XConvertBig5ToUtf8(buffer_return, len);
    }
    if (strstr(locale, "EUC") || strstr(locale, "euc")) {
      return XConvertEucCnToUtf8(buffer_return, len);
    }
    return XConvertGb2312ToUtf8(buffer_return, len);
  } else if (strstr(locale, "ko")) { 
    return XConvertEucKrToUtf8(buffer_return, len);
  }
  return len;
}

int
XUtf8LookupString(XIC                 ic,
		  XKeyPressedEvent*   event,
		  char*               buffer_return,
		  int                 bytes_buffer,
		  KeySym*             keysym,
		  Status*             status_return) {

  long ucs = -1;
  int len;
  len = XmbLookupString(ic, event, buffer_return, bytes_buffer / 5,
		        keysym, status_return);
  if (*status_return == XBufferOverflow) {
    return len * 5;
  }
  if (*keysym > 0 && *keysym < 0x100 && len == 1) {
    if (*keysym < 0x80) {
      ucs = (unsigned char)buffer_return[0];
    } else {
      ucs = *keysym;
    }
  } else  if (((*keysym >= 0x100 && *keysym <= 0xf000) ||
	      (*keysym & 0xff000000U) == 0x01000000))
  {
    ucs = XKeysymToUcs(*keysym);
  } else {
    ucs = -2;
  }

  if (ucs > 0) {
    len = XConvertUcsToUtf8((unsigned)ucs, (char *)buffer_return);
  } else if (len > 0) {
    XIM im;
    if (!ic) return 0;
    im = XIMOfIC(ic);
    if (!im) return 0;
    len = XConvertEucToUtf8(XLocaleOfIM(im), buffer_return, len, bytes_buffer);	
  }
  return len;
}

#endif /* X11 only */

/*
 * End of "$Id$".
 */
