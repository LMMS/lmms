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

#include "headers/spacing.h"

unsigned short 
XUtf8IsNonSpacing(unsigned int ucs) {

  if (ucs <= 0x0361) {
    if (ucs >= 0x0300) return ucs_table_0300[ucs - 0x0300];
    return 0;
  }

  if (ucs <= 0x0486) {
    if (ucs >= 0x0483) return ucs_table_0483[ucs - 0x0483];
    return 0;
  }

  if (ucs <= 0x05C4) {
    if (ucs >= 0x0591) return ucs_table_0591[ucs - 0x0591];
    return 0;
  }

  if (ucs <= 0x06ED) {
    if (ucs >= 0x064B) return ucs_table_064B[ucs - 0x064B];
    return 0;
  }

  if (ucs <= 0x0D4D) {
    if (ucs >= 0x0901) return ucs_table_0901[ucs - 0x0901];
    return 0;
  }

  if (ucs <= 0x0FB9) {
    if (ucs >= 0x0E31) return ucs_table_0E31[ucs - 0x0E31];
    return 0;
  }

  if (ucs <= 0x20E1) {
    if (ucs >= 0x20D0) return ucs_table_20D0[ucs - 0x20D0];
    return 0;
  }

  if (ucs <= 0x309A) {
    if (ucs >= 0x302A) return ucs_table_302A[ucs - 0x302A];
    return 0;
  }

  if (ucs <= 0xFB1E) {
    if (ucs >= 0xFB1E) return ucs_table_FB1E[ucs - 0xFB1E];
    return 0;
  }

  if (ucs <= 0xFE23) {
    if (ucs >= 0xFE20) return ucs_table_FE20[ucs - 0xFE20];
    return 0;
  }

  return 0;
}

/*
 * End of "$Id$".
 */
