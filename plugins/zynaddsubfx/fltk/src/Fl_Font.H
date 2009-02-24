//
// "$Id: Fl_Font.H 5334 2006-08-19 15:24:55Z matt $"
//
// Font definitions for the Fast Light Tool Kit (FLTK).
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

// Two internal fltk data structures:
//
// Fl_Fontdesc: an entry into the fl_font() table.  There is one of these
// for each fltk font number.
//
// Fl_FontSize: a structure for an actual system font, with junk to
// help choose it and info on character sizes.  Each Fl_Fontdesc has a
// linked list of these.  These are created the first time each system
// font/size combination is used.

#ifndef FL_FONT_
#define FL_FONT_

#include <config.h>

#  if USE_XFT
typedef struct _XftFont XftFont;
#  endif // USE_XFT

class Fl_FontSize {
public:
  Fl_FontSize *next;	// linked list for this Fl_Fontdesc
#  ifdef WIN32
  HFONT fid;
  int width[256];
  TEXTMETRIC metr;
  FL_EXPORT Fl_FontSize(const char* fontname, int size);
#  elif defined(__APPLE_QD__)
  FL_EXPORT Fl_FontSize(const char* fontname, int size);
  short font, face, size;
  short ascent, descent;
  short width[256];
  bool knowMetrics;
#  elif defined(__APPLE_QUARTZ__)
  FL_EXPORT Fl_FontSize(const char* fontname, int size);
  ATSUTextLayout layout;
  ATSUStyle style;
  short ascent, descent, q_width;
  short width[256];
  bool knowWidths;
  char *q_name;
  int size;
#  elif USE_XFT
  XftFont* font;
  const char* encoding;
  int size;
  FL_EXPORT Fl_FontSize(const char* xfontname);
#  else
  XFontStruct* font;	// X font information
  FL_EXPORT Fl_FontSize(const char* xfontname);
#  endif
  int minsize;		// smallest point size that should use this
  int maxsize;		// largest point size that should use this
#  if HAVE_GL
  unsigned int listbase;// base of display list, 0 = none
#  endif
  FL_EXPORT ~Fl_FontSize();
};

extern FL_EXPORT Fl_FontSize *fl_fontsize; // the currently selected one

struct Fl_Fontdesc {
  const char *name;
  char fontname[128];	// "Pretty" font name
  Fl_FontSize *first;	// linked list of sizes of this style
#  ifndef WIN32
  char **xlist;		// matched X font names
  int n;		// size of xlist, negative = don't free xlist!
#  endif
};

extern FL_EXPORT Fl_Fontdesc *fl_fonts; // the table

#  ifndef WIN32
// functions for parsing X font names:
FL_EXPORT const char* fl_font_word(const char *p, int n);
FL_EXPORT char *fl_find_fontsize(char *name);
#  endif

#endif

//
// End of "$Id: Fl_Font.H 5334 2006-08-19 15:24:55Z matt $".
//
