//
// "$Id: Fl_FormsPixmap.H 7903 2010-11-28 21:06:39Z matt $"
//
// Forms pixmap header file for the Fast Light Tool Kit (FLTK).
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
   Fl_FormsPixmap widget . */

#ifndef Fl_FormsPixmap_H
#define Fl_FormsPixmap_H

#include "Fl_Pixmap.H"

/**
  \class Fl_FormsPixmap
  \brief Forms pixmap drawing routines
*/
class FL_EXPORT Fl_FormsPixmap : public Fl_Widget {
    Fl_Pixmap *b;
protected:
    void draw();
public:
    Fl_FormsPixmap(Fl_Boxtype t, int X, int Y, int W, int H, const char *L= 0);

    void set(/*const*/char * const * bits);

    /**
      Set the internal pixmap pointer to an existing pixmap.
      \param[in] B existing pixmap
    */
    void Pixmap(Fl_Pixmap *B) {b = B;}

    /** Get the internal pixmap pointer. */
    Fl_Pixmap *Pixmap() const {return b;}
};

#endif

//
// End of "$Id: Fl_FormsPixmap.H 7903 2010-11-28 21:06:39Z matt $".
//
