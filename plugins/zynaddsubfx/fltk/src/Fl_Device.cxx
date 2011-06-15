//
// "$Id: Fl_Device.cxx 8504 2011-03-04 16:48:10Z manolo $"
//
// implementation of Fl_Device class for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2011 by Bill Spitzak and others.
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

const char *Fl_Device::class_id = "Fl_Device";
const char *Fl_Surface_Device::class_id = "Fl_Surface_Device";
const char *Fl_Display_Device::class_id = "Fl_Display_Device";
const char *Fl_Graphics_Driver::class_id = "Fl_Graphics_Driver";
#if defined(__APPLE__) || defined(FL_DOXYGEN)
const char *Fl_Quartz_Graphics_Driver::class_id = "Fl_Quartz_Graphics_Driver";
#endif
#if defined(WIN32) || defined(FL_DOXYGEN)
const char *Fl_GDI_Graphics_Driver::class_id = "Fl_GDI_Graphics_Driver";
#endif
#if !(defined(__APPLE__) || defined(WIN32))
const char *Fl_Xlib_Graphics_Driver::class_id = "Fl_Xlib_Graphics_Driver";
#endif


/** \brief Use this drawing surface for future graphics requests. */
void Fl_Surface_Device::set_current(void)
{
  fl_graphics_driver = _driver;
  _surface = this;
}

const Fl_Graphics_Driver::matrix Fl_Graphics_Driver::m0 = {1, 0, 0, 1, 0, 0};

Fl_Graphics_Driver::Fl_Graphics_Driver() {
  font_ = 0;
  size_ = 0;
  sptr=0; rstackptr=0; 
  fl_clip_state_number=0;
  m = m0; 
  fl_matrix = &m; 
  p = (XPOINT *)0;
  font_descriptor_ = NULL;
};

void Fl_Graphics_Driver::text_extents(const char*t, int n, int& dx, int& dy, int& w, int& h)
{
  w = (int)width(t, n);
  h = - height();
  dx = 0;
  dy = descent();
}

Fl_Display_Device::Fl_Display_Device(Fl_Graphics_Driver *graphics_driver) : Fl_Surface_Device( graphics_driver) {
#ifdef __APPLE__
  SInt32 versionMajor = 0;
  SInt32 versionMinor = 0;
  SInt32 versionBugFix = 0;
  Gestalt( gestaltSystemVersionMajor, &versionMajor );
  Gestalt( gestaltSystemVersionMinor, &versionMinor );
  Gestalt( gestaltSystemVersionBugFix, &versionBugFix );
  fl_mac_os_version = versionMajor * 10000 + versionMinor * 100 + versionBugFix;
#endif
};


//
// End of "$Id: Fl_Device.cxx 8504 2011-03-04 16:48:10Z manolo $".
//
