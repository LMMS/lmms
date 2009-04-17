//
// "$Id: fl_draw.cxx 5565 2006-12-21 19:39:26Z mike $"
//
// Label drawing code for the Fast Light Tool Kit (FLTK).
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

// Implementation of fl_draw(const char*,int,int,int,int,Fl_Align)
// Used to draw all the labels and text, this routine:
// Word wraps the labels to fit into their bounding box.
// Breaks them into lines at the newlines.
// Expands all unprintable characters to ^X or \nnn notation
// Aligns them against the inside of the box.

#define min(a,b) ((a)<(b)?(a):(b))
#include <FL/fl_draw.H>
#include <FL/Fl_Image.H>

#include "flstring.h"
#include <ctype.h>
#include <math.h>

#define MAXBUF 1024

char fl_draw_shortcut;	// set by fl_labeltypes.cxx

static char* underline_at;

// Copy p to buf, replacing unprintable characters with ^X and \nnn
// Stop at a newline or if MAXBUF characters written to buffer.
// Also word-wrap if width exceeds maxw.
// Returns a pointer to the start of the next line of caharcters.
// Sets n to the number of characters put into the buffer.
// Sets width to the width of the string in the current font.

static const char*
expand(const char* from, char* buf, double maxw, int& n, double &width,
       int wrap, int draw_symbols) {
  char* o = buf;
  char* e = buf+(MAXBUF-4);
  underline_at = 0;
  char* word_end = o;
  const char* word_start = from;
  double w = 0;

  const char* p = from;
  for (;; p++) {

    int c = *p & 255;

    if (!c || c == ' ' || c == '\n') {
      // test for word-wrap:
      if (word_start < p && wrap) {
	double newwidth = w + fl_width(word_end, o-word_end);
	if (word_end > buf && newwidth > maxw) { // break before this word
	  o = word_end;
	  p = word_start;
	  break;
	}
	word_end = o;
	w = newwidth;
      }
      if (!c) break;
      else if (c == '\n') {p++; break;}
      word_start = p+1;
    }

    if (o > e) break; // don't overflow buffer

    if (c == '\t') {
      for (c = (o-buf)%8; c<8 && o<e; c++) *o++ = ' ';
    } else if (c == '&' && fl_draw_shortcut && *(p+1)) {
      if (*(p+1) == '&') {p++; *o++ = '&';}
      else if (fl_draw_shortcut != 2) underline_at = o;
    } else if (c < ' ' || c == 127) { // ^X
      *o++ = '^';
      *o++ = c ^ 0x40;
#ifdef __APPLE__
    } else if (c == 0xCA) { // non-breaking space in MacRoman
#else
    } else if (c == 0xA0) { // non-breaking space in ISO 8859
#endif
      *o++ = ' ';
    } else if (c == '@' && draw_symbols) { // Symbol???
      if (p[1] && p[1] != '@')  break;
      *o++ = c;
      if (p[1]) p++;
    } else {
      *o++ = c;
    }
  }

  width = w + fl_width(word_end, o-word_end);
  *o = 0;
  n = o-buf;
  return p;
}

void fl_draw(
    const char* str,	// the (multi-line) string
    int x, int y, int w, int h,	// bounding box
    Fl_Align align,
    void (*callthis)(const char*,int,int,int),
    Fl_Image* img, int draw_symbols) {
  const char* p;
  const char* e;
  char buf[MAXBUF];
  int buflen;
  char symbol[2][255], *symptr;
  int symwidth[2], symoffset, symtotal;

  // count how many lines and put the last one into the buffer:
  int lines;
  double width;

  symbol[0][0] = '\0';
  symwidth[0]  = 0;

  symbol[1][0] = '\0';
  symwidth[1]  = 0;

  if (draw_symbols) {
    if (str && str[0] == '@' && str[1] && str[1] != '@') {
      // Start with a symbol...
      for (symptr = symbol[0];
           *str && !isspace(*str) && symptr < (symbol[0] + sizeof(symbol[0]) - 1);
           *symptr++ = *str++);
      *symptr = '\0';
      if (isspace(*str)) str++;
      symwidth[0] = min(w,h);
    }

    if (str && (p = strrchr(str, '@')) != NULL && p > (str + 1) && p[-1] != '@') {
      strlcpy(symbol[1], p, sizeof(symbol[1]));
      symwidth[1] = min(w,h);
    }
  }

  symtotal = symwidth[0] + symwidth[1];

  for (p = str, lines=0; p;) {
    e = expand(p, buf, w - symtotal, buflen, width, align&FL_ALIGN_WRAP,
               draw_symbols);
    lines++;
    if (!*e || (*e == '@' && e[1] != '@' && draw_symbols)) break;
    p = e;
  }

  if ((symwidth[0] || symwidth[1]) && lines) {
    if (symwidth[0]) symwidth[0] = lines * fl_height();
    if (symwidth[1]) symwidth[1] = lines * fl_height();
  }

  symtotal = symwidth[0] + symwidth[1];
  
  // figure out vertical position of the first line:
  int xpos;
  int ypos;
  int height = fl_height();
  int imgh = img ? img->h() : 0;

  symoffset = 0;

  if (align & FL_ALIGN_BOTTOM) ypos = y+h-(lines-1)*height-imgh;
  else if (align & FL_ALIGN_TOP) ypos = y+height;
  else ypos = y+(h-lines*height-imgh)/2+height;

  // draw the image unless the "text over image" alignment flag is set...
  if (img && !(align & FL_ALIGN_TEXT_OVER_IMAGE)) {
    if (img->w() > symoffset) symoffset = img->w();

    if (align & FL_ALIGN_LEFT) xpos = x + symwidth[0];
    else if (align & FL_ALIGN_RIGHT) xpos = x + w - img->w() - symwidth[1];
    else xpos = x + (w - img->w() - symtotal) / 2 + symwidth[0];

    img->draw(xpos, ypos - height);
    ypos += img->h();
  }

  // now draw all the lines:
  if (str) {
    int desc = fl_descent();
    for (p=str; ; ypos += height) {
      if (lines>1) e = expand(p, buf, w - symtotal, buflen, width,
                              align&FL_ALIGN_WRAP, draw_symbols);
      else e = "";

      if (width > symoffset) symoffset = (int)(width + 0.5);

      if (align & FL_ALIGN_LEFT) xpos = x + symwidth[0];
      else if (align & FL_ALIGN_RIGHT) xpos = x + w - (int)(width + .5) - symwidth[1];
      else xpos = x + (w - (int)(width + .5) - symtotal) / 2 + symwidth[0];

      callthis(buf,buflen,xpos,ypos-desc);

      if (underline_at && underline_at >= buf && underline_at < (buf + buflen))
	callthis("_",1,xpos+int(fl_width(buf,underline_at-buf)),ypos-desc);

      if (!*e || (*e == '@' && e[1] != '@')) break;
      p = e;
    }
  }

  // draw the image if the "text over image" alignment flag is set...
  if (img && (align & FL_ALIGN_TEXT_OVER_IMAGE)) {
    if (img->w() > symoffset) symoffset = img->w();

    if (align & FL_ALIGN_LEFT) xpos = x + symwidth[0];
    else if (align & FL_ALIGN_RIGHT) xpos = x + w - img->w() - symwidth[1];
    else xpos = x + (w - img->w() - symtotal) / 2 + symwidth[0];

    img->draw(xpos, ypos);
  }

  // draw the symbols, if any...
  if (symwidth[0]) {
    // draw to the left
    if (align & FL_ALIGN_LEFT) xpos = x;
    else if (align & FL_ALIGN_RIGHT) xpos = x + w - symtotal - symoffset;
    else xpos = x + (w - symoffset - symtotal) / 2;

    if (align & FL_ALIGN_BOTTOM) ypos = y + h - symwidth[0];
    else if (align & FL_ALIGN_TOP) ypos = y;
    else ypos = y + (h - symwidth[0]) / 2;

    fl_draw_symbol(symbol[0], xpos, ypos, symwidth[0], symwidth[0], fl_color());
  }

  if (symwidth[1]) {
    // draw to the right
    if (align & FL_ALIGN_LEFT) xpos = x + symoffset + symwidth[0];
    else if (align & FL_ALIGN_RIGHT) xpos = x + w - symwidth[1];
    else xpos = x + (w - symoffset - symtotal) / 2 + symoffset + symwidth[0];

    if (align & FL_ALIGN_BOTTOM) ypos = y + h - symwidth[1];
    else if (align & FL_ALIGN_TOP) ypos = y;
    else ypos = y + (h - symwidth[1]) / 2;

    fl_draw_symbol(symbol[1], xpos, ypos, symwidth[1], symwidth[1], fl_color());
  }
}

void fl_draw(
  const char* str,	// the (multi-line) string
  int x, int y, int w, int h,	// bounding box
  Fl_Align align,
  Fl_Image* img,
  int draw_symbols) {
  if ((!str || !*str) && !img) return;
  if (w && h && !fl_not_clipped(x, y, w, h) && (align & FL_ALIGN_INSIDE)) return;
  if (align & FL_ALIGN_CLIP) fl_clip(x, y, w, h);
  fl_draw(str, x, y, w, h, align, fl_draw, img, draw_symbols);
  if (align & FL_ALIGN_CLIP) fl_pop_clip();
}

void fl_measure(const char* str, int& w, int& h, int draw_symbols) {
  if (!str || !*str) {w = 0; h = 0; return;}
  h = fl_height();
  const char* p;
  const char* e;
  char buf[MAXBUF];
  int buflen;
  int lines;
  double width;
  int W = 0;
  char symbol[2][255], *symptr;
  int symwidth[2], symtotal;

  // count how many lines and put the last one into the buffer:
  symbol[0][0] = '\0';
  symwidth[0]  = 0;

  symbol[1][0] = '\0';
  symwidth[1]  = 0;

  if (draw_symbols) {
    if (str && str[0] == '@' && str[1] && str[1] != '@') {
      // Start with a symbol...
      for (symptr = symbol[0];
           *str && !isspace(*str) && symptr < (symbol[0] + sizeof(symbol[0]) - 1);
           *symptr++ = *str++);
      *symptr = '\0';
      if (isspace(*str)) str++;
      symwidth[0] = h;
    }

    if (str && (p = strrchr(str, '@')) != NULL && p > (str + 1) && p[-1]!='@') {
      strlcpy(symbol[1], p, sizeof(symbol[1]));
      symwidth[1] = h;
    }
  }

  symtotal = symwidth[0] + symwidth[1];
  
  for (p = str, lines=0; p;) {
    e = expand(p, buf, w - symtotal, buflen, width, w != 0, draw_symbols);
    if ((int)ceil(width) > W) W = (int)ceil(width);
    lines++;
    if (!*e || (*e == '@' && draw_symbols)) break;
    p = e;
  }

  if ((symwidth[0] || symwidth[1]) && lines) {
    if (symwidth[0]) symwidth[0] = lines * fl_height();
    if (symwidth[1]) symwidth[1] = lines * fl_height();
  }

  symtotal = symwidth[0] + symwidth[1];

  w = W + symtotal;
  h = lines*h;
}

//
// End of "$Id: fl_draw.cxx 5565 2006-12-21 19:39:26Z mike $".
//
