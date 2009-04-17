//
// "$Id: Fl_Repeat_Button.cxx 5219 2006-06-21 06:52:10Z matt $"
//
// Repeat button widget for the Fast Light Tool Kit (FLTK).
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
#include <FL/Fl_Repeat_Button.H>

#define INITIALREPEAT .5
#define REPEAT .1

void Fl_Repeat_Button::repeat_callback(void *v) {
  Fl_Button *b = (Fl_Button*)v;
  Fl::add_timeout(REPEAT,repeat_callback,b);
  b->do_callback();
}

int Fl_Repeat_Button::handle(int event) {
  int newval;
  switch (event) {
  case FL_HIDE:
  case FL_DEACTIVATE:
  case FL_RELEASE:
    newval = 0; goto J1;
  case FL_PUSH:
  case FL_DRAG:
    if (Fl::visible_focus()) Fl::focus(this);
    newval = Fl::event_inside(this);
  J1:
    if (!active()) 
      newval = 0;
    if (value(newval)) {
      if (newval) {
	Fl::add_timeout(INITIALREPEAT,repeat_callback,this);
	do_callback();
      } else {
	Fl::remove_timeout(repeat_callback,this);
      }
    }
    return 1;
  default:
    return Fl_Button::handle(event);
  }
}

//
// End of "$Id: Fl_Repeat_Button.cxx 5219 2006-06-21 06:52:10Z matt $".
//
