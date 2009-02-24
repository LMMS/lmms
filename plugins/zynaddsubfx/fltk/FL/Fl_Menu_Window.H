//
// "$Id: Fl_Menu_Window.H 4288 2005-04-16 00:13:17Z mike $"
//
// Menu window header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Menu_Window_H
#define Fl_Menu_Window_H

#include "Fl_Single_Window.H"

class FL_EXPORT Fl_Menu_Window : public Fl_Single_Window {
  enum {NO_OVERLAY = 128};
public:
  void show();
  void erase();
  void flush();
  void hide();
  int overlay() {return !(flags()&NO_OVERLAY);}
  void set_overlay() {clear_flag(NO_OVERLAY);}
  void clear_overlay() {set_flag(NO_OVERLAY);}
  ~Fl_Menu_Window();
  Fl_Menu_Window(int W, int H, const char *l = 0)
    : Fl_Single_Window(W,H,l) { image(0); }
  Fl_Menu_Window(int X, int Y, int W, int H, const char *l = 0)
    : Fl_Single_Window(X,Y,W,H,l) { image(0); }
};

#endif

//
// End of "$Id: Fl_Menu_Window.H 4288 2005-04-16 00:13:17Z mike $".
//
