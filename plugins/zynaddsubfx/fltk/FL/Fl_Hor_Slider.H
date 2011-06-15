//
// "$Id: Fl_Hor_Slider.H 8726 2011-05-23 18:32:47Z AlbrechtS $"
//
// Horizontal slider header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Hor_Slider widget . */

#ifndef Fl_Hor_Slider_H
#define Fl_Hor_Slider_H

#include "Fl_Slider.H"

/** Horizontal Slider class.

  \see class Fl_Slider.
*/
class FL_EXPORT Fl_Hor_Slider : public Fl_Slider {
public:

  /**
    Creates a new Fl_Hor_Slider widget using the given position,
    size, and label string.
  */

#if defined(FL_DLL)	// implementation in src/Fl_Slider.cxx
  Fl_Hor_Slider(int X,int Y,int W,int H,const char *l=0);
#else
  Fl_Hor_Slider(int X,int Y,int W,int H,const char *l=0)
    : Fl_Slider(X,Y,W,H,l) { type(FL_HOR_SLIDER); }
#endif
};

#endif

//
// End of "$Id: Fl_Hor_Slider.H 8726 2011-05-23 18:32:47Z AlbrechtS $".
//
