//
// "$Id: Fl_XPM_Image.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Fl_XPM_Image routines.
//
// Copyright 1997-2005 by Bill Spitzak and others.
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
// Contents:
//
//

//
// Include necessary header files...
//

#include <FL/Fl.H>
#include <FL/Fl_XPM_Image.H>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"


//
// 'hexdigit()' - Convert a hex digit to an integer.
//

static int hexdigit(int x) {	// I - Hex digit...
  if (isdigit(x)) return x-'0';
  if (isupper(x)) return x-'A'+10;
  if (islower(x)) return x-'a'+10;
  return 20;
}

#define MAXSIZE 2048
#define INITIALLINES 256

Fl_XPM_Image::Fl_XPM_Image(const char *name) : Fl_Pixmap((char *const*)0) {
  FILE *f;

  if ((f = fopen(name, "rb")) == NULL) return;

  // read all the c-strings out of the file:
  char** new_data = new char *[INITIALLINES];
  char** temp_data;
  int malloc_size = INITIALLINES;
  char buffer[MAXSIZE+20];
  int i = 0;
  while (fgets(buffer,MAXSIZE+20,f)) {
    if (buffer[0] != '\"') continue;
    char *myp = buffer;
    char *q = buffer+1;
    while (*q != '\"' && myp < buffer+MAXSIZE) {
      if (*q == '\\') switch (*++q) {
      case '\r':
      case '\n':
	fgets(q,(buffer+MAXSIZE+20)-q,f); break;
      case 0:
	break;
      case 'x': {
	q++;
	int n = 0;
	for (int x = 0; x < 3; x++) {
	  int xd = hexdigit(*q);
	  if (xd > 15) break;
	  n = (n<<4)+xd;
	  q++;
	}
	*myp++ = n;
      } break;
      default: {
	int c = *q++;
	if (c>='0' && c<='7') {
	  c -= '0';
	  for (int x=0; x<2; x++) {
	    int xd = hexdigit(*q);
	    if (xd>7) break;
	    c = (c<<3)+xd;
	    q++;
	  }
	}
	*myp++ = c;
      } break;
      } else {
	*myp++ = *q++;
      }
    }
    *myp++ = 0;
    if (i >= malloc_size) {
      temp_data = new char *[malloc_size + INITIALLINES];
      memcpy(temp_data, new_data, sizeof(char *) * malloc_size);
      delete[] new_data;
      new_data = temp_data;
      malloc_size += INITIALLINES;
    }
    new_data[i] = new char[myp-buffer+1];
    memcpy(new_data[i], buffer,myp-buffer);
    new_data[i][myp-buffer] = 0;
    i++;
  }

  fclose(f);

  data((const char **)new_data, i);
  alloc_data = 1;

  measure();
}


//
// End of "$Id: Fl_XPM_Image.cxx 5190 2006-06-09 16:16:34Z mike $".
//
