//
// "$Id: Fl_Window.cxx 5251 2006-06-28 10:23:33Z matt $"
//
// Window widget class for the Fast Light Tool Kit (FLTK).
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

// The Fl_Window is a window in the fltk library.
// This is the system-independent portions.  The huge amount of 
// crap you need to do to communicate with X is in Fl_x.cxx, the
// equivalent (but totally different) crap for MSWindows is in Fl_win32.cxx

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <stdlib.h>
#include "flstring.h"

#ifdef __APPLE_QUARTZ__
#include <FL/fl_draw.H>
#endif

void Fl_Window::_Fl_Window() {
  type(FL_WINDOW);
  box(FL_FLAT_BOX);
  if (Fl::scheme_bg_) {
    labeltype(FL_NORMAL_LABEL);
    align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    image(Fl::scheme_bg_);
  } else {
    labeltype(FL_NO_LABEL);
  }
  i = 0;
  xclass_ = 0;
  icon_ = 0;
  iconlabel_ = 0;
  resizable(0);
  size_range_set = 0;
  minw = maxw = minh = maxh = 0;
  callback((Fl_Callback*)default_callback);
}

Fl_Window::Fl_Window(int X,int Y,int W, int H, const char *l)
: Fl_Group(X, Y, W, H, l) {
  cursor_default = FL_CURSOR_DEFAULT;
  cursor_fg      = FL_BLACK;
  cursor_bg      = FL_WHITE;

  _Fl_Window();
  set_flag(FL_FORCE_POSITION);
}

Fl_Window::Fl_Window(int W, int H, const char *l)
// fix common user error of a missing end() with current(0):
  : Fl_Group((Fl_Group::current(0),0), 0, W, H, l) {
  cursor_default = FL_CURSOR_DEFAULT;
  cursor_fg      = FL_BLACK;
  cursor_bg      = FL_WHITE;

  _Fl_Window();
  clear_visible();
}

Fl_Window *Fl_Widget::window() const {
  for (Fl_Widget *o = parent(); o; o = o->parent())
    if (o->type() >= FL_WINDOW) return (Fl_Window*)o;
  return 0;
}

int Fl_Window::x_root() const {
  Fl_Window *p = window();
  if (p) return p->x_root() + x();
  return x();
}

int Fl_Window::y_root() const {
  Fl_Window *p = window();
  if (p) return p->y_root() + y();
  return y();
}

void Fl_Window::draw() {
  const char *savelabel = label();
  int saveflags = flags();
  int savex = x(); x(0);
  int savey = y(); y(0);
  // Make sure we don't draw the window title in the window background...
  clear_flag(COPIED_LABEL); // do not free copied labels!
  Fl_Widget::label(0);
  Fl_Group::draw();
#ifdef __APPLE_QUARTZ__
  if (!parent() && resizable() && (!size_range_set || minh!=maxh || minw!=maxw)) {
    int dx = Fl::box_dw(box())-Fl::box_dx(box());
    int dy = Fl::box_dh(box())-Fl::box_dy(box());
    if (dx<=0) dx = 1;
    if (dy<=0) dy = 1;
    int x1 = w()-dx-1, x2 = x1, y1 = h()-dx-1, y2 = y1;
    Fl_Color c[4] = {
      color(),
      fl_color_average(color(), FL_WHITE, 0.7f),
      fl_color_average(color(), FL_BLACK, 0.6f),
      fl_color_average(color(), FL_BLACK, 0.8f),
    };
    int i;
    for (i=dx; i<12; i++) {
      fl_color(c[i&3]);
      fl_line(x1--, y1, x2, y2--);
    }
  }
#endif
  // Restore the label...
  Fl_Widget::label(savelabel);
  set_flag(saveflags);
  y(savey);
  x(savex);
}

void Fl_Window::label(const char *name) {
  label(name, iconlabel());
}

void Fl_Window::copy_label(const char *a) {
  if (flags() & COPIED_LABEL) {
    free((void *)label());
    clear_flag(COPIED_LABEL);
  }
  if (a) a = strdup(a);
  label(a, iconlabel());
  set_flag(COPIED_LABEL);
}


void Fl_Window::iconlabel(const char *iname) {
  uchar saveflags = flags();
  label(label(), iname);
  set_flag(saveflags);
}

// the Fl::atclose pointer is provided for back compatability.  You
// can now just change the callback for the window instead.

void Fl::default_atclose(Fl_Window* window, void* v) {
  window->hide();
  Fl_Widget::default_callback(window, v); // put on Fl::read_queue()
}

void (*Fl::atclose)(Fl_Window*, void*) = default_atclose;

void Fl_Window::default_callback(Fl_Window* win, void* v) {
  Fl::atclose(win, v);
}

Fl_Window *Fl_Window::current() {
  return current_;
}


//
// End of "$Id: Fl_Window.cxx 5251 2006-06-28 10:23:33Z matt $".
//
