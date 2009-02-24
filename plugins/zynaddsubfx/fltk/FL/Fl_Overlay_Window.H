//
// "$Id: Fl_Overlay_Window.H 4288 2005-04-16 00:13:17Z mike $"
//
// Overlay window header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Overlay_Window_H
#define Fl_Overlay_Window_H

#include "Fl_Double_Window.H"

class FL_EXPORT Fl_Overlay_Window : public Fl_Double_Window {
  friend class _Fl_Overlay;
  virtual void draw_overlay() = 0;
  Fl_Window *overlay_;
public:
  void show();
  void flush();
  void hide();
  void resize(int,int,int,int);
  ~Fl_Overlay_Window();
  int can_do_overlay();
  void redraw_overlay();
  Fl_Overlay_Window(int W, int H, const char *l=0)
    : Fl_Double_Window(W,H,l) {overlay_ = 0; force_doublebuffering_=1; image(0); }
  Fl_Overlay_Window(int X, int Y, int W, int H, const char *l=0)
    : Fl_Double_Window(X,Y,W,H,l) {overlay_ = 0; force_doublebuffering_=1; image(0); }
  void show(int a, char **b) {Fl_Double_Window::show(a,b);}
};

#endif

//
// End of "$Id: Fl_Overlay_Window.H 4288 2005-04-16 00:13:17Z mike $".
//
