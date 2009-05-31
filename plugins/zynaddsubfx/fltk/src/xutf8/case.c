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
 * This file is required on all platforms for utf8 support
 */

#include "headers/case.h"
#include <stdlib.h>

int 
XUtf8Tolower(int ucs) {
  int ret;
  if (ucs <= 0x02B6) {
    if (ucs >= 0x0041) {
      ret = ucs_table_0041[ucs - 0x0041];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x0556) {
    if (ucs >= 0x0386) {
      ret = ucs_table_0386[ucs - 0x0386];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x10C5) {
    if (ucs >= 0x10A0) {
      ret = ucs_table_10A0[ucs - 0x10A0];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x1FFC) {
    if (ucs >= 0x1E00) {
      ret = ucs_table_1E00[ucs - 0x1E00];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x2133) {
    if (ucs >= 0x2102) {
      ret = ucs_table_2102[ucs - 0x2102];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x24CF) {
    if (ucs >= 0x24B6) {
      ret = ucs_table_24B6[ucs - 0x24B6];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0x33CE) {
    if (ucs >= 0x33CE) {
      ret = ucs_table_33CE[ucs - 0x33CE];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  if (ucs <= 0xFF3A) {
    if (ucs >= 0xFF21) {
      ret = ucs_table_FF21[ucs - 0xFF21];
      if (ret > 0) return ret;
    }
    return ucs;
  }

  return ucs;
}

int 
XUtf8Toupper(int ucs) {
  int i;
  static unsigned short *table = NULL;

  if (!table) {
    table = (unsigned short*) malloc(sizeof(unsigned short) * 0x10000);
    for (i = 0; i < 0x10000; i++) {
      table[i] = (unsigned short) i;
    }
    for (i = 0; i < 0x10000; i++) {
      int l;
      l = XUtf8Tolower(i);
      if (l != i) table[l] = (unsigned short) i;
    }
  }
  if (ucs >= 0x10000 || ucs < 0) return ucs;
  return table[ucs];
}

/*
* End of "$Id$".
*/
