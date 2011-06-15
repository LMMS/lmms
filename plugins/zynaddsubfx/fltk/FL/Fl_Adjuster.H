//
// "$Id: Fl_Adjuster.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Adjuster widget header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Adjuster widget . */

// 3-button "slider", made for Nuke

#ifndef Fl_Adjuster_H
#define Fl_Adjuster_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

/**
  The Fl_Adjuster widget was stolen from Prisms, and has proven
  to be very useful for values that need a large dynamic range.
  \image html adjuster1.png 
  \image latex adjuster1.png "Fl_Adjuster" width=4cm
  <P>When you  press a button and drag to the right the value increases.
  When you drag  to the left it decreases.  The largest button adjusts by
  100 *  step(), the next by 10 * step() and that
  smallest button  by step().  Clicking on the buttons
  increments by 10 times the  amount dragging by a pixel does. Shift +
  click decrements by 10 times  the amount.
*/
class FL_EXPORT Fl_Adjuster : public Fl_Valuator {
  int drag;
  int ix;
  int soft_;
protected:
  void draw();
  int handle(int);
  void value_damage();
public:
  Fl_Adjuster(int X,int Y,int W,int H,const char *l=0);
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  void soft(int s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag the value outside
    the range.  If they drag the value to one of the ends, let go, then
    grab again and continue to drag, they can get to any value.  Default is
    one.
  */
  int soft() const {return soft_;}
};

#endif

//
// End of "$Id: Fl_Adjuster.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
