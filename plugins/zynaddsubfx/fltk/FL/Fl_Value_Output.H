//
// "$Id: Fl_Value_Output.H 7989 2010-12-09 12:19:03Z greg.ercolano $"
//
// Value output header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Value_Output widget . */

#ifndef Fl_Value_Output_H
#define Fl_Value_Output_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

/**
  The Fl_Value_Output widget displays a floating point value.
  If step() is not zero, the user can adjust the value by
  dragging the mouse left and right.  The left button moves one step()
  per pixel, the middle by 10 * step(), and the right button by 
  100 * step().
  <P>This is much lighter-weight than 
  Fl_Value_Input because it contains no text editing code or
  character buffer. </P>
  <P ALIGN=CENTER>\image html Fl_Value_Output.png 
  \image latex  Fl_Value_Output.png "Fl_Value_Output" width=4cm
*/
class FL_EXPORT Fl_Value_Output : public Fl_Valuator {
  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  uchar soft_;
  Fl_Color textcolor_;

protected:
  void draw();

public:
  int handle(int);
  Fl_Value_Output(int x,int y,int w,int h,const char *l=0);

  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  void soft(uchar s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  uchar soft() const {return soft_;}

  /**    Gets the typeface of the text in the value box.  */
  Fl_Font textfont() const {return textfont_;}
  /**    Sets the typeface of the text in the value box.  */
  void textfont(Fl_Font s) {textfont_ = s;}
  /**    Gets the size of the text in the value box.  */
  Fl_Fontsize textsize() const {return textsize_;}
  void textsize(Fl_Fontsize s) {textsize_ = s;}
  /**    Sets the color of the text in the value box.  */
  Fl_Color textcolor() const {return textcolor_;}
  /**    Gets the color of the text in the value box.  */
  void textcolor(Fl_Color s) {textcolor_ = s;}
};

#endif

//
// End of "$Id: Fl_Value_Output.H 7989 2010-12-09 12:19:03Z greg.ercolano $".
//
