//
// "$Id: Fl_Multiline_Input.H 6889 2009-09-19 22:09:00Z greg.ercolano $"
//
// Multiline input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2009 by Bill Spitzak and others.
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
   Fl_Multiline_Input widget . */


#ifndef Fl_Multiline_Input_H
#define Fl_Multiline_Input_H

#include "Fl_Input.H"

/**
  This input field displays '\n' characters as new lines rather than ^J,
  and accepts the Return, Tab, and up and down arrow keys.  This is for
  editing multiline text.
  <P>
  This is far from the nirvana of text editors, and is probably only
  good for small bits of text, 10 lines at most. Note that this widget
  does not support scrollbars or per-character color control.
  <P>
  If you are presenting large amounts of text and need scrollbars
  or full color control of characters, you probably want Fl_Text_Editor
  instead.
  <P>
*/
class Fl_Multiline_Input : public Fl_Input {
public:
  /**
    Creates a new Fl_Multiline_Input widget using the given
    position, size, and label string. The default boxtype is FL_DOWN_BOX.
    <P>Inherited destructor destroys the widget and any value associated with it.
  */
  Fl_Multiline_Input(int X,int Y,int W,int H,const char *l = 0)
    : Fl_Input(X,Y,W,H,l) {type(FL_MULTILINE_INPUT);}
};

#endif

//
// End of "$Id: Fl_Multiline_Input.H 6889 2009-09-19 22:09:00Z greg.ercolano $".
//
