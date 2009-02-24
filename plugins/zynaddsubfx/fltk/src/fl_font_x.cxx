//
// "$Id: fl_font_x.cxx 5421 2006-09-05 11:26:41Z matt $"
//
// Standard X11 font selection code for the Fast Light Tool Kit (FLTK).
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

Fl_FontSize::Fl_FontSize(const char* name) {
  font = XLoadQueryFont(fl_display, name);
  if (!font) {
    Fl::warning("bad font: %s", name);
    font = XLoadQueryFont(fl_display, "fixed"); // if fixed fails we crash
  }
#  if HAVE_GL
  listbase = 0;
#  endif
}

Fl_FontSize* fl_fontsize;

Fl_FontSize::~Fl_FontSize() {
#  if HAVE_GL
// Delete list created by gl_draw().  This is not done by this code
// as it will link in GL unnecessarily.  There should be some kind
// of "free" routine pointer, or a subclass?
// if (listbase) {
//  int base = font->min_char_or_byte2;
//  int size = font->max_char_or_byte2-base+1;
//  int base = 0; int size = 256;
//  glDeleteLists(listbase+base,size);
// }
#  endif
  if (this == fl_fontsize) fl_fontsize = 0;
  XFreeFont(fl_display, font);
}

////////////////////////////////////////////////////////////////

// WARNING: if you add to this table, you must redefine FL_FREE_FONT
// in Enumerations.H & recompile!!
static Fl_Fontdesc built_in_table[] = {
{"-*-helvetica-medium-r-normal--*"},
{"-*-helvetica-bold-r-normal--*"},
{"-*-helvetica-medium-o-normal--*"},
{"-*-helvetica-bold-o-normal--*"},
{"-*-courier-medium-r-normal--*"},
{"-*-courier-bold-r-normal--*"},
{"-*-courier-medium-o-normal--*"},
{"-*-courier-bold-o-normal--*"},
{"-*-times-medium-r-normal--*"},
{"-*-times-bold-r-normal--*"},
{"-*-times-medium-i-normal--*"},
{"-*-times-bold-i-normal--*"},
{"-*-symbol-*"},
{"-*-lucidatypewriter-medium-r-normal-sans-*"},
{"-*-lucidatypewriter-bold-r-normal-sans-*"},
{"-*-*zapf dingbats-*"}
};

Fl_Fontdesc* fl_fonts = built_in_table;

#define MAXSIZE 32767

// return dash number N, or pointer to ending null if none:
const char* fl_font_word(const char* p, int n) {
  while (*p) {if (*p=='-') {if (!--n) break;} p++;}
  return p;
}

// return a pointer to a number we think is "point size":
char* fl_find_fontsize(char* name) {
  char* c = name;
  // for standard x font names, try after 7th dash:
  if (*c == '-') {
    c = (char*)fl_font_word(c,7);
    if (*c++ && isdigit(*c)) return c;
    return 0; // malformed x font name?
  }
  char* r = 0;
  // find last set of digits:
  for (c++;* c; c++)
    if (isdigit(*c) && !isdigit(*(c-1))) r = c;
  return r;
}

const char* fl_encoding = "iso8859-1";

// return true if this matches fl_encoding:
int fl_correct_encoding(const char* name) {
  if (*name != '-') return 0;
  const char* c = fl_font_word(name,13);
  return (*c++ && !strcmp(c,fl_encoding));
}

// locate or create an Fl_FontSize for a given Fl_Fontdesc and size:
static Fl_FontSize* find(int fnum, int size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use font 0 if still undefined
  Fl_FontSize* f;
  for (f = s->first; f; f = f->next)
    if (f->minsize <= size && f->maxsize >= size) return f;
  fl_open_display();
  if (!s->xlist) {
    s->xlist = XListFonts(fl_display, s->name, 100, &(s->n));
    if (!s->xlist) {	// use fixed if no matching font...
      s->first = new Fl_FontSize("fixed");
      s->first->minsize = 0;
      s->first->maxsize = 32767;
      return s->first;
    }
  }
  // search for largest <= font size:
  char* name = s->xlist[0]; int ptsize = 0;	// best one found so far
  int matchedlength = 32767;
  char namebuffer[1024];	// holds scalable font name
  int found_encoding = 0;
  int m = s->n; if (m<0) m = -m;
  for (int n=0; n < m; n++) {

    char* thisname = s->xlist[n];
    if (fl_correct_encoding(thisname)) {
      if (!found_encoding) ptsize = 0; // force it to choose this
      found_encoding = 1;
    } else {
      if (found_encoding) continue;
    }
    char* c = fl_find_fontsize(thisname);
    int thissize = c ? atoi(c) : MAXSIZE;
    int thislength = strlen(thisname);
    if (thissize == size && thislength < matchedlength) {
      // exact match, use it:
      name = thisname;
      ptsize = size;
      matchedlength = thislength;
    } else if (!thissize && ptsize!=size) {
      // whoa!  A scalable font!  Use unless exact match found:
      int l = c-thisname;
      memcpy(namebuffer,thisname,l);
      l += sprintf(namebuffer+l,"%d",size);
      while (*c == '0') c++;
      strcpy(namebuffer+l,c);
      name = namebuffer;
      ptsize = size;
    } else if (!ptsize ||	// no fonts yet
	       thissize < ptsize && ptsize > size || // current font too big
	       thissize > ptsize && thissize <= size // current too small
      ) {
      name = thisname; ptsize = thissize;
      matchedlength = thislength;
    }
  }

  if (ptsize != size) { // see if we already found this unscalable font:
    for (f = s->first; f; f = f->next) {
      if (f->minsize <= ptsize && f->maxsize >= ptsize) {
	if (f->minsize > size) f->minsize = size;
	if (f->maxsize < size) f->maxsize = size;
	return f;
      }
    }
  }

  // okay, we definately have some name, make the font:
  f = new Fl_FontSize(name);
  if (ptsize < size) {f->minsize = ptsize; f->maxsize = size;}
  else {f->minsize = size; f->maxsize = ptsize;}
  f->next = s->first;
  s->first = f;
  return f;

}

////////////////////////////////////////////////////////////////
// Public interface:

int fl_font_ = 0;
int fl_size_ = 0;
XFontStruct* fl_xfont = 0;
void *fl_xftfont = 0;
static GC font_gc;

void fl_font(int fnum, int size) {
  if (fnum==-1) {
    fl_font_ = 0; fl_size_ = 0;
    return;
  }
  if (fnum == fl_font_ && size == fl_size_) return;
  fl_font_ = fnum; fl_size_ = size;
  Fl_FontSize* f = find(fnum, size);
  if (f != fl_fontsize) {
    fl_fontsize = f;
    fl_xfont = f->font;
    font_gc = 0;
  }
}

int fl_height() {
  if (fl_xfont) return (fl_xfont->ascent + fl_xfont->descent);
  else return -1;
}

int fl_descent() {
  if (fl_xfont) return fl_xfont->descent;
  else return -1;
}

double fl_width(const char* c, int n) {
  if (!fl_xfont) return -1.0;
  XCharStruct* p = fl_xfont->per_char;
  if (!p) return n*fl_xfont->min_bounds.width;
  int a = fl_xfont->min_char_or_byte2;
  int b = fl_xfont->max_char_or_byte2 - a;
  int w = 0;
  while (n--) {
    int x = *(uchar*)c++ - a;
    if (x >= 0 && x <= b) w += p[x].width;
    else w += fl_xfont->min_bounds.width;
  }
  return w;
}

double fl_width(uchar c) {
  if (!fl_xfont) return -1;
  XCharStruct* p = fl_xfont->per_char;
  if (p) {
    int a = fl_xfont->min_char_or_byte2;
    int b = fl_xfont->max_char_or_byte2 - a;
    int x = c-a;
    if (x >= 0 && x <= b) return p[x].width;
  }
  return fl_xfont->min_bounds.width;
}

void fl_draw(const char* str, int n, int x, int y) {
  if (font_gc != fl_gc) {
    if (!fl_xfont) fl_font(FL_HELVETICA, 14);
    font_gc = fl_gc;
    XSetFont(fl_display, fl_gc, fl_xfont->fid);
  }
  XDrawString(fl_display, fl_window, fl_gc, x, y, str, n);
}
  
void fl_draw(const char* str, int n, float x, float y) {
  fl_draw(str, n, (int)x, (int)y);
}

//
// End of "$Id: fl_font_x.cxx 5421 2006-09-05 11:26:41Z matt $".
//
