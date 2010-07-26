//
// "$Id: Fl_Device.cxx 7659 2010-07-01 13:21:32Z manolo $"
//
// implementation of Fl_Device class for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010 by Bill Spitzak and others.
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
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//

#include <FL/Fl.H>
#include <FL/Fl_Device.H>
#include <FL/Fl_Image.H>

const char *Fl_Device::device_type = "Fl_Device";
const char *Fl_Surface_Device::device_type = "Fl_Surface_Device";
const char *Fl_Display_Device::device_type = "Fl_Display_Device";
const char *Fl_Graphics_Driver::device_type = "Fl_Graphics_Driver";
#if defined(__APPLE__) || defined(FL_DOXYGEN)
const char *Fl_Quartz_Graphics_Driver::device_type = "Fl_Quartz_Graphics_Driver";
#endif
#if defined(WIN32) || defined(FL_DOXYGEN)
const char *Fl_GDI_Graphics_Driver::device_type = "Fl_GDI_Graphics_Driver";
#endif
#if !(defined(__APPLE__) || defined(WIN32))
const char *Fl_Xlib_Graphics_Driver::device_type = "Fl_Xlib_Graphics_Driver";
#endif


/** \brief Use this drawing surface for future graphics requests. */
void Fl_Surface_Device::set_current(void)
{
  fl_graphics_driver = _driver;
  fl_surface = this;
}

//
// End of "$Id: Fl_Device.cxx 7659 2010-07-01 13:21:32Z manolo $".
//
