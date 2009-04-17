//
// "$Id: fl_overlay.cxx 5614 2007-01-18 15:25:09Z matt $"
//
// Overlay support for the Fast Light Tool Kit (FLTK).
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

// Extremely limited "overlay" support.  You can use this to drag out
// a rectangle in response to mouse events.  It is your responsibility
// to erase the overlay before drawing anything that might intersect
// it.

#include <FL/x.H>
#include <FL/fl_draw.H>
#ifdef __APPLE__
#include <config.h>
#endif

//#define USE_XOR

static int px,py,pw,ph;

#ifndef USE_XOR
#include <stdlib.h>
static uchar *bgN = 0L, *bgS = 0L, *bgE = 0L, *bgW = 0L;
static int bgx, bgy, bgw, bgh;
#endif

static void draw_current_rect() {
#ifdef USE_XOR
# ifdef WIN32
  int old = SetROP2(fl_gc, R2_NOT);
  fl_rect(px, py, pw, ph);
  SetROP2(fl_gc, old);
# elif defined(__APPLE_QD__)
  PenMode( patXor );
  fl_rect(px, py, pw, ph);
  PenMode( patCopy );
# elif defined(__APPLE_QUARTZ__)
  // warning: Quartz does not support xor drawing
  // Use the Fl_Overlay_Window instead.
  fl_color(FL_WHITE);
  fl_rect(px, py, pw, ph);
# else
  XSetFunction(fl_display, fl_gc, GXxor);
  XSetForeground(fl_display, fl_gc, 0xffffffff);
  XDrawRectangle(fl_display, fl_window, fl_gc, px, py, pw, ph);
  XSetFunction(fl_display, fl_gc, GXcopy);
# endif
#else
  if (bgN) { free(bgN); bgN = 0L; }
  if (bgS) { free(bgS); bgS = 0L; }
  if (bgE) { free(bgE); bgE = 0L; }
  if (bgW) { free(bgW); bgW = 0L; }
  if (pw>0 && ph>0) {
    bgE = fl_read_image(0L, px+pw-1, py, 1, ph);
    bgW = fl_read_image(0L, px, py, 1, ph);
    bgS = fl_read_image(0L, px, py+ph-1, pw, 1);
    bgN = fl_read_image(0L, px, py, pw, 1);
    bgx = px; bgy = py;
    bgw = pw; bgh = ph;
  }
  fl_color(FL_WHITE);
  fl_line_style(FL_SOLID);
  fl_rect(px, py, pw, ph);
  fl_color(FL_BLACK);
  fl_line_style(FL_DOT);
  fl_rect(px, py, pw, ph);
  fl_line_style(FL_SOLID);
#endif
}

static void erase_current_rect() {
#ifdef USE_XOR
# ifdef __APPLE_QUARTZ__
  fl_rect(px, py, pw, ph);
# else
  draw_current_rect();
# endif
#else
  if (bgN) fl_draw_image(bgN, bgx, bgy, bgw, 1);
  if (bgS) fl_draw_image(bgS, bgx, bgy+bgh-1, bgw, 1);
  if (bgW) fl_draw_image(bgW, bgx, bgy, 1, bgh);
  if (bgE) fl_draw_image(bgE, bgx+bgw-1, bgy, 1, bgh);
#endif
}

void fl_overlay_clear() {
  if (pw > 0) {erase_current_rect(); pw = 0;}
}

void fl_overlay_rect(int x, int y, int w, int h) {
  if (w < 0) {x += w; w = -w;} else if (!w) w = 1;
  if (h < 0) {y += h; h = -h;} else if (!h) h = 1;
  if (pw > 0) {
    if (x==px && y==py && w==pw && h==ph) return;
    erase_current_rect();
  }
  px = x; py = y; pw = w; ph = h;
  draw_current_rect();
}

//
// End of "$Id: fl_overlay.cxx 5614 2007-01-18 15:25:09Z matt $".
//
