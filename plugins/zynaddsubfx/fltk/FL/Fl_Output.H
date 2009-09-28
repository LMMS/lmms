//
// "$Id: Fl_Output.H 6898 2009-09-23 20:43:27Z matt $"
//
// Output header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Output widget . */

#ifndef Fl_Output_H
#define Fl_Output_H

#include "Fl_Input.H"
/**
  This widget displays a piece of text.  When you set the value()
  , Fl_Output does a strcpy() to it's own storage,
  which is useful for program-generated values.  The user may select
  portions of the text using the mouse and paste the contents into other
  fields or programs.
  <P align=CENTER>\image html text.gif</P> 
  \image latex text.eps "Fl_Output" width=8cm
  <P>There is a single subclass, 
  Fl_Multiline_Output, which allows you to display multiple lines of
  text. Fl_Multiline_Output does not provide scroll bars. If a more 
  complete text editing widget is needed, use Fl_Text_Display instead.</P>
  <P>The text may contain any characters except \\0, and will correctly
  display anything, using ^X notation for unprintable control characters
  and \\nnn notation for unprintable characters with the high bit set. It
  assumes the font can draw any characters in the ISO-Latin1 character
  set.
*/
class Fl_Output : public Fl_Input {
public:
  /**
    Creates a new Fl_Output widget using the given position,
    size, and label string. The default boxtype is FL_DOWN_BOX.
    <P>Inherited destrucor destroys the widget and any value associated with it.
  */
    Fl_Output(int X,int Y,int W,int H, const char *l = 0)
    : Fl_Input(X, Y, W, H, l) {type(FL_NORMAL_OUTPUT);}
};

#endif 

//
// End of "$Id: Fl_Output.H 6898 2009-09-23 20:43:27Z matt $".
//
