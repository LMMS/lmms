//
// "$Id: Fl_abort.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Warning/error message code for the Fast Light Tool Kit (FLTK).
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

// This method is in it's own source file so that stdlib and stdio
// do not need to be included in Fl.cxx:
// You can also override this by redefining all of these.

#include <FL/Fl.H>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "flstring.h"

#ifdef WIN32
#  include <windows.h>

static void warning(const char *, ...) {
  // Show nothing for warnings under WIN32...
}

static void error(const char *format, ...) {
  va_list args;
  char buf[1024];
  va_start(args, format);
  vsnprintf(buf, 1024, format, args);
  va_end(args);
  MessageBox(0,buf,"Error",MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
}

static void fatal(const char *format, ...) {
  va_list args;
  char buf[1024];
  va_start(args, format);
  vsnprintf(buf, 1024, format, args);
  va_end(args);
  MessageBox(0,buf,"Error",MB_ICONSTOP|MB_SYSTEMMODAL);
  ::exit(1);
}

#else

static void warning(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  fflush(stderr);
}

static void error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  fflush(stderr);
}

static void fatal(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);
  fflush(stderr);
  ::exit(1);
}

#endif

void (*Fl::warning)(const char* format, ...) = ::warning;
void (*Fl::error)(const char* format, ...) = ::error;
void (*Fl::fatal)(const char* format, ...) = ::fatal;

//
// End of "$Id: Fl_abort.cxx 5190 2006-06-09 16:16:34Z mike $".
//
