//
// "$Id: Fl_Light_Button.cxx 5721 2007-02-27 19:23:24Z matt $"
//
// Lighted button widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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

// Subclass of Fl_Button where the "box" indicates whether it is
// pushed or not, and the "down box" is drawn small and square on
// the left to indicate the current state.

// The default down_box of zero draws a rectangle designed to look
// just like Flame's buttons.

#include <FL/Fl.H>
#include <FL/Fl_Light_Button.H>
#include <FL/fl_draw.H>
#include "flstring.h"

void Fl_Light_Button::draw() {
  if (box()) draw_box(this==Fl::pushed() ? fl_down(box()) : box(), color());
  Fl_Color col = value() ? (active_r() ? selection_color() :
                            fl_inactive(selection_color())) : color();
  int W;
  int dx, dy;

  W  = labelsize();
  dx = Fl::box_dx(box()) + 2;
  dy = (h() - W) / 2;
  // if (dy < 0) dy = 0;         // neg. offset o.k. for vertical centering

  if (down_box()) {
    // draw other down_box() styles:
    switch (down_box()) {
      case FL_DOWN_BOX :
      case FL_UP_BOX :
      case _FL_PLASTIC_DOWN_BOX :
      case _FL_PLASTIC_UP_BOX :
        // Check box...
        draw_box(down_box(), x()+dx, y()+dy, W, W, FL_BACKGROUND2_COLOR);
	if (value()) {
	  if (Fl::scheme() && !strcmp(Fl::scheme(), "gtk+")) {
	    fl_color(FL_SELECTION_COLOR);
	  } else {
	    fl_color(col);
	  }
	  int tx = x() + dx + 3;
	  int tw = W - 6;
	  int d1 = tw/3;
	  int d2 = tw-d1;
	  int ty = y() + dy + (W+d2)/2-d1-2;
	  for (int n = 0; n < 3; n++, ty++) {
	    fl_line(tx, ty, tx+d1, ty+d1);
	    fl_line(tx+d1, ty+d1, tx+tw-1, ty+d1-d2+1);
	  }
	}
        break;
      case _FL_ROUND_DOWN_BOX :
      case _FL_ROUND_UP_BOX :
        // Radio button...
        draw_box(down_box(), x()+dx, y()+dy, W, W, FL_BACKGROUND2_COLOR);
	if (value()) {
	  int tW = (W - Fl::box_dw(down_box())) / 2 + 1;
	  if ((W - tW) & 1) tW++; // Make sure difference is even to center
	  int tdx = dx + (W - tW) / 2;
	  int tdy = dy + (W - tW) / 2;

	  if (Fl::scheme() && !strcmp(Fl::scheme(), "gtk+")) {
	    fl_color(FL_SELECTION_COLOR);
	    tW --;
	    fl_pie(x() + tdx - 1, y() + tdy - 1, tW + 3, tW + 3, 0.0, 360.0);
	    fl_arc(x() + tdx - 1, y() + tdy - 1, tW + 3, tW + 3, 0.0, 360.0);
	    fl_color(fl_color_average(FL_WHITE, FL_SELECTION_COLOR, 0.2f));
	  } else fl_color(col);

	  switch (tW) {
	    // Larger circles draw fine...
	    default :
              fl_pie(x() + tdx, y() + tdy, tW, tW, 0.0, 360.0);
	      break;

            // Small circles don't draw well on many systems...
	    case 6 :
	      fl_rectf(x() + tdx + 2, y() + tdy, tW - 4, tW);
	      fl_rectf(x() + tdx + 1, y() + tdy + 1, tW - 2, tW - 2);
	      fl_rectf(x() + tdx, y() + tdy + 2, tW, tW - 4);
	      break;

	    case 5 :
	    case 4 :
	    case 3 :
	      fl_rectf(x() + tdx + 1, y() + tdy, tW - 2, tW);
	      fl_rectf(x() + tdx, y() + tdy + 1, tW, tW - 2);
	      break;

	    case 2 :
	    case 1 :
	      fl_rectf(x() + tdx, y() + tdy, tW, tW);
	      break;
	  }

	  if (Fl::scheme() && !strcmp(Fl::scheme(), "gtk+")) {
	    fl_color(fl_color_average(FL_WHITE, FL_SELECTION_COLOR, 0.5));
	    fl_arc(x() + tdx, y() + tdy, tW + 1, tW + 1, 60.0, 180.0);
	  }
	}
        break;
      default :
        draw_box(down_box(), x()+dx, y()+dy, W, W, col);
        break;
    }
  } else {
    // if down_box() is zero, draw light button style:
    int hh = h()-2*dy - 2;
    int ww = W/2+1;
    int xx = dx;
    if (w()<ww+2*xx) xx = (w()-ww)/2;
    if (Fl::scheme() && !strcmp(Fl::scheme(), "plastic")) {
      col = active_r() ? selection_color() : fl_inactive(selection_color());
      fl_color(value() ? col : fl_color_average(col, FL_BLACK, 0.5f));
      fl_pie(x()+xx, y()+dy+1, ww, hh, 0, 360);
    } else {
      draw_box(FL_THIN_DOWN_BOX, x()+xx, y()+dy+1, ww, hh, col);
    }
    dx = (ww + 2 * dx - W) / 2;
  }
  draw_label(x()+W+2*dx, y(), w()-W-2*dx, h());
  if (Fl::focus() == this) draw_focus();
}

int Fl_Light_Button::handle(int event) {
  switch (event) {
  case FL_RELEASE:
    if (box()) redraw();
  default:
    return Fl_Button::handle(event);
  }
}

Fl_Light_Button::Fl_Light_Button(int X, int Y, int W, int H, const char* l)
: Fl_Button(X, Y, W, H, l) {
  type(FL_TOGGLE_BUTTON);
  selection_color(FL_YELLOW);
  align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE);
}

//
// End of "$Id: Fl_Light_Button.cxx 5721 2007-02-27 19:23:24Z matt $".
//
