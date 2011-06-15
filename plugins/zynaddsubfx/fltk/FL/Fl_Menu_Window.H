//
// "$Id: Fl_Menu_Window.H 7903 2010-11-28 21:06:39Z matt $"
//
// Menu window header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

/* \file
   Fl_Menu_Window widget . */

#ifndef Fl_Menu_Window_H
#define Fl_Menu_Window_H

#include "Fl_Single_Window.H"

/**
  The Fl_Menu_Window widget is a window type used for menus. By
  default the window is drawn in the hardware overlay planes if they are
  available so that the menu don't force the rest of the window to
  redraw.
*/
class FL_EXPORT Fl_Menu_Window : public Fl_Single_Window {
public:
  void show();
  void erase();
  void flush();
  void hide();
  /** Tells if hardware overlay mode is set */
  unsigned int overlay() {return !(flags()&NO_OVERLAY);}
  /** Tells FLTK to use hardware overlay planes if they are available.  */
  void set_overlay() {clear_flag(NO_OVERLAY);}
  /** Tells FLTK to use normal drawing planes instead of overlay planes.
      This is usually necessary if your menu contains multi-color pixmaps. */
  void clear_overlay() {set_flag(NO_OVERLAY);}
  ~Fl_Menu_Window();
  /** Creates a new Fl_Menu_Window widget using the given size, and label string. */
  Fl_Menu_Window(int W, int H, const char *l = 0)
    : Fl_Single_Window(W,H,l) { image(0); }
  /** Creates a new Fl_Menu_Window widget using the given position, size, and label string. */
  Fl_Menu_Window(int X, int Y, int W, int H, const char *l = 0)
    : Fl_Single_Window(X,Y,W,H,l) { image(0); }
};

#endif

//
// End of "$Id: Fl_Menu_Window.H 7903 2010-11-28 21:06:39Z matt $".
//
