//
// "$Id: Fl_Roller.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Roller header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Roller widget . */

#ifndef Fl_Roller_H
#define Fl_Roller_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

/**
  The Fl_Roller widget is a "dolly" control commonly used to
  move 3D objects.
  <P ALIGN=CENTER>\image html Fl_Roller.png 
  \image latex Fl_Roller.png "Fl_Roller" width=4cm
*/
class FL_EXPORT Fl_Roller : public Fl_Valuator {
protected:
  void draw();
public:
  int handle(int);
  Fl_Roller(int X,int Y,int W,int H,const char* L=0);
};

#endif

//
// End of "$Id: Fl_Roller.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
