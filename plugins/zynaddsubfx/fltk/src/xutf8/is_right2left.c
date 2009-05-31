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

unsigned short 
XUtf8IsRightToLeft(unsigned int ucs) {

#if 0
  /* for debug only */
  if (ucs <= 0x005A) {
    if (ucs >= 0x0041) return 1;
    return 0;
  }
#endif

  /* HEBREW */
  if (ucs <= 0x05F4) {
    if (ucs >= 0x0591) return 1;
    return 0;
  }
  
  /* ARABIC */
  if (ucs <= 0x06ED) {
    if (ucs >= 0x060C)  return 1;
    return 0;
  }

  if (ucs <= 0x06F9) {
    if (ucs >= 0x06F0) return 1;
    return 0;
  }

  if (ucs == 0x200F) return 1;

  if (ucs == 0x202B) return 1;

  if (ucs == 0x202E) return 1;

  if (ucs <= 0xFB4F) {
    if (ucs >= 0xFB1E) return 1;
    return 0;
  }
  
  if (ucs <= 0xFDFB) {
    if (ucs >= 0xFB50) return 1;
    return 0;
  }

  if (ucs <= 0xFEFC) {
    if (ucs >= 0xFE70) return 1;
    return 0;
  }

  return 0;
}

/*
 * End of "$Id$".
 */
