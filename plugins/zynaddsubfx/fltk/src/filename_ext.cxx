//
// "$Id: filename_ext.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Filename extension routines for the Fast Light Tool Kit (FLTK).
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

// returns pointer to the last '.' or to the null if none:

#include <FL/filename.H>

/** Gets the extensions of a filename.
   \code
   #include <FL/filename.H>
   [..]
   const char *out;
   out = fl_filename_ext("/some/path/foo.txt");        // result: ".txt"
   out = fl_filename_ext("/some/path/foo");            // result: NULL
   \endcode
   \param[in] buf the filename to be parsed
   \return a pointer to the extension (including '.') if any or NULL otherwise
 */
const char *fl_filename_ext(const char *buf) {
  const char *q = 0;
  const char *p = buf;
  for (p=buf; *p; p++) {
    if (*p == '/') q = 0;
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
    else if (*p == '\\') q = 0;
#endif
    else if (*p == '.') q = p;
  }
  return q ? q : p;
}

//
// End of "$Id: filename_ext.cxx 7903 2010-11-28 21:06:39Z matt $".
//
