//
// "$Id: Fl_Tabs.H 5326 2006-08-17 13:43:07Z matt $"
//
// Tab header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Tabs_H
#define Fl_Tabs_H

#include "Fl_Group.H"

class FL_EXPORT Fl_Tabs : public Fl_Group {
  Fl_Widget *value_;
  Fl_Widget *push_;
  int tab_positions(int*, int*);
  int tab_height();
  void draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int sel=0);
protected:
  void redraw_tabs();
  void draw();

public:
  int handle(int);
  Fl_Widget *value();
  int value(Fl_Widget *);
  Fl_Widget *push() const {return push_;}
  int push(Fl_Widget *);
  Fl_Tabs(int,int,int,int,const char * = 0);
  Fl_Widget *which(int event_x, int event_y);
};

#endif

//
// End of "$Id: Fl_Tabs.H 5326 2006-08-17 13:43:07Z matt $".
//
