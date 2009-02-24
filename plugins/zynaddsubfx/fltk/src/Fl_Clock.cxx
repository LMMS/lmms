//
// "$Id: Fl_Clock.cxx 5472 2006-09-20 03:03:14Z mike $"
//
// Clock widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Clock.H>
#include <FL/fl_draw.H>
#include <math.h>
#include <time.h>
#ifndef WIN32
#  include <sys/time.h>
#endif /* !WIN32 */

// Original clock display written by Paul Haeberli at SGI.
// Modifications by Mark Overmars for Forms
// Further changes by Bill Spitzak for fltk

const float hourhand[4][2] = {{-0.5f, 0}, {0, 1.5f}, {0.5f, 0}, {0, -7.0f}};
const float  minhand[4][2] = {{-0.5f, 0}, {0, 1.5f}, {0.5f, 0}, {0, -11.5f}};
const float  sechand[4][2] = {{-0.1f, 0}, {0, 2.0f}, {0.1f, 0}, {0, -11.5f}};

static void drawhand(double ang,const float v[][2],Fl_Color fill,Fl_Color line)
{
  fl_push_matrix();
  fl_rotate(ang);
  fl_color(fill); fl_begin_polygon();
  int i; for (i=0; i<4; i++) fl_vertex(v[i][0],v[i][1]); fl_end_polygon();
  fl_color(line); fl_begin_loop();
  for (i=0; i<4; i++) fl_vertex(v[i][0],v[i][1]); fl_end_loop();
  fl_pop_matrix();
}

void Fl_Clock_Output::drawhands(Fl_Color fill, Fl_Color line) {
  if (!active_r()) {
    fill = fl_inactive(fill);
    line = fl_inactive(line);
  }
  drawhand(-360*(hour()+minute()/60.0)/12, hourhand, fill, line);
  drawhand(-360*(minute()+second()/60.0)/60, minhand, fill, line);
  drawhand(-360*(second()/60.0), sechand, fill, line);
}

static void rect(double x, double y, double w, double h) {
  double r = x+w;
  double t = y+h;
  fl_begin_polygon();
  fl_vertex(x, y);
  fl_vertex(r, y);
  fl_vertex(r, t);
  fl_vertex(x, t);
  fl_end_polygon();
}

void Fl_Clock_Output::draw(int X, int Y, int W, int H) {
  Fl_Color box_color = type()==FL_ROUND_CLOCK ? FL_GRAY : color();
  Fl_Color shadow_color = fl_color_average(box_color, FL_BLACK, 0.5);
  draw_box(box(), X, Y, W, H, box_color);
  fl_push_matrix();
  fl_translate(X+W/2.0-.5, Y+H/2.0-.5);
  fl_scale((W-1)/28.0, (H-1)/28.0);
  if (type() == FL_ROUND_CLOCK) {
    fl_color(active_r() ? color() : fl_inactive(color()));
    fl_begin_polygon(); fl_circle(0,0,14); fl_end_polygon();
    fl_color(active_r() ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
    fl_begin_loop(); fl_circle(0,0,14); fl_end_loop();
  }
  // draw the shadows:
  fl_push_matrix();
  fl_translate(0.60, 0.60);
  drawhands(shadow_color, shadow_color);
  fl_pop_matrix();
  // draw the tick marks:
  fl_push_matrix();
  fl_color(active_r() ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
  for (int i=0; i<12; i++) {
    if (i==6) rect(-0.5, 9, 1, 2);
    else if (i==3 || i==0 || i== 9) rect(-0.5, 9.5, 1, 1);
    else rect(-0.25, 9.5, .5, 1);
    fl_rotate(-30);
  }
  fl_pop_matrix();
  // draw the hands:
  drawhands(selection_color(), FL_FOREGROUND_COLOR); // color was 54
  fl_pop_matrix();
}

void Fl_Clock_Output::draw() {
  draw(x(), y(), w(), h());
  draw_label();
}

void Fl_Clock_Output::value(int H, int m, int s) {
  if (H!=hour_ || m!=minute_ || s!=second_) {
    hour_ = H; minute_ = m; second_ = s;
    value_ = (H * 60 + m) * 60 + s;
    damage(FL_DAMAGE_CHILD);
  }
}

void Fl_Clock_Output::value(ulong v) {
  value_ = v;
  struct tm *timeofday;
  // Some platforms, notably Windows, now use a 64-bit time_t value...
  time_t vv = (time_t)v;
  timeofday = localtime(&vv);
  value(timeofday->tm_hour, timeofday->tm_min, timeofday->tm_sec);
}

Fl_Clock_Output::Fl_Clock_Output(int X, int Y, int W, int H, const char *l)
: Fl_Widget(X, Y, W, H, l) {
  box(FL_UP_BOX);
  selection_color(fl_gray_ramp(5));
  align(FL_ALIGN_BOTTOM);
  hour_ = 0;
  minute_ = 0;
  second_ = 0;
  value_ = 0;
}

////////////////////////////////////////////////////////////////

Fl_Clock::Fl_Clock(int X, int Y, int W, int H, const char *l)
  : Fl_Clock_Output(X, Y, W, H, l) {}

Fl_Clock::Fl_Clock(uchar t, int X, int Y, int W, int H, const char *l)
  : Fl_Clock_Output(X, Y, W, H, l) {
  type(t);
  box(t==FL_ROUND_CLOCK ? FL_NO_BOX : FL_UP_BOX);
}

static void tick(void *v) {
  ((Fl_Clock*)v)->value(time(0));
  Fl::add_timeout(1.0, tick, v);
}

int Fl_Clock::handle(int event) {
  switch (event) {
  case FL_SHOW:
    tick(this);
    break;
  case FL_HIDE:
    Fl::remove_timeout(tick, this);
    break;
  }
  return Fl_Clock_Output::handle(event);
}
  
Fl_Clock::~Fl_Clock() {
  Fl::remove_timeout(tick, this);
}

//
// End of "$Id: Fl_Clock.cxx 5472 2006-09-20 03:03:14Z mike $".
//
