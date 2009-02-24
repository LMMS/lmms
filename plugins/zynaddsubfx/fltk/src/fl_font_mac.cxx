//
// "$Id: fl_font_mac.cxx 5651 2007-02-01 20:13:57Z matt $"
//
// MacOS font selection routines for the Fast Light Tool Kit (FLTK).
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

#include <config.h>

Fl_FontSize::Fl_FontSize(const char* name, int Size) {
  next = 0;
#  if HAVE_GL
  listbase = 0;
#  endif
#ifdef __APPLE_QD__
  knowMetrics = 0;
  switch (*name++) {
  case 'I': face = italic; break;
  case 'P': face = italic | bold; break;
  case 'B': face = bold; break;
  default: face = 0; break;
  }
  unsigned char fn[80]; 
  fn[0] = strlen(name); strcpy((char*)(fn+1), name);
  GetFNum(fn, &font);
  size = Size;
  FMInput fIn = { font, size, face, 0, 0, { 1, 1}, { 1, 1} };
  FMOutput *fOut = FMSwapFont(&fIn);
  ascent = fOut->ascent;	//: the following three lines give only temporary aproimations
  descent = fOut->descent;
  for (int i=0; i<256; i++) width[i] = fOut->widMax;
  minsize = maxsize = size;
#elif defined(__APPLE_QUARTZ__)
  knowWidths = 0;
    // OpenGL needs those for its font handling
  q_name = strdup(name);
  size = Size;
  OSStatus err;
    // fill our structure with a few default values
  ascent = Size*3/4;
  descent = Size-ascent;
  q_width = Size*2/3;
  minsize = maxsize = Size;
    // now use ATS to get the actual Glyph size information
  CFStringRef cfname = CFStringCreateWithCString(0L, name, kCFStringEncodingASCII);
  ATSFontRef font = ATSFontFindFromName(cfname, kATSOptionFlagsDefault);
  if (font) {
    ATSFontMetrics m = { 0 };
    ATSFontGetHorizontalMetrics(font, kATSOptionFlagsDefault, &m);
    if (m.avgAdvanceWidth) q_width = int(m.avgAdvanceWidth*Size);
      // playing with the offsets a little to make standard sizes fit
    if (m.ascent) ascent  = int(m.ascent*Size-0.5f);
    if (m.descent) descent = -int(m.descent*Size-1.5f);
  }
  CFRelease(cfname);
    // now we allocate everything needed to render text in this font later
    // get us the default layout and style
  err = ATSUCreateTextLayout(&layout);
  UniChar mTxt[2] = { 65, 0 };
  err = ATSUSetTextPointerLocation(layout, mTxt, kATSUFromTextBeginning, 1, 1);
  err = ATSUCreateStyle(&style);
  err = ATSUSetRunStyle(layout, style, kATSUFromTextBeginning, kATSUToTextEnd);
    // now set the actual font, size and attributes. We also set the font matrix to 
    // render our font up-side-down, so when rendered through our inverted CGContext,
    // text will appear normal again.
  Fixed fsize = IntToFixed(Size);
  ATSUFontID fontID = FMGetFontFromATSFontRef(font);
  static CGAffineTransform font_mx = { 1, 0, 0, -1, 0, 0 };
  ATSUAttributeTag sTag[] = { kATSUFontTag, kATSUSizeTag, kATSUFontMatrixTag };
  ByteCount sBytes[] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(CGAffineTransform) };
  ATSUAttributeValuePtr sAttr[] = { &fontID, &fsize, &font_mx };
  err = ATSUSetAttributes(style, 3, sTag, sBytes, sAttr);
    // next, make sure that Quartz will only render at integer coordinates
  ATSLineLayoutOptions llo = kATSLineUseDeviceMetrics|kATSLineDisableAllLayoutOperations;
  ATSUAttributeTag aTag[] = { kATSULineLayoutOptionsTag };
  ByteCount aBytes[] = { sizeof(ATSLineLayoutOptions) };
  ATSUAttributeValuePtr aAttr[] = { &llo };
  err = ATSUSetLineControls (layout, kATSUFromTextBeginning, 1, aTag, aBytes, aAttr);
    // now we are finally ready to measure some letter to get the bounding box
  Fixed bBefore, bAfter, bAscent, bDescent;
  err = ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, 1, &bBefore, &bAfter, &bAscent, &bDescent);
    // Requesting a certain height font on Mac does not guarantee that ascent+descent
    // equal the requested height. fl_height will reflect the actual height that we got.
    // The font "Apple Chancery" is a pretty extreme example of overlapping letters.
  float fa = -FixedToFloat(bAscent), fd = -FixedToFloat(bDescent);
  if (fa>0.0f && fd>0.0f) {
    //float f = Size/(fa+fd);
    ascent = fa; //int(fa*f+0.5f);
    descent = fd; //Size - ascent;
  }
  int w = FixedToInt(bAfter);
  if (w)  
    q_width = FixedToInt(bAfter);
#endif
}

Fl_FontSize* fl_fontsize = 0L;

Fl_FontSize::~Fl_FontSize() {
/*
#if HAVE_GL
 // ++ todo: remove OpenGL font alocations
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
  */
  if (this == fl_fontsize) fl_fontsize = 0;
#ifdef __APPLE_QUARTZ__
  ATSUDisposeTextLayout(layout);
  ATSUDisposeStyle(style);
#endif
}

////////////////////////////////////////////////////////////////

static Fl_Fontdesc built_in_table[] = {
#ifdef __APPLE_QD__
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
{" Chicago"},
{"BChicago"},
{" Webdings"},
#elif defined(__APPLE_QUARTZ__)
{"Arial"},
{"Arial Bold"},
{"Arial Italic"},
{"Arial Bold Italic"},
{"Courier New"},
{"Courier New Bold"},
{"Courier New Italic"},
{"Courier New Bold Italic"},
{"Times New Roman"},
{"Times New Roman Bold"},
{"Times New Roman Italic"},
{"Times New Roman Bold Italic"},
{"Symbol"},
{"Monaco"},
{"Andale Mono"}, // there is no bold Monaco font on standard Mac
{"Webdings"},
#endif
};

#ifdef __APPLE_QUARTZ__
static UniChar utf16lut[128] = {
  0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 
  0x00e0, 0x00e2, 0x00e4, 0x00e3, 0x00e5, 0x00e7, 0x00e9, 0x00e8, 
  0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
  0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 
  0x2020, 0x00b0, 0x00a2, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x00df, 
  0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
  0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 
  0x220f, 0x03c0, 0x222b, 0x00aa, 0x00ba, 0x03a9, 0x00e6, 0x00f8, 
  0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
  0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 
  0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x25ca, 
  0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02, 
  0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 
  0x00cb, 0x00c8, 0x00cd, 0x00ce, 0x00cf, 0x00cc, 0x00d3, 0x00d4, 
  0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
  0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7, 
};
static UniChar *utf16buf = 0;
static int utf16len = 0;
UniChar *fl_macToUtf16(const char *txt, int len)
{
  if ((len+1)>utf16len) {
    utf16len = len+100;
    free(utf16buf);
    utf16buf = (UniChar*)malloc((utf16len+1)*sizeof(UniChar));
  }
  int i;
  unsigned char c;
  const unsigned char *src = (unsigned char const*)txt;
  UniChar *dst = utf16buf;
  for (i=0; i<len; i++) {
    c = *src++;
    *dst++ =(c<128) ? c : utf16lut[c-128];
  }
  *dst = 0;
  return utf16buf;
}
#endif

Fl_Fontdesc* fl_fonts = built_in_table;

void fl_font(Fl_FontSize* s) {
  fl_fontsize = s;
#ifdef __APPLE_QD__
  if (fl_window) SetPort( GetWindowPort(fl_window) );
  TextFont(fl_fontsize->font);	//: select font into current QuickDraw GC
  TextFace(fl_fontsize->face);
  TextSize(fl_fontsize->size);
  if (!fl_fontsize->knowMetrics) {	//: get the true metrics for the currnet GC 
                                        //: (fails on multiple monitors with different dpi's!)
    FontInfo fi; GetFontInfo(&fi);
    fl_fontsize->ascent = fi.ascent;
    fl_fontsize->descent = fi.descent;
    FMetricRec mr; FontMetrics(&mr);
    short *f = (short*)*mr.wTabHandle; //: get the char size table
    for (int i=0; i<256; i++) fl_fontsize->width[i] = f[2*i];
    fl_fontsize->knowMetrics = 1;
  }
#elif defined(__APPLE_QUARTZ__)
  // we will use fl_fontsize later to access the required style and layout
#else
# error : need to defined either Quartz or Quickdraw
#endif
}

static Fl_FontSize* find(int fnum, int size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_FontSize* f;
  for (f = s->first; f; f = f->next)
    if (f->minsize <= size && f->maxsize >= size) return f;
  f = new Fl_FontSize(s->name, size);
  f->next = s->first;
  s->first = f;
  return f;
}

////////////////////////////////////////////////////////////////
// Public interface:

int fl_font_ = 0;
int fl_size_ = 0;

void fl_font(int fnum, int size) {
  if (fnum==-1) {
    fl_font_ = 0; 
    fl_size_ = 0;
    return;
  }
  fl_font_ = fnum;
  fl_size_ = size;
  fl_font(find(fnum, size));
}

int fl_height() {
  if (fl_fontsize) return fl_fontsize->ascent+fl_fontsize->descent;
  else return -1;
}

int fl_descent() {
  if (fl_fontsize) return fl_fontsize->descent;
  else return -1;
}

double fl_width(const char* txt, int n) {
#ifdef __APPLE_QD__
  return (double)TextWidth( txt, 0, n );
#else
  if (!fl_fontsize) {
    fl_font(0, 12); // avoid a crash!
    if (!fl_fontsize)
      return 8*n; // user must select a font first!
  }
  if (!fl_fontsize->knowWidths) {
    if (!fl_gc) {
      Fl_Window *w = Fl::first_window();
      if (w) w->make_current();
      if (!fl_gc) {
        if (fl_fontsize) return fl_fontsize->q_width*n;
        return 8*n;
        // We fall back to some internal QuickDraw port.
        // The result should be the same.
      }
    }
    char buf[2];
    for (int i=0; i<256; i++) {
      OSStatus err;
      buf[0] = (char)i;
        // convert to UTF-16 first
      UniChar *uniStr = fl_macToUtf16(buf, 1);
        // now collect our ATSU resources
      ATSUTextLayout layout = fl_fontsize->layout;
        // activate the current GC
      ByteCount iSize = sizeof(CGContextRef);
      ATSUAttributeTag iTag = kATSUCGContextTag;
      ATSUAttributeValuePtr iValuePtr=&fl_gc;
      ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);
        // now measure the bounding box
      err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, 1, 1);
      Fixed bBefore, bAfter, bAscent, bDescent;
      err = ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, 1, &bBefore, &bAfter, &bAscent, &bDescent);
      fl_fontsize->width[i] = FixedToInt(bAfter);
    }
    fl_fontsize->knowWidths = 1;
  }
  int len = 0;
  const char *src = txt;
  for (int j=0; j<n; j++) {
    unsigned char c = *src++;
    len += fl_fontsize->width[c];
  }
  return len;
#endif
}

double fl_width(uchar c) {
  return fl_width((const char*)(&c), 1);
}

void fl_draw(const char *str, int n, float x, float y);

void fl_draw(const char* str, int n, int x, int y) {
#ifdef __APPLE_QD__
  MoveTo(x, y);
  DrawText((const char *)str, 0, n);
#elif defined(__APPLE_QUARTZ__)
  fl_draw(str, n, (float)x-0.0f, (float)y-0.5f);
#else
#  error : neither Quartz no Quickdraw chosen
#endif
}

void fl_draw(const char *str, int n, float x, float y) {
#ifdef __APPLE_QD__
  fl_draw(str, n, (int)x, (int)y);
#elif defined(__APPLE_QUARTZ__)
  OSStatus err;
    // convert to UTF-16 first 
  UniChar *uniStr = fl_macToUtf16(str, n);
    // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;

  ByteCount iSize = sizeof(CGContextRef);
  ATSUAttributeTag iTag = kATSUCGContextTag;
  ATSUAttributeValuePtr iValuePtr=&fl_gc;
  ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x), FloatToFixed(y));
#else
#  error : neither Quartz no Quickdraw chosen
#endif
}

//
// End of "$Id: fl_font_mac.cxx 5651 2007-02-01 20:13:57Z matt $".
//
