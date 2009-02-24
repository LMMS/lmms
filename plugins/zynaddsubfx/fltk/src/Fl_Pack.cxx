//
// "$Id: Fl_Pack.cxx 5981 2007-11-19 16:21:19Z matt $"
//
// Packing widget for the Fast Light Tool Kit (FLTK).
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

// Based on code by Curtis Edwards
// Group that compresses all it's children together and resizes to surround
// them on each redraw (only if box() is zero)
// Bugs: ?

#include <FL/Fl.H>
#include <FL/Fl_Pack.H>
#include <FL/fl_draw.H>

Fl_Pack::Fl_Pack(int X, int Y, int W, int H,const char *l)
: Fl_Group(X, Y, W, H, l) {
  resizable(0);
  spacing_ = 0;
  // type(VERTICAL); // already set like this
}

void Fl_Pack::draw() {
  int tx = x()+Fl::box_dx(box());
  int ty = y()+Fl::box_dy(box());
  int tw = w()-Fl::box_dw(box());
  int th = h()-Fl::box_dh(box());
  int rw, rh;
  int current_position = horizontal() ? tx : ty;
  int maximum_position = current_position;
  uchar d = damage();
  Fl_Widget*const* a = array();
  if (horizontal()) {
    rw = -spacing_;
    rh = th;
    
    for (int i = children(); i--;)
      if (child(i)->visible()) {
        if (child(i) != this->resizable()) rw += child(i)->w();
        rw += spacing_;
      }
  } else {
    rw = tw;
    rh = -spacing_;
    
    for (int i = children(); i--;)
      if (child(i)->visible()) {
        if (child(i) != this->resizable()) rh += child(i)->h();
        rh += spacing_;
      }
  }
  for (int i = children(); i--;) {
    Fl_Widget* o = *a++;
    if (o->visible()) {
      int X,Y,W,H;
      if (horizontal()) {
        X = current_position;
        W = o->w();
        Y = ty;
        H = th;
      } else {
        X = tx;
        W = tw;
        Y = current_position;
        H = o->h();
      }
      // Last child, if resizable, takes all remaining room
      if(i == 0 && o == this->resizable()) {
        if(horizontal())
          W = tw - rw;
        else
          H = th - rh;
      }
      if (spacing_ && current_position>maximum_position && box() &&
        (X != o->x() || Y != o->y() || d&FL_DAMAGE_ALL)) {
        fl_color(color());
        if (horizontal())
          fl_rectf(maximum_position, ty, spacing_, th);
        else
          fl_rectf(tx, maximum_position, tw, spacing_);
      }
      if (X != o->x() || Y != o->y() || W != o->w() || H != o->h()) {
        o->resize(X,Y,W,H);
        o->clear_damage(FL_DAMAGE_ALL);
      }
      if (d&FL_DAMAGE_ALL) {
        draw_child(*o);
        draw_outside_label(*o);
      } else update_child(*o);
      // child's draw() can change it's size, so use new size:
      current_position += (horizontal() ? o->w() : o->h());
      if (current_position > maximum_position)
        maximum_position = current_position;
      current_position += spacing_;
    }
  }
  
  if (horizontal()) {
    if (maximum_position < tx+tw && box()) {
      fl_color(color());
      fl_rectf(maximum_position, ty, tx+tw-maximum_position, th);
    }
    tw = maximum_position-tx;
  } else {
    if (maximum_position < ty+th && box()) {
      fl_color(color());
      fl_rectf(tx, maximum_position, tw, ty+th-maximum_position);
    }
    th = maximum_position-ty;
  }
  
  tw += Fl::box_dw(box()); if (tw <= 0) tw = 1;
  th += Fl::box_dh(box()); if (th <= 0) th = 1;
  if (tw != w() || th != h()) {
    Fl_Widget::resize(x(),y(),tw,th);
    d = FL_DAMAGE_ALL;
  }
  if (d&FL_DAMAGE_ALL) {
    draw_box();
    draw_label();
  }
}

//
// End of "$Id: Fl_Pack.cxx 5981 2007-11-19 16:21:19Z matt $".
//
