//
// "$Id: Fl_Multiline_Output.H 8726 2011-05-23 18:32:47Z AlbrechtS $"
//
// Multi line output header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Multiline_Output widget . */

#ifndef Fl_Multiline_Output_H
#define Fl_Multiline_Output_H

#include "Fl_Output.H"

/**
  This widget is a subclass of Fl_Output that displays multiple
  lines of text. It also displays tab characters as whitespace to the
  next column.

  Note that this widget does not support scrollbars, or per-character
  color control.

  If you are presenting large amounts of read-only text 
  and need scrollbars, or full color control of characters,
  then use Fl_Text_Display. If you want to display HTML text,
  use Fl_Help_View.
*/
class FL_EXPORT Fl_Multiline_Output : public Fl_Output {
public:

  /**
    Creates a new Fl_Multiline_Output widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.

    Inherited destructor destroys the widget and any value associated with it.
  */
#if defined(FL_DLL)	// implementation in src/Fl_Input.cxx
  Fl_Multiline_Output(int X,int Y,int W,int H,const char *l = 0);
#else
  Fl_Multiline_Output(int X,int Y,int W,int H,const char *l = 0)
    : Fl_Output(X,Y,W,H,l) {type(FL_MULTILINE_OUTPUT);}
#endif
};

#endif

//
// End of "$Id: Fl_Multiline_Output.H 8726 2011-05-23 18:32:47Z AlbrechtS $".
//
