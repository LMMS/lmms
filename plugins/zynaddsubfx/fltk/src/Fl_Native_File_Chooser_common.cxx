// "$Id: Fl_Native_File_Chooser_common.cxx 7977 2010-12-08 13:16:27Z AlbrechtS $"
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

#include <string.h>
#include <FL/Enumerations.H>

// COPY A STRING WITH 'new'
//    Value can be NULL
//
static char *strnew(const char *val) {
  if ( val == NULL ) return(NULL);
  char *s = new char[strlen(val)+1];
  strcpy(s, val);
  return(s);
}

// FREE STRING CREATED WITH strnew(), NULLS OUT STRING
//    Value can be NULL
//
static char *strfree(char *val) {
  if ( val ) delete [] val;
  return(NULL);
}

// 'DYNAMICALLY' APPEND ONE STRING TO ANOTHER
//    Returns newly allocated string, or NULL 
//    if s && val == NULL.
//    's' can be NULL; returns a strnew(val).
//    'val' can be NULL; s is returned unmodified.
//
//    Usage:
//	char *s = strnew("foo");	// s = "foo"
//      s = strapp(s, "bar");		// s = "foobar"
//
#if !defined(WIN32)
static char *strapp(char *s, const char *val) {
  if ( ! val ) {
    return(s);			// Nothing to append? return s
  }
  if ( ! s ) {
    return(strnew(val));	// New string? return copy of val
  }
  char *news = new char[strlen(s)+strlen(val)+1];
  strcpy(news, s);
  strcat(news, val);
  delete [] s;			// delete old string
  return(news);			// return new copy
}
#endif

// APPEND A CHARACTER TO A STRING
//     This does NOT allocate space for the new character.
//
static void chrcat(char *s, char c) {
  char tmp[2] = { c, '\0' };
  strcat(s, tmp);
}

//
// End of "$Id: Fl_Native_File_Chooser_common.cxx 7977 2010-12-08 13:16:27Z AlbrechtS $".
//
