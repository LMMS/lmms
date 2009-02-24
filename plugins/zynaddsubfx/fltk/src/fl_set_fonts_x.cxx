//
// "$Id: fl_set_fonts_x.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// X11 font utilities for the Fast Light Tool Kit (FLTK).
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

// This function fills in the fltk font table with all the fonts that
// are found on the X server.  It tries to place the fonts into families
// and to sort them so the first 4 in a family are normal, bold, italic,
// and bold italic.

// Standard X fonts are matched by a pattern that is always of
// this form, and this pattern is put in the table:
// "-*-family-weight-slant-width1-style-*-registry-encoding"

// Non-standard font names (those not starting with '-') are matched
// by a pattern of the form "prefix*suffix", where the '*' is where
// fltk thinks the point size is, or by the actual font name if no
// point size is found.

// Fltk knows how to pull an "attribute" out of a font name, such as
// bold or italic, by matching known x font field values.  All words
// that don't match a known attribute are combined into the "name"
// of the font.  Names are compared before attributes for sorting, this
// makes the bold and plain version of a font come out next to each
// other despite the poor X font naming scheme.

// By default fl_set_fonts() only does iso8859-1 encoded fonts.  You can
// do all normal X fonts by passing "-*" or every possible font with "*".

// Fl::set_font will take strings other than the ones this stores
// and can identify any font on X that way.  You may want to write your
// own system of font management and not use this code.

// turn word N of a X font name into either some attribute bits
// (right now 0, FL_BOLD, or FL_ITALIC), or into -1 indicating that
// the word should be put into the name:

static int attribute(int n, const char *p) {
  // don't put blank things into name:
  if (!*p || *p=='-' || *p=='*') return 0;
  if (n == 3) { // weight
    if (!strncmp(p,"normal",6) ||
	!strncmp(p,"light",5) ||
	!strncmp(p,"medium",6) ||
	!strncmp(p,"book",4)) return 0;
    if (!strncmp(p,"bold",4) || !strncmp(p,"demi",4)) return FL_BOLD;
  } else if (n == 4) { // slant
    if (*p == 'r') return 0;
    if (*p == 'i' || *p == 'o') return FL_ITALIC;
  } else if (n == 5) { // sWidth
    if (!strncmp(p,"normal",6)) return 0;
  }
  return -1;
}

// return non-zero if the registry-encoding should be used:
extern const char* fl_encoding;
static int use_registry(const char *p) {
  return *p && *p!='*' && strcmp(p,fl_encoding);
}

// Bug: older versions calculated the value for *ap as a side effect of
// making the name, and then forgot about it. To avoid having to change
// the header files I decided to store this value in the last character
// of the font name array.
#define ENDOFBUFFER 127 // sizeof(Fl_Font.fontname)-1

// turn a stored (with *'s) X font name into a pretty name:
const char* Fl::get_font_name(Fl_Font fnum, int* ap) {
  Fl_Fontdesc *f = fl_fonts + fnum;
  if (!f->fontname[0]) {
    int type = 0;
    const char* p = f->name;
    if (!p) {
      if (ap) *ap = 0;
      return "";
    }
    char *o = f->fontname;

    if (*p != '-') { // non-standard font, just replace * with spaces:
      if (strstr(p,"bold")) type = FL_BOLD;
      if (strstr(p,"ital")) type |= FL_ITALIC;
      for (;*p; p++) {
	if (*p == '*' || *p == ' ' || *p == '-') {
	  do p++; while (*p == '*' || *p == ' ' || *p == '-');
	  if (!*p) break;
	  if (o < (f->fontname + ENDOFBUFFER - 1)) *o++ = ' ';
	}
	if (o < (f->fontname + ENDOFBUFFER - 1)) *o++ = *p;
      }
      *o = 0;

    } else { // standard dash-seperated font:

      // get the family:
      const char *x = fl_font_word(p,2); if (*x) x++; if (*x=='*') x++;
      if (!*x) {
	if (ap) *ap = 0;
	return p;
      }
      const char *e = fl_font_word(x,1);
      if ((e - x) < (int)(ENDOFBUFFER - 1)) {
	// MRS: we want strncpy here, not strlcpy...
	strncpy(o,x,e-x);
	o += e-x;
      } else {
	strlcpy(f->fontname, x, ENDOFBUFFER);
	o = f->fontname+ENDOFBUFFER-1;
      }

      // collect all the attribute words:
      for (int n = 3; n <= 6; n++) {
	// get the next word:
	if (*e) e++; x = e; e = fl_font_word(x,1);
	int t = attribute(n,x);
	if (t < 0) {
	  if (o < (f->fontname + ENDOFBUFFER - 1)) *o++ = ' ';
	  if ((e - x) < (int)(ENDOFBUFFER - (o - f->fontname) - 1)) {
	    // MRS: we want strncpy here, not strlcpy...
	    strncpy(o,x,e-x);
	    o += e-x;
	  } else {
	    strlcpy(o,x, ENDOFBUFFER - (o - f->fontname) - 1);
	    o = f->fontname+ENDOFBUFFER-1;
	  }
	} else type |= t;
      }

      // skip over the '*' for the size and get the registry-encoding:
      x = fl_font_word(e,2);
      if (*x) {x++; *o++ = '('; while (*x) *o++ = *x++; *o++ = ')';}

      *o = 0;
      if (type & FL_BOLD) strlcat(f->fontname, " bold", ENDOFBUFFER);
      if (type & FL_ITALIC) strlcat(f->fontname, " italic", ENDOFBUFFER);
    }
    f->fontname[ENDOFBUFFER] = (char)type;
  }
  if (ap) *ap = f->fontname[ENDOFBUFFER];
  return f->fontname;
}

extern "C" {
// sort raw (non-'*') X font names into perfect order:

static int ultrasort(const void *aa, const void *bb) {
  const char *a = *(char **)aa;
  const char *b = *(char **)bb;

  // sort all non x-fonts at the end:
  if (*a != '-') {
    if (*b == '-') return 1;
    // 2 non-x fonts are matched by "numeric sort"
    int ret = 0;
    for (;;) {
      if (isdigit(*a) && isdigit(*b)) {
	int na = strtol(a, (char **)&a, 10);
	int nb = strtol(b, (char **)&b, 10);
	if (!ret) ret = na-nb;
      } else if (*a != *b) {
	return (*a-*b);
      } else if (!*a) {
	return ret;
      } else {
	a++; b++;
      }
    }
  } else {
    if (*b != '-') return -1;
  }

  // skip the foundry (assumme equal):
  for (a++; *a && *a++!='-';);
  for (b++; *b && *b++!='-';);

  // compare the family and all the attribute words:
  int atype = 0;
  int btype = 0;
  for (int n = 2; n <= 6; n++) {
    int at = attribute(n,a);
    int bt = attribute(n,b);
    if (at < 0) {
      if (bt >= 0) return 1;
      for (;;) {if (*a!=*b) return *a-*b; b++; if (!*a || *a++=='-') break;}
    } else {
      if (bt < 0) return -1;
      a = fl_font_word(a,1); if (*a) a++;
      b = fl_font_word(b,1); if (*b) b++;
      atype |= at; btype |= bt;
    }
  }

  // remember the pixel size:
  int asize = atoi(a);
  int bsize = atoi(b);

  // compare the registry/encoding:
  a = fl_font_word(a,6); if (*a) a++;
  b = fl_font_word(b,6); if (*b) b++;
  if (use_registry(a)) {
    if (!use_registry(b)) return 1;
    int r = strcmp(a,b); if (r) return r;
  } else {
    if (use_registry(b)) return -1;
  }

  if (atype != btype) return atype-btype;
  if (asize != bsize) return asize-bsize;

  // something wrong, just do a string compare...
  return strcmp(*(char**)aa, *(char**)bb);
}
}

// converts a X font name to a standard starname, returns point size:
static int to_canonical(char *to, const char *from, size_t tolen) {
  char* c = fl_find_fontsize((char*)from);
  if (!c) return -1; // no point size found...
  const char* endptr;
  int size = strtol(c,(char**)&endptr,10);
  if (from[0] == '-') {
    // replace the "foundry" with -*-:
    *to++ = '-'; *to++ = '*';
    for (from++; *from && *from != '-'; from++);
    // skip to the registry-encoding:
    endptr = (char*)fl_font_word(endptr,6);
    if (*endptr && !use_registry(endptr+1)) endptr = "";
  }
  int n = c-from;
  // MRS: we want strncpy here, not strlcpy...
  if (n > (int)(tolen - 1)) return -1;
  strncpy(to,from,n);
  to[n++] = '*';
  strlcpy(to+n,endptr, tolen - n);
  return size;
}

static int fl_free_font = FL_FREE_FONT;

Fl_Font Fl::set_fonts(const char* xstarname) {
  if (fl_free_font > FL_FREE_FONT) // already been here
    return (Fl_Font)fl_free_font;
  fl_open_display();
  int xlistsize;
  char buf[20];
  if (!xstarname) {
    strcpy(buf,"-*-"); strcpy(buf+3,fl_encoding);
    xstarname = buf;
  }
  char **xlist = XListFonts(fl_display, xstarname, 10000, &xlistsize);
  if (!xlist) return (Fl_Font)fl_free_font;
  qsort(xlist, xlistsize, sizeof(*xlist), ultrasort);
  int used_xlist = 0;
  for (int i=0; i<xlistsize;) {
    int first_xlist = i;
    const char *p = xlist[i++];
    char canon[1024];
    int size = to_canonical(canon, p, sizeof(canon));
    if (size >= 0) {
      for (;;) { // find all matching fonts:
	if (i >= xlistsize) break;
	const char *q = xlist[i];
	char this_canon[1024];
	if (to_canonical(this_canon, q, sizeof(this_canon)) < 0) break;
	if (strcmp(canon, this_canon)) break;
	i++;
      }
      /*if (*p=='-' || i > first_xlist+1)*/ p = canon;
    }
    int j;
    for (j = 0;; j++) {
      if (j < FL_FREE_FONT) {
	// see if it is one of our built-in fonts:
	// if so, set the list of x fonts, since we have it anyway
	if (fl_fonts[j].name && !strcmp(fl_fonts[j].name, p)) break;
      } else {
	j = fl_free_font++;
	if (p == canon) p = strdup(p); else used_xlist = 1;
	Fl::set_font((Fl_Font)j, p);
	break;
      }
    }
    if (!fl_fonts[j].xlist) {
      fl_fonts[j].xlist = xlist+first_xlist;
      fl_fonts[j].n = -(i-first_xlist);
      used_xlist = 1;
    }
  }
  if (!used_xlist) XFreeFontNames(xlist);
  return (Fl_Font)fl_free_font;
}

int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0
  if (!s->xlist) {
    fl_open_display();
    s->xlist = XListFonts(fl_display, s->name, 100, &(s->n));
    if (!s->xlist) return 0;
  }
  int listsize = s->n; if (listsize<0) listsize = -listsize;
  static int sizes[128];
  int numsizes = 0;
  for (int i = 0; i < listsize; i++) {
    char *q = s->xlist[i];
    char *d = fl_find_fontsize(q);
    if (!d) continue;
    int s = strtol(d,0,10);
    if (!numsizes || sizes[numsizes-1] < s) {
      sizes[numsizes++] = s;
    } else {
      // insert-sort the new size into list:
      int n;
      for (n = numsizes-1; n > 0; n--) if (sizes[n-1] < s) break;
      if (sizes[n] != s) {
	for (int m = numsizes; m > n; m--) sizes[m] = sizes[m-1];
	sizes[n] = s;
	numsizes++;
      }
    }
  }
  sizep = sizes;
  return numsizes;
}

//
// End of "$Id: fl_set_fonts_x.cxx 5190 2006-06-09 16:16:34Z mike $".
//
