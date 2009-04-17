//
// "$Id: fl_line_style.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Line style code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include "flstring.h"
#include <stdio.h>

#ifdef __APPLE_QUARTZ__
float fl_quartz_line_width_ = 1.0f;
static enum CGLineCap fl_quartz_line_cap_ = kCGLineCapButt;
static enum CGLineJoin fl_quartz_line_join_ = kCGLineJoinMiter;
static float *fl_quartz_line_pattern = 0;
static int fl_quartz_line_pattern_size = 0;
void fl_quartz_restore_line_style_() {
  CGContextSetLineWidth(fl_gc, fl_quartz_line_width_);
  CGContextSetLineCap(fl_gc, fl_quartz_line_cap_);
  CGContextSetLineJoin(fl_gc, fl_quartz_line_join_);
  CGContextSetLineDash(fl_gc, 0, fl_quartz_line_pattern, fl_quartz_line_pattern_size);
}
#endif

void fl_line_style(int style, int width, char* dashes) {
#ifdef WIN32
  // According to Bill, the "default" cap and join should be the
  // "fastest" mode supported for the platform.  I don't know why
  // they should be different (same graphics cards, etc., right?) MRS
  static DWORD Cap[4]= {PS_ENDCAP_FLAT, PS_ENDCAP_FLAT, PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE};
  static DWORD Join[4]={PS_JOIN_ROUND, PS_JOIN_MITER, PS_JOIN_ROUND, PS_JOIN_BEVEL};
  int s1 = PS_GEOMETRIC | Cap[(style>>8)&3] | Join[(style>>12)&3];
  DWORD a[16]; int n = 0;
  if (dashes && dashes[0]) {
    s1 |= PS_USERSTYLE;
    for (n = 0; n < 16 && *dashes; n++) a[n] = *dashes++;
  } else {
    s1 |= style & 0xff; // allow them to pass any low 8 bits for style
  }
  if ((style || n) && !width) width = 1; // fix cards that do nothing for 0?
  LOGBRUSH penbrush = {BS_SOLID,fl_RGB(),0}; // can this be fl_brush()?
  HPEN newpen = ExtCreatePen(s1, width, &penbrush, n, n ? a : 0);
  if (!newpen) {
    Fl::error("fl_line_style(): Could not create GDI pen object.");
    return;
  }
  HPEN oldpen = (HPEN)SelectObject(fl_gc, newpen);
  DeleteObject(oldpen);
  DeleteObject(fl_current_xmap->pen);
  fl_current_xmap->pen = newpen;
#elif defined(__APPLE_QD__)
  // QuickDraw supports pen size and pattern, but no arbitrary line styles.
  static Pattern	styles[] = {
    { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } },	// FL_SOLID
    { { 0xf0, 0xf0, 0xf0, 0xf0, 0x0f, 0x0f, 0x0f, 0x0f } },	// FL_DASH
    { { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 } }	// FL_DOT
  };

  if (!width) width = 1;
  PenSize(width, width);

  style &= 0xff;
  if (style > 2) style = 2;
  PenPat(styles + style);
#elif defined(__APPLE_QUARTZ__)
  static enum CGLineCap Cap[4] = { kCGLineCapButt, kCGLineCapButt, 
                                   kCGLineCapRound, kCGLineCapSquare };
  static enum CGLineJoin Join[4] = { kCGLineJoinMiter, kCGLineJoinMiter, 
                                    kCGLineJoinRound, kCGLineJoinBevel };
  if (width<1) width = 1;
  fl_quartz_line_width_ = (float)width; 
  fl_quartz_line_cap_ = Cap[(style>>8)&3];
  fl_quartz_line_join_ = Join[(style>>12)&3];
  char *d = dashes; 
  static float pattern[16];
  if (d && *d) {
    float *p = pattern;
    while (*d) { *p++ = (float)*d++; }
    fl_quartz_line_pattern = pattern;
    fl_quartz_line_pattern_size = d-dashes;
  } else if (style & 0xff) {
    char dash, dot, gap;
    // adjust lengths to account for cap:
    if (style & 0x200) {
      dash = char(2*width);
      dot = 1; 
      gap = char(2*width-1);
    } else {
      dash = char(3*width);
      dot = gap = char(width);
    }
    float *p = pattern;
    switch (style & 0xff) {
    case FL_DASH:       *p++ = dash; *p++ = gap; break;
    case FL_DOT:        *p++ = dot; *p++ = gap; break;
    case FL_DASHDOT:    *p++ = dash; *p++ = gap; *p++ = dot; *p++ = gap; break;
    case FL_DASHDOTDOT: *p++ = dash; *p++ = gap; *p++ = dot; *p++ = gap; *p++ = dot; *p++ = gap; break;
    }
    fl_quartz_line_pattern_size = p-pattern;
    fl_quartz_line_pattern = pattern;
  } else {
    fl_quartz_line_pattern = 0; fl_quartz_line_pattern_size = 0;
  }
  fl_quartz_restore_line_style_();
#else
  int ndashes = dashes ? strlen(dashes) : 0;
  // emulate the WIN32 dash patterns on X
  char buf[7];
  if (!ndashes && (style&0xff)) {
    int w = width ? width : 1;
    char dash, dot, gap;
    // adjust lengths to account for cap:
    if (style & 0x200) {
      dash = char(2*w);
      dot = 1; // unfortunately 0 does not work
      gap = char(2*w-1);
    } else {
      dash = char(3*w);
      dot = gap = char(w);
    }
    char* p = dashes = buf;
    switch (style & 0xff) {
    case FL_DASH:	*p++ = dash; *p++ = gap; break;
    case FL_DOT:	*p++ = dot; *p++ = gap; break;
    case FL_DASHDOT:	*p++ = dash; *p++ = gap; *p++ = dot; *p++ = gap; break;
    case FL_DASHDOTDOT: *p++ = dash; *p++ = gap; *p++ = dot; *p++ = gap; *p++ = dot; *p++ = gap; break;
    }
    ndashes = p-buf;
  }
  static int Cap[4] = {CapButt, CapButt, CapRound, CapProjecting};
  static int Join[4] = {JoinMiter, JoinMiter, JoinRound, JoinBevel};
  XSetLineAttributes(fl_display, fl_gc, width, 
		     ndashes ? LineOnOffDash : LineSolid,
		     Cap[(style>>8)&3], Join[(style>>12)&3]);
  if (ndashes) XSetDashes(fl_display, fl_gc, 0, dashes, ndashes);
#endif
}


//
// End of "$Id: fl_line_style.cxx 5190 2006-06-09 16:16:34Z mike $".
//
