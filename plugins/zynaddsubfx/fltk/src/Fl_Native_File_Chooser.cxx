// "$Id: Fl_Native_File_Chooser.cxx 8378 2011-02-05 22:35:51Z matt $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2010 by Bill Spitzak and others.
// Copyright 2004 Greg Ercolano.
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

// Use Windows' chooser
#ifdef WIN32
#include "Fl_Native_File_Chooser_WIN32.cxx"
#endif

// Use Apple's chooser
#ifdef __APPLE__
#include <FL/Fl_Native_File_Chooser.H>
#endif

// All else falls back to FLTK's own chooser
#if ! defined(__APPLE__) && !defined(WIN32)
#include "Fl_Native_File_Chooser_FLTK.cxx"
#endif

const char *Fl_Native_File_Chooser::file_exists_message = "File exists. Are you sure you want to overwrite?";

//
// End of "$Id: Fl_Native_File_Chooser.cxx 8378 2011-02-05 22:35:51Z matt $".
//
