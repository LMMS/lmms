//
// "$Id: fl_show_colormap.H 8621 2011-04-23 15:46:30Z AlbrechtS $"
//
// Colormap picker header file for the Fast Light Tool Kit (FLTK).
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

/** \file
   The fl_show_colormap() function hides the implementation classes used
   to provide the popup window and color selection mechanism.
*/

#ifndef fl_show_colormap_H
#define fl_show_colormap_H

/* doxygen comment here to avoid exposing ColorMenu in fl_show_colormap.cxx
*/

/** \addtogroup  fl_attributes
    @{ */

/**
  \brief Pops up a window to let the user pick a colormap entry.
  \image html fl_show_colormap.png
  \image latex fl_show_colormap.png "fl_show_colormap" height=10cm
  \param[in] oldcol color to be highlighted when grid is shown.
  \retval Fl_Color value of the chosen colormap entry.
  \see Fl_Color_Chooser
*/
FL_EXPORT Fl_Color fl_show_colormap(Fl_Color oldcol);

/** @} */

#endif

//
// End of "$Id: fl_show_colormap.H 8621 2011-04-23 15:46:30Z AlbrechtS $".
//
