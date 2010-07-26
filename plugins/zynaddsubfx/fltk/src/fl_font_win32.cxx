//
// "$Id: fl_font_win32.cxx 7652 2010-06-21 15:49:45Z manolo $"
//
// WIN32 font selection routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2009 by Bill Spitzak and others.
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

#include <FL/Fl_Printer.H>

static int fl_angle_ = 0;

#ifndef FL_DOXYGEN
Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize size) {
  int weight = FW_NORMAL;
  int italic = 0;
  switch (*name++) {
  case 'I': italic = 1; break;
  case 'P': italic = 1;
  case 'B': weight = FW_BOLD; break;
  case ' ': break;
  default: name--;
  }
  fid = CreateFont(
    -size, // negative makes it use "char size"
    0,	            // logical average character width
    fl_angle_*10,	            // angle of escapement
    fl_angle_*10,	            // base-line orientation angle
    weight,
    italic,
    FALSE,	        // underline attribute flag
    FALSE,	        // strikeout attribute flag
    DEFAULT_CHARSET,    // character set identifier
    OUT_DEFAULT_PRECIS,	// output precision
    CLIP_DEFAULT_PRECIS,// clipping precision
    DEFAULT_QUALITY,	// output quality
    DEFAULT_PITCH,	// pitch and family
    name	        // pointer to typeface name string
    );
  angle = fl_angle_;
  if (!fl_gc) fl_GetDC(0);
  SelectObject(fl_gc, fid);
  GetTextMetrics(fl_gc, &metr);
//  BOOL ret = GetCharWidthFloat(fl_gc, metr.tmFirstChar, metr.tmLastChar, font->width+metr.tmFirstChar);
// ...would be the right call, but is not implemented into Window95! (WinNT?)
  //GetCharWidth(fl_gc, 0, 255, width);
  int i;
  for (i = 0; i < 64; i++) width[i] = NULL;
#if HAVE_GL
  listbase = 0;
  for (i = 0; i < 64; i++) glok[i] = 0;
#endif
  minsize = maxsize = size;
}

Fl_Font_Descriptor* fl_fontsize;

Fl_Font_Descriptor::~Fl_Font_Descriptor() {
#if HAVE_GL
// Delete list created by gl_draw().  This is not done by this code
// as it will link in GL unnecessarily.  There should be some kind
// of "free" routine pointer, or a subclass?
// if (listbase) {
//  int base = font->min_char_or_byte2;
//  int size = font->max_char_or_byte2-base+1;
//  int base = 0; int size = 256;
//  glDeleteLists(listbase+base,size);
// }
#endif
  if (this == fl_fontsize) fl_fontsize = 0;
  DeleteObject(fid);
  int i;
  for (i = 0; i < 64; i++) free(width[i]);
}

////////////////////////////////////////////////////////////////

// WARNING: if you add to this table, you must redefine FL_FREE_FONT
// in Enumerations.H & recompile!!
static Fl_Fontdesc built_in_table[] = {
{" Arial"},
{"BArial"},
{"IArial"},
{"PArial"},
{" Courier New"},
{"BCourier New"},
{"ICourier New"},
{"PCourier New"},
{" Times New Roman"},
{"BTimes New Roman"},
{"ITimes New Roman"},
{"PTimes New Roman"},
{" Symbol"},
{" Terminal"},
{"BTerminal"},
{" Wingdings"},
};

Fl_Fontdesc* fl_fonts = built_in_table;

static Fl_Font_Descriptor* find(Fl_Font fnum, Fl_Fontsize size, int angle) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_Font_Descriptor* f;
  for (f = s->first; f; f = f->next)
    if (f->minsize <= size && f->maxsize >= size && f->angle == angle) return f;
  f = new Fl_Font_Descriptor(s->name, size);
  f->next = s->first;
  s->first = f;
  return f;
}

////////////////////////////////////////////////////////////////
// Public interface:

Fl_Font fl_font_ = 0;
Fl_Fontsize fl_size_ = 0;
//static HDC font_gc;

void fl_font(Fl_Font fnum, Fl_Fontsize size, int angle) {
  if (fnum==-1) { // just make sure that we will load a new font next time
    fl_font_ = 0; fl_size_ = 0; fl_angle_ = 0;
    return;
  }
  if (fnum == fl_font_ && size == fl_size_ && angle == fl_angle_) return;
  fl_font_ = fnum; fl_size_ = size; fl_angle_ = angle;
  fl_fontsize = find(fnum, size, angle);
}

void Fl_Graphics_Driver::font(Fl_Font fnum, Fl_Fontsize size) {
  fl_font(fnum, size, 0);
}

int fl_height() {
  if (fl_fontsize) return (fl_fontsize->metr.tmAscent + fl_fontsize->metr.tmDescent);
  else return -1;
}

int fl_descent() {
  if (fl_fontsize) return fl_fontsize->metr.tmDescent;
  else return -1;
}

// Unicode string buffer
static xchar *wstr = NULL;
static int wstr_len    = 0;


double fl_width(const char* c, int n) {
  int i = 0;
  if (!fl_fontsize) return -1.0;
  double w = 0.0;
  char *end = (char *)&c[n];
  while (i < n) {
    unsigned int ucs;
//  int l = fl_utf2ucs((const unsigned char*)c + i, n - i, &ucs);
    int l;
    ucs = fl_utf8decode((const char*)(c + i), end, &l);
//  if (l < 1) l = 1;
    i += l;
    if (!fl_nonspacing(ucs)) {
      w += fl_width(ucs);
    }
  }
  return w;
}

double fl_width(unsigned int c) {
  unsigned int r;
  r = (c & 0xFC00) >> 10;
  if (!fl_fontsize->width[r]) {
    SelectObject(fl_gc, fl_fontsize->fid);
    fl_fontsize->width[r] = (int*) malloc(sizeof(int) * 0x0400);
    SIZE s;
    unsigned short i = 0, ii = r * 0x400;
    // The following code makes a best effort attempt to obtain a valid fl_gc.
    // If no fl_gc is available at the time we call fl_width(), then we first
    // try to obtain a gc from the first fltk window.
    // If that is null then we attempt to obtain the gc from the current screen
    // using (GetDC(NULL)).
    // This should resolve STR #2086
    HDC gc = fl_gc;
    HWND hWnd = 0;
    if (!gc) { // We have no valid gc, try and obtain one
	// Use our first fltk window, or fallback to using the screen via GetDC(NULL)
	hWnd = Fl::first_window() ? fl_xid(Fl::first_window()) : NULL;
	gc = GetDC(hWnd);
    }
    if (!gc)
	Fl::fatal("Invalid graphic context: fl_width() failed because no valid HDC was found!");
    for (; i < 0x400; i++) {
      GetTextExtentPoint32W(gc, (WCHAR*)&ii, 1, &s);
      fl_fontsize->width[r][i] = s.cx;
      ii++;
    }
    if (gc && gc!=fl_gc) ReleaseDC(hWnd, gc);
  }
  return (double) fl_fontsize->width[r][c & 0x03FF];
}

/* Add function pointer to allow us to access GetGlyphIndicesW on systems that have it,
 * without crashing on systems that do not. */
/* DWORD WINAPI GetGlyphIndicesW(HDC,LPCWSTR,int,LPWORD,DWORD) */
typedef DWORD (WINAPI* fl_GetGlyphIndices_func)(HDC,LPCWSTR,int,LPWORD,DWORD);

static fl_GetGlyphIndices_func fl_GetGlyphIndices = NULL; // used to hold a proc pointer for GetGlyphIndicesW
static int have_loaded_GetGlyphIndices = 0; // Set this non-zero once we have tried to load GetGlyphIndices

// Function that tries to dynamically load GetGlyphIndicesW at runtime
static void GetGlyphIndices_init() {
  // Since not all versions of Windows include GetGlyphIndicesW support,
  // we do a run-time check for the required function.
  HMODULE hMod = GetModuleHandle("GDI32.DLL");
  if (hMod) {
    // check that GetGlyphIndicesW is available
    fl_GetGlyphIndices = (fl_GetGlyphIndices_func)GetProcAddress(hMod, "GetGlyphIndicesW");
  }
  have_loaded_GetGlyphIndices = -1; // set this non-zero when we have attempted to load GetGlyphIndicesW
} // GetGlyphIndices_init function

static void on_printer_extents_update(int &dx, int &dy, int &w, int &h)
// converts text extents from device coords to logical coords
{
  POINT pt[3] = { {0, 0}, {dx, dy}, {dx+w, dy+h} };
  DPtoLP(fl_gc, pt, 3);
  w = pt[2].x - pt[1].x;
  h = pt[2].y - pt[1].y;
  dx = pt[1].x - pt[0].x;
  dy = pt[1].y - pt[0].y;
}

// if printer context, extents shd be converted to logical coords
#define EXTENTS_UPDATE(x,y,w,h) \
  if (Fl_Surface_Device::surface()->type() == Fl_Printer::device_type) { on_printer_extents_update(x,y,w,h); }

static unsigned short *ext_buff = NULL; // UTF-16 converted version of input UTF-8 string
static unsigned wc_len = 0; // current string buffer dimension
static WORD *gi = NULL; // glyph indices array
// Function to determine the extent of the "inked" area of the glyphs in a string
void fl_text_extents(const char *c, int n, int &dx, int &dy, int &w, int &h) {
  if (!fl_fontsize) {
    w = 0; h = 0;
    dx = dy = 0;
    return;
  }
  static const MAT2 matrix = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
  GLYPHMETRICS metrics;
  int maxw = 0, maxh = 0, dh;
  int minx = 0, miny = -999999;
  unsigned len = 0, idx = 0;
  HWND hWnd = 0;

  // Have we loaded the GetGlyphIndicesW function yet?
  if (have_loaded_GetGlyphIndices == 0) {
    GetGlyphIndices_init();
  }
  // Do we have a usable GetGlyphIndices function?
  if(!fl_GetGlyphIndices) goto exit_error; // No GetGlyphIndices function, use fallback mechanism instead

  // The following code makes a best effort attempt to obtain a valid fl_gc.
  // See description in fl_width() above for an explanation.
  if (!fl_gc) { // We have no valid gc, try and obtain one
	// Use our first fltk window, or fallback to using the screen via GetDC(NULL)
	hWnd = Fl::first_window() ? fl_xid(Fl::first_window()) : NULL;
	fl_gc = GetDC(hWnd);
  }
  if (!fl_gc)goto exit_error; // no valid gc, attempt to use fallback measure

  // now convert the string to WCHAR and measure it
  len = fl_utf8toUtf16(c, n, ext_buff, wc_len);
  if(len >= wc_len) {
    if(ext_buff) {delete [] ext_buff;}
    if(gi) {delete [] gi;}
	wc_len = len + 64;
    ext_buff = new unsigned short[wc_len];
	gi = new WORD[wc_len];
    len = fl_utf8toUtf16(c, n, ext_buff, wc_len);
  }
  SelectObject(fl_gc, fl_fontsize->fid);

  if (fl_GetGlyphIndices(fl_gc, (WCHAR*)ext_buff, len, gi, 0) == GDI_ERROR) {
    // some error occured here - just return fl_measure values?
    goto exit_error;
  }

  // now we have the glyph array we measure each glyph in turn...
  for(idx = 0; idx < len; idx++){
    if (GetGlyphOutlineW (fl_gc, gi[idx], GGO_METRICS | GGO_GLYPH_INDEX,
					      &metrics, 0, NULL, &matrix) == GDI_ERROR) {
                    goto exit_error;
    }
    maxw += metrics.gmCellIncX;
	if(idx == 0) minx = metrics.gmptGlyphOrigin.x;
    dh = metrics.gmBlackBoxY - metrics.gmptGlyphOrigin.y;
	if(dh > maxh) maxh = dh;
	if(miny < metrics.gmptGlyphOrigin.y) miny = metrics.gmptGlyphOrigin.y;
  }

  // for the last cell, we only want the bounding X-extent, not the glyphs increment step
  maxw = maxw - metrics.gmCellIncX + metrics.gmBlackBoxX + metrics.gmptGlyphOrigin.x;
  w = maxw - minx;
  h = maxh + miny;
  dx = minx;
  dy = -miny;
  EXTENTS_UPDATE(dx, dy, w, h);
  return; // normal exit

exit_error:
  // some error here - just return fl_measure values
  w = (int)fl_width(c, n);
  h = fl_height();
  dx = 0;
  dy = fl_descent() - h;
  EXTENTS_UPDATE(dx, dy, w, h);
  return;
} // fl_text_extents

void Fl_Graphics_Driver::draw(const char* str, int n, int x, int y) {
  int i = 0;
  int lx = 0;
  char *end = (char *)&str[n];
  COLORREF oldColor = SetTextColor(fl_gc, fl_RGB());
   SelectObject(fl_gc, fl_fontsize->fid);
  while (i < n) {
    unsigned int u;
	unsigned int u1;
    unsigned short ucs;
//  int l = fl_utf2ucs((const unsigned char*)str + i, n - i, &u);
    int l;
    u = fl_utf8decode((const char*)(str + i), end, &l);
    if (u1 = fl_nonspacing(u)) {
      x -= lx;
	  u = u1;
    } else {
      lx = (int) fl_width(u);
    }
    ucs = u;
    if (l < 1) l = 1;
    i += l;
    TextOutW(fl_gc, x, y, (WCHAR*)&ucs, 1);
    x += lx;
  }
  SetTextColor(fl_gc, oldColor);
}

void Fl_Graphics_Driver::draw(int angle, const char* str, int n, int x, int y) {
  fl_font(fl_font_, fl_size_, angle);
//  fl_draw(str, n, (int)x, (int)y);
  int i = 0, i2=0;
  char *end = (char *)&str[n];
  COLORREF oldColor = SetTextColor(fl_gc, fl_RGB());
  SelectObject(fl_gc, fl_fontsize->fid);
  //unsigned short ucs[n]; //only GCC, but not MSVC
  unsigned short* ucs = new unsigned short[n];
  while (i < n) {
    unsigned int u;
    int l;
    u = fl_utf8decode((const char*)(str + i), end, &l);
    ucs[i2] = u;
    if (l < 1) l = 1;
    i += l;
    ++i2;
  }
  TextOutW(fl_gc, x, y, (WCHAR*)ucs, i2);
  delete[] ucs;
  SetTextColor(fl_gc, oldColor);
  fl_font(fl_font_, fl_size_);
}

void Fl_Graphics_Driver::rtl_draw(const char* c, int n, int x, int y) {
  int wn;
  int i = 0;
  int lx = 0;
//  if (n > wstr_len) {
//    wstr = (xchar*) realloc(wstr, sizeof(xchar) * (n + 1));
//    wstr_len = n;
//  }
//wn = fl_utf2unicode((const unsigned char *)c, n, wstr);
  wn = fl_utf8toUtf16(c, n, (unsigned short*)wstr, wstr_len);
  if(wn >= wstr_len) {
    wstr = (xchar*) realloc(wstr, sizeof(xchar) * (wn + 1));
    wstr_len = wn + 1;
    wn = fl_utf8toUtf16(c, n, (unsigned short*)wstr, wstr_len);
  }

  COLORREF oldColor = SetTextColor(fl_gc, fl_RGB());
  SelectObject(fl_gc, fl_fontsize->fid);
  while (i < wn) {
    lx = (int) fl_width(wstr[i]);
    x -= lx;
    TextOutW(fl_gc, x, y, (WCHAR*)wstr + i, 1);
    if (fl_nonspacing(wstr[i])) {
      x += lx;
    }
    i++;
  }
  SetTextColor(fl_gc, oldColor);
}
#endif
//
// End of "$Id: fl_font_win32.cxx 7652 2010-06-21 15:49:45Z manolo $".
//
