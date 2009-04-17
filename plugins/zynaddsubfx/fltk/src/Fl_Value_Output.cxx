//
// "$Id: Fl_Value_Output.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Value output widget for the Fast Light Tool Kit (FLTK).
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

// Fltk widget for drag-adjusting a floating point value.
// This is much lighter than Fl_Value_Input because it has no text editor
// If step() is zero then it can be used to display a floating-point value

#include <FL/Fl.H>
#include <FL/Fl_Value_Output.H>
#include <FL/fl_draw.H>

void Fl_Value_Output::draw() {
  Fl_Boxtype b = box() ? box() : FL_DOWN_BOX;
  int X = x()+Fl::box_dx(b);
  int Y = y()+Fl::box_dy(b);
  int W = w()-Fl::box_dw(b);
  int H = h()-Fl::box_dh(b);
  if (damage()&~FL_DAMAGE_CHILD)
    draw_box(b, color());
  else {
    fl_color(color());
    fl_rectf(X, Y, W, H);
  }
  char buf[128];
  format(buf);
  fl_color(active_r() ? textcolor() : fl_inactive(textcolor()));
  fl_font(textfont(), textsize());
  fl_draw(buf,X,Y,W,H,FL_ALIGN_LEFT);
}

int Fl_Value_Output::handle(int event) {
  if (!step()) return 0;
  double v;
  int delta;
  int mx = Fl::event_x();
  static int ix, drag;
  switch (event) {
  case FL_PUSH:
    ix = mx;
    drag = Fl::event_button();
    handle_push();
    return 1;
  case FL_DRAG:
    delta = Fl::event_x()-ix;
    if (delta > 5) delta -= 5;
    else if (delta < -5) delta += 5;
    else delta = 0;
    switch (drag) {
    case 3: v = increment(previous_value(),delta*100); break;
    case 2: v = increment(previous_value(),delta*10); break;
    default:v = increment(previous_value(),delta); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));;
    return 1;
  case FL_RELEASE:
    handle_release();
    return 1;
  case FL_ENTER :
  case FL_LEAVE :
    return 1;
  default:
    return 0;
  }
}

Fl_Value_Output::Fl_Value_Output(int X, int Y, int W, int H,const char *l)
: Fl_Valuator(X,Y,W,H,l) {
  box(FL_NO_BOX);
  align(FL_ALIGN_LEFT);
  textfont_ = FL_HELVETICA;
  textsize_ = (uchar)FL_NORMAL_SIZE;
  textcolor_ = FL_FOREGROUND_COLOR;
  soft_ = 0;
}

//
// End of "$Id: Fl_Value_Output.cxx 5190 2006-06-09 16:16:34Z mike $".
//
