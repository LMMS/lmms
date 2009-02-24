//
// "$Id: fl_set_font.cxx 5420 2006-09-05 11:16:15Z matt $"
//
// Font utilities for the Fast Light Tool Kit (FLTK).
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

// Add a font to the internal table.
// Also see fl_set_fonts.cxx which adds all possible fonts.

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include "flstring.h"
#include "Fl_Font.H"
#include <stdlib.h>

static int table_size;

void Fl::set_font(Fl_Font fnum, const char* name) {
  while (fnum >= table_size) {
    int i = table_size;
    if (!i) {	// don't realloc the built-in table
      table_size = 2*FL_FREE_FONT;
      i = FL_FREE_FONT;
      Fl_Fontdesc* t = (Fl_Fontdesc*)malloc(table_size*sizeof(Fl_Fontdesc));
      memcpy(t, fl_fonts, FL_FREE_FONT*sizeof(Fl_Fontdesc));
      fl_fonts = t;
    } else {
      table_size = 2*table_size;
      fl_fonts=(Fl_Fontdesc*)realloc(fl_fonts, table_size*sizeof(Fl_Fontdesc));
    }
    for (; i < table_size; i++) {
      fl_fonts[i].fontname[0] = 0;
      fl_fonts[i].name = 0;
#if !defined(WIN32) && !defined(__APPLE__)
      fl_fonts[i].xlist = 0;
      fl_fonts[i].n = 0;
#endif // !WIN32 && !__APPLE__
    }
  }
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (s->name) {
    if (!strcmp(s->name, name)) {s->name = name; return;}
#if !defined(WIN32) && !defined(__APPLE__)
    if (s->xlist && s->n >= 0) XFreeFontNames(s->xlist);
#endif
    for (Fl_FontSize* f = s->first; f;) {
      Fl_FontSize* n = f->next; delete f; f = n;
    }
    s->first = 0;
  }
  s->name = name;
  s->fontname[0] = 0;
#if !defined(WIN32) && !defined(__APPLE__)
  s->xlist = 0;
#endif
  s->first = 0;
  fl_font(-1, 0);
}

void Fl::set_font(Fl_Font fnum, Fl_Font from) {
  Fl::set_font(fnum, get_font(from));
}

const char* Fl::get_font(Fl_Font fnum) {return fl_fonts[fnum].name;}

//
// End of "$Id: fl_set_font.cxx 5420 2006-09-05 11:16:15Z matt $".
//
