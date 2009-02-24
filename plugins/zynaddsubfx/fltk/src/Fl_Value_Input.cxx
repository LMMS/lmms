//
// "$Id: Fl_Value_Input.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Value input widget for the Fast Light Tool Kit (FLTK).
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

// FLTK widget for drag-adjusting a floating point value.
// Warning: this works by making a child Fl_Input object, even
// though this object is *not* an Fl_Group.  May be a kludge?

#include <FL/Fl.H>
#include <FL/Fl_Value_Input.H>
#include <FL/Fl_Group.H>
#include <stdlib.h>
#include <FL/math.h>


void Fl_Value_Input::input_cb(Fl_Widget*, void* v) {
  Fl_Value_Input& t = *(Fl_Value_Input*)v;
  double nv;
  if ((t.step() - floor(t.step()))>0.0 || t.step() == 0.0) nv = strtod(t.input.value(), 0);
  else nv = strtol(t.input.value(), 0, 0);
  if (nv != t.value() || t.when() & FL_WHEN_NOT_CHANGED) {
    t.set_value(nv);
    t.set_changed();
    if (t.when()) t.do_callback();
  }
}

void Fl_Value_Input::draw() {
  if (damage()&~FL_DAMAGE_CHILD) input.clear_damage(FL_DAMAGE_ALL);
  input.box(box());
  input.color(color(), selection_color());
  input.draw();
  input.clear_damage();
}

void Fl_Value_Input::resize(int X, int Y, int W, int H) {
  Fl_Valuator::resize(X, Y, W, H);
  input.resize(X, Y, W, H);
}

void Fl_Value_Input::value_damage() {
  char buf[128];
  format(buf);
  input.value(buf);
  input.mark(input.position()); // turn off selection highlight
}

int Fl_Value_Input::handle(int event) {
  double v;
  int delta;
  int mx = Fl::event_x_root();
  static int ix, drag;
  input.when(when());
  switch (event) {
  case FL_PUSH:
    if (!step()) goto DEFAULT;
    ix = mx;
    drag = Fl::event_button();
    handle_push();
    return 1;
  case FL_DRAG:
    if (!step()) goto DEFAULT;
    delta = mx-ix;
    if (delta > 5) delta -= 5;
    else if (delta < -5) delta += 5;
    else delta = 0;
    switch (drag) {
    case 3: v = increment(previous_value(), delta*100); break;
    case 2: v = increment(previous_value(), delta*10); break;
    default:v = increment(previous_value(), delta); break;
    }
    v = round(v);
    handle_drag(soft()?softclamp(v):clamp(v));;
    return 1;
  case FL_RELEASE:
    if (!step()) goto DEFAULT;
    if (value() != previous_value() || !Fl::event_is_click())
      handle_release();
    else {
      input.handle(FL_PUSH);
      input.handle(FL_RELEASE);
    }
    return 1;
  case FL_FOCUS:
    return input.take_focus();
  default:
  DEFAULT:
    input.type(((step() - floor(step()))>0.0 || step() == 0.0) ? FL_FLOAT_INPUT : FL_INT_INPUT);
    return input.handle(event);
  }
}

Fl_Value_Input::Fl_Value_Input(int X, int Y, int W, int H, const char* l)
: Fl_Valuator(X, Y, W, H, l), input(X, Y, W, H, 0) {
  soft_ = 0;
  if (input.parent())  // defeat automatic-add
    ((Fl_Group*)input.parent())->remove(input);
  input.parent((Fl_Group *)this); // kludge!
  input.callback(input_cb, this);
  input.when(FL_WHEN_CHANGED);
  box(input.box());
  color(input.color());
  selection_color(input.selection_color());
  align(FL_ALIGN_LEFT);
  value_damage();
}

//
// End of "$Id: Fl_Value_Input.cxx 5190 2006-06-09 16:16:34Z mike $".
//
