//
// "$Id: Fl_XPM_Image.H 7903 2010-11-28 21:06:39Z matt $"
//
// XPM image header file for the Fast Light Tool Kit (FLTK).
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
  
   Fl_XPM_Image class . */

#ifndef Fl_XPM_Image_H
#define Fl_XPM_Image_H
#  include "Fl_Pixmap.H"

/**
  The Fl_XPM_Image class supports loading, caching,
  and drawing of X Pixmap (XPM) images, including transparency.
*/
class FL_EXPORT Fl_XPM_Image : public Fl_Pixmap {

  public:

  Fl_XPM_Image(const char* filename);
};

#endif // !Fl_XPM_Image

//
// End of "$Id: Fl_XPM_Image.H 7903 2010-11-28 21:06:39Z matt $".
//
