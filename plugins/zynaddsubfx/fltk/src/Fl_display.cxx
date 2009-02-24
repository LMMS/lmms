//
// "$Id: Fl_display.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Display function for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

// Startup method to set what display to use.
// Using setenv makes programs that are exec'd use the same display.

#include <FL/Fl.H>
#include <stdlib.h>
#include "flstring.h"

void Fl::display(const char *d) {
#if defined(__APPLE__) || defined(WIN32)
  (void)d;
#else
  static char e[1024];
  strcpy(e,"DISPLAY=");
  strlcat(e,d,sizeof(e));
  for (char *c = e+8; *c!=':'; c++) {
    if (!*c) {
      strlcat(e,":0.0",sizeof(e));
      break;
    }
  }
  putenv(e);
#endif // __APPLE__
}

//
// End of "$Id: Fl_display.cxx 5190 2006-06-09 16:16:34Z mike $".
//
