//
// "$Id: Fl_XColor.H 4288 2005-04-16 00:13:17Z mike $"
//
// X-specific color definitions for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2001 by Bill Spitzak and others.
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

#include <config.h>
#include <FL/Enumerations.H>

// one of these for each color in fltk's "colormap":
// if overlays are enabled, another one for the overlay
struct Fl_XColor {
  unsigned char r,g,b;	// actual color used by X
  unsigned char mapped;	// true when XAllocColor done
  unsigned long pixel;	// the X pixel to use
};
extern Fl_XColor fl_xmap[/*overlay*/][256];

// mask & shifts to produce xcolor for truecolor visuals:
extern unsigned char fl_redmask, fl_greenmask, fl_bluemask;
extern int fl_redshift, fl_greenshift, fl_blueshift, fl_extrashift;

//
// End of "$Id: Fl_XColor.H 4288 2005-04-16 00:13:17Z mike $".
//
