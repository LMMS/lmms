//
// "$Id: fl_rounded_box.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Rounded box drawing routines for the Fast Light Tool Kit (FLTK).
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

#define RN	5
#define RS	15
#define BW	3

static double offset[RN] = { 0.0, 0.07612, 0.29289, 0.61732, 1.0};

static void rbox(int fill, int x, int y, int w, int h) {
  int i;
  int rsx ,rsy, rs;
  rsx = w*2/5; rsy = h*2/5;
  if (rsx > rsy) rs = rsy; else  rs = rsx;
  if (rs > RS) rs = RS;
  rsx = rs; rsy = rs;

  if (fill) fl_begin_polygon(); else fl_begin_loop();
  for (i=0; i<RN; i++)
    fl_vertex(x + offset[RN-i-1]*rsx, y + offset[i] * rsy);
  for (i=0; i<RN; i++)
    fl_vertex(x + offset[i]*rsx, y + h-1 - offset[RN-i-1] * rsy);
  for (i=0; i<RN; i++)
    fl_vertex(x + w-1 - offset[RN-i-1]*rsx, y + h-1 - offset[i] * rsy);
  for (i=0; i<RN; i++)
    fl_vertex(x + w-1 - offset[i]*rsx, y + offset[RN-i-1] * rsy);
  if (fill) fl_end_polygon(); else fl_end_loop();
}

static void fl_rflat_box(int x, int y, int w, int h, Fl_Color c) {
  fl_color(c); rbox(1, x, y, w, h); rbox(0, x, y, w, h);
}

static void fl_rounded_frame(int x, int y, int w, int h, Fl_Color c) {
  fl_color(c); rbox(0, x, y, w, h);
}

static void fl_rounded_box(int x, int y, int w, int h, Fl_Color c) {
  fl_color(c); rbox(1, x, y, w, h);
  fl_color(FL_BLACK); rbox(0, x, y, w, h);
}

static void fl_rshadow_box(int x, int y, int w, int h, Fl_Color c) {
  // draw shadow:
  fl_color(FL_DARK3);
  rbox(1, x+BW, y+BW, w, h);
  rbox(0, x+BW, y+BW, w, h);
  // draw the box:
  fl_rounded_box(x, y, w, h, c);
}

extern void fl_internal_boxtype(Fl_Boxtype, Fl_Box_Draw_F*);

Fl_Boxtype fl_define_FL_ROUNDED_BOX() {
  fl_internal_boxtype(_FL_ROUNDED_FRAME, fl_rounded_frame);
  fl_internal_boxtype(_FL_ROUNDED_BOX, fl_rounded_box);
  return _FL_ROUNDED_BOX;
}

Fl_Boxtype fl_define_FL_RFLAT_BOX() {
  fl_internal_boxtype(_FL_RFLAT_BOX, fl_rflat_box);
  return _FL_RFLAT_BOX;
}

Fl_Boxtype fl_define_FL_RSHADOW_BOX() {
  fl_internal_boxtype(_FL_RSHADOW_BOX, fl_rshadow_box);
  return _FL_RSHADOW_BOX;
}

//
// End of "$Id: fl_rounded_box.cxx 5190 2006-06-09 16:16:34Z mike $".
//
