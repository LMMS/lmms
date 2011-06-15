//
// "$Id: Fl_Browser_load.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// File loading routines for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>
#include <stdio.h>
#include <FL/fl_utf8.h>

/**
  Clears the browser and reads the file, adding each line from the file
  to the browser.  If the filename is NULL or a zero-length
  string then this just clears the browser.  This returns zero if there
  was any error in opening or reading the file, in which case errno
  is set to the system error.  The data() of each line is set
  to NULL.
  \param[in] filename The filename to load
  \returns 1 if OK, 0 on error (errno has reason)
  \see add()
*/
int Fl_Browser::load(const char *filename) {
#define MAXFL_BLINE 1024
    char newtext[MAXFL_BLINE];
    int c;
    int i;
    clear();
    if (!filename || !(filename[0])) return 1;
    FILE *fl = fl_fopen(filename,"r");
    if (!fl) return 0;
    i = 0;
    do {
	c = getc(fl);
	if (c == '\n' || c <= 0 || i>=(MAXFL_BLINE-1)) {
	    newtext[i] = 0;
	    add(newtext);
	    i = 0;
	} else {
	    newtext[i++] = c;
	}
    } while (c >= 0);
    fclose(fl);
    return 1;
}

//
// End of "$Id: Fl_Browser_load.cxx 7903 2010-11-28 21:06:39Z matt $".
//
