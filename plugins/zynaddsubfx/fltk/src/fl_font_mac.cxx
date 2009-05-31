//
// "$Id: fl_font_mac.cxx 6779 2009-04-24 09:28:30Z yuri $"
//
// MacOS font selection routines for the Fast Light Tool Kit (FLTK).
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

#include <config.h>

/* from fl_utf.c */
extern unsigned fl_utf8toUtf16(const char* src, unsigned srclen, unsigned short* dst, unsigned dstlen);

// if no font has been selected yet by the user, get one.
#define check_default_font() {if (!fl_fontsize) fl_font(0, 12);}

Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize Size) {
  next = 0;
#  if HAVE_GL
  listbase = 0;
#  endif

//  knowWidths = 0;
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
	// say that our passed-in name is encoded as UTF-8, since this works for plain ASCII names too...
  CFStringRef cfname = CFStringCreateWithCString(0L, name, kCFStringEncodingUTF8);
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
//  ATSUFontID fontID = FMGetFontFromATSFontRef(font);
  ATSUFontID fontID;
  ATSUFindFontFromName(name, strlen(name), kFontFullName, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage, &fontID);

  // draw the font upside-down... Compensate for fltk/OSX origin differences
  static CGAffineTransform font_mx = { 1, 0, 0, -1, 0, 0 };
  ATSUAttributeTag sTag[] = { kATSUFontTag, kATSUSizeTag, kATSUFontMatrixTag };
  ByteCount sBytes[] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(CGAffineTransform) };
  ATSUAttributeValuePtr sAttr[] = { &fontID, &fsize, &font_mx };
  err = ATSUSetAttributes(style, 3, sTag, sBytes, sAttr);
    // next, make sure that Quartz will only render at integer coordinates
  ATSLineLayoutOptions llo = kATSLineUseDeviceMetrics | kATSLineDisableAllLayoutOperations;
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

# define ENABLE_TRANSIENT_FONTS  1

# ifdef ENABLE_TRANSIENT_FONTS
  // Now, by way of experiment, try enabling Transient Font Matching, this will
  // cause ATSU to find a suitable font to render any chars the current font can't do...
  ATSUSetTransientFontMatching (layout, true);
# endif
}

Fl_Font_Descriptor* fl_fontsize = 0L;

Fl_Font_Descriptor::~Fl_Font_Descriptor() {
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
  ATSUDisposeTextLayout(layout);
  ATSUDisposeStyle(style);
}

////////////////////////////////////////////////////////////////

static Fl_Fontdesc built_in_table[] = {
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
};

static UniChar *utfWbuf = 0;
static unsigned utfWlen = 0;

static UniChar *mac_Utf8_to_Utf16(const char *txt, int len, int *new_len)
{
  unsigned wlen = fl_utf8toUtf16(txt, len, (unsigned short*)utfWbuf, utfWlen);
  if(wlen >= utfWlen)
  {
    utfWlen = wlen + 100;
	if(utfWbuf) free(utfWbuf);
    utfWbuf = (UniChar*)malloc((utfWlen)*sizeof(UniChar));
	wlen = fl_utf8toUtf16(txt, len, (unsigned short*)utfWbuf, utfWlen);
  }
  *new_len = wlen;
  return utfWbuf;
} // mac_Utf8_to_Utf16

Fl_Fontdesc* fl_fonts = built_in_table;

void fl_font(Fl_Font_Descriptor* s) {
  fl_fontsize = s;
#if defined(__APPLE_QUARTZ__)
  // we will use fl_fontsize later to access the required style and layout
#else
# error : need to defined either Quartz or Quickdraw
#endif
}

static Fl_Font_Descriptor* find(Fl_Font fnum, Fl_Fontsize size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_Font_Descriptor* f;
  for (f = s->first; f; f = f->next)
    if (f->minsize <= size && f->maxsize >= size) return f;
  f = new Fl_Font_Descriptor(s->name, size);
  f->next = s->first;
  s->first = f;
  return f;
}

////////////////////////////////////////////////////////////////
// Public interface:

Fl_Font fl_font_ = 0;
Fl_Fontsize fl_size_ = 0;


void fl_font(Fl_Font fnum, Fl_Fontsize size) {
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
  check_default_font();
  if (fl_fontsize) return fl_fontsize->ascent+fl_fontsize->descent;
  else return -1;
}

int fl_descent() {
  check_default_font();
  if (fl_fontsize) 
    return fl_fontsize->descent+1;
  else return -1;
}

double fl_width(const UniChar* txt, int n) {
  check_default_font();
  if (!fl_fontsize) {
    check_default_font(); // avoid a crash!
    if (!fl_fontsize)
      return 8*n; // user must select a font first!
  }
      OSStatus err;
  Fixed bBefore, bAfter, bAscent, bDescent;
  ATSUTextLayout layout;
  ByteCount iSize;
  ATSUAttributeTag iTag;
  ATSUAttributeValuePtr iValuePtr;

// Here's my ATSU text measuring attempt... This seems to do the Right Thing
  // now collect our ATSU resources and measure our text string
  layout = fl_fontsize->layout;
        // activate the current GC
  iSize = sizeof(CGContextRef);
  iTag = kATSUCGContextTag;
  iValuePtr = &fl_gc;
      ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);
        // now measure the bounding box
  err = ATSUSetTextPointerLocation(layout, txt, kATSUFromTextBeginning, n, n);
  err = ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, n, &bBefore, &bAfter, &bAscent, &bDescent);
  // If err is OK then return length, else return 0. Or something...
  int len = FixedToInt(bAfter);
  return len;
}

double fl_width(const char* txt, int n) {
  int wc_len = n;
  UniChar *uniStr = mac_Utf8_to_Utf16(txt, n, &wc_len);
  return fl_width(uniStr, wc_len);
}

double fl_width(uchar c) {
  return fl_width((const char*)(&c), 1);
}

double fl_width(unsigned int wc) {
  return fl_width((const UniChar*)(&wc), 1);
}

// text extent calculation
void fl_text_extents(const UniChar* txt, int n, int &dx, int &dy, int &w, int &h) {
  if (!fl_fontsize) {
    check_default_font(); // avoid a crash!
    if (!fl_fontsize)
      w = 8.0 * n; // user must select a font first!
      h = 8.0;
      return;
  }
  OSStatus err;
  ATSUTextLayout layout;
  ByteCount iSize;
  ATSUAttributeTag iTag;
  ATSUAttributeValuePtr iValuePtr;

// Here's my ATSU text measuring attempt... This seems to do the Right Thing
  // now collect our ATSU resources and measure our text string
  layout = fl_fontsize->layout;
        // activate the current GC
  iSize = sizeof(CGContextRef);
  iTag = kATSUCGContextTag;
  iValuePtr = &fl_gc;
      ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);
        // now measure the bounding box
  err = ATSUSetTextPointerLocation(layout, txt, kATSUFromTextBeginning, n, n);
  Rect bbox;
  err = ATSUMeasureTextImage(layout, kATSUFromTextBeginning, n, 0, 0, &bbox);
  w = bbox.right - bbox.left;
  h = bbox.bottom - bbox.top;
  dx = bbox.left;
  dy = -bbox.bottom;
//printf("r: %d l: %d t: %d b: %d w: %d h: %d\n", bbox.right, bbox.left, bbox.top, bbox.bottom, w, h);
  return;
} // fl_text_extents

void fl_text_extents(const char *c, int n, int &dx, int &dy, int &w, int &h) {
  int wc_len = n;
  UniChar *uniStr = mac_Utf8_to_Utf16(c, n, &wc_len);
  fl_text_extents(uniStr, wc_len, dx, dy, w, h);
} // fl_text_extents


void fl_draw(const char *str, int n, float x, float y);

void fl_draw(const char* str, int n, int x, int y) {
  fl_draw(str, n, (float)x-0.0f, (float)y-0.5f);
}

void fl_draw(const char *str, int n, float x, float y) {
  OSStatus err;
    // convert to UTF-16 first
  UniChar *uniStr = mac_Utf8_to_Utf16(str, n, &n);
  
  // avoid a crash if no font has been selected by user yet !
  check_default_font();
  // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;

  ByteCount iSize = sizeof(CGContextRef);
  ATSUAttributeTag iTag = kATSUCGContextTag;
  ATSUAttributeValuePtr iValuePtr=&fl_gc;
  ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x), FloatToFixed(y));
}

void fl_draw(int angle, const char *str, int n, int x, int y) {
  OSStatus err;
    // convert to UTF-16 first
  UniChar *uniStr = mac_Utf8_to_Utf16(str, n, &n);
  
  // avoid a crash if no font has been selected by user yet !
  check_default_font();
  // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;

  Fixed ang = IntToFixed(-angle);
  ByteCount iSize[] = {sizeof(Fixed), sizeof(CGContextRef)};
  ATSUAttributeTag iTag[] = {kATSULineRotationTag, kATSUCGContextTag};
  ATSUAttributeValuePtr aAttr[] = { &ang,  &fl_gc};
  ATSUSetLayoutControls(layout, 2, iTag, iSize, aAttr);

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x), FloatToFixed(y));
  //restore layout baseline
  ang = IntToFixed(0);
  ATSUSetLayoutControls(layout, 2, iTag, iSize, aAttr);
}

void fl_rtl_draw(const char* c, int n, int x, int y) {
// I guess with ATSU the thing to do is force the layout mode to RTL and let ATSU draw the text...
  double offs = fl_width(c, n);
  OSStatus err;
  // convert to UTF-16 first
  UniChar *uniStr = mac_Utf8_to_Utf16(c, n, &n);
  // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;
  // reverse the layout direction
  ATSUAttributeTag llo = kATSURightToLeftBaseDirection; // layout option
  ByteCount iSize[] = {sizeof(ATSUAttributeTag), sizeof(CGContextRef)};
  ATSUAttributeTag iTag[] = {kATSULineDirectionTag, kATSUCGContextTag};
  ATSUAttributeValuePtr aAttr[] = { &llo,  &fl_gc};
  ATSUSetLayoutControls (layout, 2, iTag, iSize, aAttr );

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x-offs), FloatToFixed(y));
}

//
// End of "$Id: fl_font_mac.cxx 6779 2009-04-24 09:28:30Z yuri $".
//
