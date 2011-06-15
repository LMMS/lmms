//
// "$Id: filename_setext.cxx 8063 2010-12-19 21:20:10Z matt $"
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

// Replace .ext with new extension

#include <FL/filename.H>
#include "flstring.h"

/**
   Replaces the extension in \p buf of max.<br>
   size \p buflen with the extension in \p ext.<br>
   If there's no '.' in \p buf, \p ext is appended.<br>
   If \p ext is NULL, behaves as if it were an empty string ("").

   \b Example
   \code
   #include <FL/filename.H>
   [..]
   char buf[FL_PATH_MAX] = "/path/myfile.cxx";
   fl_filename_setext(buf, sizeof(buf), ".txt");      // buf[] becomes "/path/myfile.txt"
   \endcode

   \return buf itself for calling convenience.
*/
char *fl_filename_setext(char *buf, int buflen, const char *ext) {
  char *q = (char *)fl_filename_ext(buf);
  if (ext) {
    strlcpy(q,ext,buflen - (q - buf));
  } else *q = 0;
  return(buf);
}


//
// End of "$Id: filename_setext.cxx 8063 2010-12-19 21:20:10Z matt $".
//
