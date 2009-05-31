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

#include <stdlib.h>
#include <string.h>

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
#include "lcUniConv/iso8859_10.h"
#include "lcUniConv/iso8859_11.h"
#include "lcUniConv/iso8859_13.h"
#include "lcUniConv/iso8859_14.h"
#include "lcUniConv/iso8859_15.h"
#include "lcUniConv/iso8859_2.h"
#include "lcUniConv/iso8859_3.h"
#include "lcUniConv/iso8859_4.h"
#include "lcUniConv/iso8859_5.h"
#include "lcUniConv/iso8859_6.h"
#include "lcUniConv/iso8859_7.h"
#include "lcUniConv/iso8859_8.h"
#include "lcUniConv/iso8859_9.h"
#include "lcUniConv/jisx0201.h"
#include "lcUniConv/jisx0208.h"
#include "lcUniConv/jisx0212.h"
#include "lcUniConv/koi8_r.h"
#include "lcUniConv/koi8_u.h"
#include "lcUniConv/ksc5601.h"
#include "lcUniConv/cp1251.h"
#include "headers/symbol_.h"
#include "headers/dingbats_.h"
  
/*************** conv_gen.c ************/

/*const*/
int ucs2fontmap(char *s, unsigned int ucs, int enc) {
  switch(enc) {
  case 0:	/* iso10646-1 */
    s[0] = (char) ((ucs & 0xFF00) >> 8);
    s[1] = (char) (ucs & 0xFF);
    return 0;
    break;
  case 1:	/* iso8859-1 */
    if (ucs <= 0x00FF) {
      if (ucs >= 0x0001) {
        s[0] = 0;
        s[1] = (char) (ucs & 0xFF);
        return 1;
      }
    }
    break;
  case 2:	/* iso8859-2 */
    if (ucs <= 0x00a0) {
      s[0] = 0;
      s[1] = (char) ucs;
      return 2;
    } else if (ucs < 0x0180) {
      if (ucs >= 0x00a0) {
	s[0] = 0;
	s[1] = (char)  iso8859_2_page00[ucs-0x00a0];
        if (s[1]) return 2;
      }
    } else if (ucs < 0x02e0) {
      if (ucs >= 0x02c0) {
        s[0] = 0;
        s[1] = (char) iso8859_2_page02[ucs-0x02c0];
        if (s[1]) return 2;
      }	
    }
    break;
  case 3:	/* iso8859-3 */
    if (iso8859_3_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 3;
    } 
    break;
  case 4:	/* iso8859-4 */
    if (iso8859_4_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 4;
    } 
    break;
  case 5:	/* iso8859-5 */
    if (iso8859_5_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 5;
    } 
    break;
  case 6:	/* iso8859-6 */
    if (iso8859_6_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 6;
    } 
    break;
  case 7:	/* iso8859-7 */
    if (iso8859_7_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 7;
    } 
    break;
  case 8:	/* iso8859-8 */
    if (iso8859_8_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 8;
    } 
    break;
  case 9:	/* iso8859-9 */
    if (iso8859_9_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 9;
    } 
    break;
  case 10:	/* iso8859-10 */
    if (iso8859_10_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 10;
    } 
    break;
  case 25:	/* iso8859-11 */
    if (iso8859_11_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 25;
    } 
    break;
  case 11:	/* iso8859-13 */
    if (iso8859_13_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 11;
    } 
    break;
  case 12:	/* iso8859-14 */
    if (iso8859_14_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 12;
    } 
    break;
  case 13:	/* iso8859-15 */
    if (iso8859_15_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 13;
    } 
    break;
  case 14:	/* koi8-r */
    if (koi8_r_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 14;
    } 
    break;
  case 15:	/* big5 */
    if (big5_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 15;
    } 
    break;
  case 16:	/* ksc5601.1987-0 */
    if (ksc5601_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 16;
    } 
    break;
  case 17:	/* gb2312.1980-0 */
    if (gb2312_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 17;
    } 
    break;
  case 18:	/* jisx0201.1976-0 */
    if (jisx0201_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 18;
    } 
    break;
  case 19:	/* jisx0208.1983-0 */
    if (jisx0208_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 19;
    } 
    break;
  case 20:	/* jisx0212.1990-0 */
    if (jisx0212_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 20;
    } 
    break;
  case 21:	/* symbol */
    if (ucs <= 0x00F7) {
      if (ucs >= 0x0020) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_0020[ucs - 0x0020];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0x0192) {
      if (ucs >= 0x0192) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_0192[ucs - 0x0192];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0x03D6) {
      if (ucs >= 0x0391) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_0391[ucs - 0x0391];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0x232A) {
      if (ucs >= 0x2022) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_2022[ucs - 0x2022];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0x25CA) {
      if (ucs >= 0x25CA) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_25CA[ucs - 0x25CA];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0x2666) {
      if (ucs >= 0x2660) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_2660[ucs - 0x2660];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0xF6DB) {
      if (ucs >= 0xF6D9) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_F6D9[ucs - 0xF6D9];
        if (s[1]) return 21;
      }
    } else if (ucs <= 0xF8FE) {
      if (ucs >= 0xF8E5) {
        s[0] = 0;
        s[1] = unicode_to_symbol_1b_F8E5[ucs - 0xF8E5];
        if (s[1]) return 21;
      }
    }
    break;
  case 22:	/* dingbats */
    if (ucs <= 0x00A0) {
      if (ucs >= 0x0020) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_0020[ucs - 0x0020];
        if (s[1]) return 22;
      }
    } else if (ucs <= 0x2195) {
      if (ucs >= 0x2192) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_2192[ucs - 0x2192];
        if (s[1]) return 22;
      }
    } else if (ucs <= 0x2469) {
      if (ucs >= 0x2460) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_2460[ucs - 0x2460];
        if (s[1]) return 22;
      }
    } else if (ucs <= 0x2666) {
      if (ucs >= 0x25A0) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_25A0[ucs - 0x25A0];
        if (s[1]) return 22;
      }
    } else if (ucs <= 0x27BE) {
      if (ucs >= 0x2701) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_2701[ucs - 0x2701];
        if (s[1]) return 22;
      }
    } else if (ucs <= 0xF8E4) {
      if (ucs >= 0xF8D7) {
        s[0] = 0;
        s[1] = unicode_to_dingbats_1b_F8D7[ucs - 0xF8D7];
        if (s[1]) return 22;
      }
    }
    break;
  case 23:	/* koi8-u */
    if (koi8_u_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 23;
    } 
    break;
  case 24:	/* microsoft-cp1251 */
    if (cp1251_wctomb(NULL, (unsigned char*)s, ucs, 2) > 0) {
      return 24;
    } 
    break;
  default:
    break;
  };
  return -1;
};

/*const*/
int encoding_number(const char *enc) {
  if (!enc || !strncmp(enc, "iso10646-1", 10)) {
    return 0;
  } else if (!strcmp(enc, "iso8859-1")) {
    return 1;
  } else if (!strcmp(enc, "iso8859-2")) {
    return 2;
  } else if (!strcmp(enc, "iso8859-3")) {
    return 3;
  } else if (!strcmp(enc, "iso8859-4")) {
    return 4;
  } else if (!strcmp(enc, "iso8859-5")) {
    return 5;
  } else if (!strcmp(enc, "iso8859-6")) {
    return 6;
  } else if (!strcmp(enc, "iso8859-7")) {
    return 7;
  } else if (!strcmp(enc, "iso8859-8")) {
    return 8;
  } else if (!strcmp(enc, "iso8859-9")) {
    return 9;
  } else if (!strcmp(enc, "iso8859-10")) {
    return 10;
  } else if (!strcmp(enc, "iso8859-13")) {
    return 11;
  } else if (!strcmp(enc, "iso8859-14")) {
    return 12;
  } else if (!strcmp(enc, "iso8859-15")) {
    return 13;
  } else if (!strcmp(enc, "koi8-r")) {
    return 14;
  } else if (!strcmp(enc, "big5-0") || !strcmp(enc, "big5.eten-0") ||
    !strcmp(enc, "big5p-0")) 
  {
    return 15;
  } else if (!strcmp(enc, "ksc5601.1987-0")) {
    return 16;
  } else if (!strcmp(enc, "gb2312.1980-0") || !strcmp(enc, "gb2312.80-0") || 
     !strcmp(enc, "gb2312.80&gb8565.88") ||  !strcmp(enc, "gb2312.80-0")) 
  {
    return 17;
  } else if (!strcmp(enc, "jisx0201.1976-0")) {
    return 18;
  } else if (!strcmp(enc, "jisx0208.1983-0") || !strcmp(enc, "jisx0208.1990-0")
    || !strcmp(enc, "jisx0208.1978-0")) 
  {
    return 19;
  } else if (!strcmp(enc, "jisx0212.1990-0")) {
    return 20;
  } else if (!strcmp(enc, "symbol")) {
    return 21;
  } else if (!strcmp(enc, "dingbats") || !strcmp(enc, "zapfdingbats") || 
    !strcmp(enc, "zapf dingbats") || !strcmp(enc, "itc zapf dingbats")) 
  {
    return 22;
  } else if (!strcmp(enc, "koi8-u")) {
    return 23;
  } else if (!strcmp(enc, "microsoft-cp1251")) {
    return 24;
  } else if (!strcmp(enc, "iso8859-11")) {
    return 25;
  };
  return -1;
};

/*
 * End of "$Id$".
 */
