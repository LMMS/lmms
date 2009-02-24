//
// "$Id: filename_isdir.cxx 5948 2007-10-07 10:12:32Z matt $"
//
// Directory detection routines for the Fast Light Tool Kit (FLTK).
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

// Used by fl_file_chooser

#include "flstring.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <FL/filename.H>


#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
static inline int isdirsep(char c) {return c=='/' || c=='\\';}
#else
#define isdirsep(c) ((c)=='/')
#endif

int _fl_filename_isdir_quick(const char* n) {
  // Do a quick optimization for filenames with a trailing slash...
  if (*n && isdirsep(n[strlen(n) - 1])) return 1;
  return fl_filename_isdir(n);
}

int fl_filename_isdir(const char* n) {
  struct stat	s;
  char		fn[1024];
  int		length;

  length = strlen(n);

#ifdef WIN32
  // This workaround brought to you by the fine folks at Microsoft!
  // (read lots of sarcasm in that...)
  if (length < (int)(sizeof(fn) - 1)) {
    if (length < 4 && isalpha(n[0]) && n[1] == ':' &&
        (isdirsep(n[2]) || !n[2])) {
      // Always use D:/ for drive letters
      fn[0] = n[0];
      strcpy(fn + 1, ":/");
      n = fn;
    } else if (length > 0 && isdirsep(n[length - 1])) {
      // Strip trailing slash from name...
      length --;
      memcpy(fn, n, length);
      fn[length] = '\0';
      n = fn;
    }
  }
#else
  // Matt: Just in case, we strip the slash for other operating
  // systems as well, avoid bugs by sloppy implementations
  // of "stat".
  if (length > 1 && isdirsep(n[length - 1])) {
    length --;
    memcpy(fn, n, length);
    fn[length] = '\0';
    n = fn;
  }
#endif

  return !stat(n, &s) && (s.st_mode&0170000)==0040000;
}

//
// End of "$Id: filename_isdir.cxx 5948 2007-10-07 10:12:32Z matt $".
//
