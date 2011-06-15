//
// "$Id: Fl_Check_Button.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Check button header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Check_Button widget . */

#ifndef Fl_Check_Button_H
#define Fl_Check_Button_H

#include "Fl_Light_Button.H"

/**
  \class Fl_Check_Button
  \brief A button with an "checkmark" to show its status.

  \image html Fl_Check_Button.png
  \image latex Fl_Check_Button.png  "Fl_Check_Button" width=4cm
  Buttons generate callbacks when they are clicked by the user. You control
  exactly when and how by changing the values for type() and when().

  The Fl_Check_Button subclass displays its "ON" state by showing a "checkmark"
  rather than drawing itself pushed in.
  
  \todo Refactor Fl_Check_Button doxygen comments (add color() info etc?)
  \todo Generate Fl_Check_Button.gif with visible checkmark.
 */
class FL_EXPORT Fl_Check_Button : public Fl_Light_Button {
public:
  Fl_Check_Button(int X, int Y, int W, int H, const char *L = 0);
};

#endif

//
// End of "$Id: Fl_Check_Button.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
