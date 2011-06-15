//
// "$Id: fl_font_mac.cxx 8597 2011-04-17 13:18:55Z ianmacarthur $"
//
// MacOS font selection routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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

static CGAffineTransform font_mx = { 1, 0, 0, -1, 0, 0 };
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
static CFMutableDictionaryRef attributes = NULL;
#endif

Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize Size) {
  next = 0;
#  if HAVE_GL
  listbase = 0;
#  endif

//  knowWidths = 0;
    // OpenGL needs those for its font handling
  q_name = strdup(name);
  size = Size;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if (fl_mac_os_version >= 100500) {//unfortunately, CTFontCreateWithName != NULL on 10.4 also!
  CFStringRef str = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
  fontref = CTFontCreateWithName(str, size, NULL);
  CGGlyph glyph[2];
  const UniChar A[2]={'W','.'};
  CTFontGetGlyphsForCharacters(fontref, A, glyph, 2);
  CGSize advances[2];
  double w;
  CTFontGetAdvancesForGlyphs(fontref, kCTFontHorizontalOrientation, glyph, advances, 2);
  w = advances[0].width;
  if ( abs(advances[0].width - advances[1].width) < 1E-2 ) {//this is a fixed-width font
    // slightly rescale fixed-width fonts so the character width has an integral value
    CFRelease(fontref);
    CGFloat fsize = size / ( w/floor(w + 0.5) );
    fontref = CTFontCreateWithName(str, fsize, NULL);
    w = CTFontGetAdvancesForGlyphs(fontref, kCTFontHorizontalOrientation, glyph, NULL, 1);
  }
  CFRelease(str);
  ascent = (short)(CTFontGetAscent(fontref) + 0.5);
  descent = (short)(CTFontGetDescent(fontref) + 0.5);
  q_width = w + 0.5;
  for (unsigned i = 0; i < sizeof(width)/sizeof(float*); i++) width[i] = NULL;
  if (!attributes) {
    static CFNumberRef zero_ref;
    float zero = 0.;
    zero_ref = CFNumberCreate(NULL, kCFNumberFloat32Type, &zero);
    // deactivate kerning for all fonts, so that string width = sum of character widths
    // which allows fast fl_width() implementation.
    attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
					   3,
					   &kCFTypeDictionaryKeyCallBacks,
					   &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue (attributes, kCTKernAttributeName, zero_ref);
  }
  if (ascent == 0) { // this may happen with some third party fonts
    CFDictionarySetValue (attributes, kCTFontAttributeName, fontref);
    CFAttributedStringRef mastr = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("Wj"), attributes);
    CTLineRef ctline = CTLineCreateWithAttributedString(mastr);
    CFRelease(mastr);
    CGFloat fascent, fdescent;
    CTLineGetTypographicBounds(ctline, &fascent, &fdescent, NULL);
    CFRelease(ctline);
    ascent = (short)(fascent + 0.5);
    descent = (short)(fdescent + 0.5);
    }
}
else {
#endif
#if ! __LP64__
  OSStatus err;
    // fill our structure with a few default values
  ascent = Size*3/4;
  descent = Size-ascent;
  q_width = Size*2/3;
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
    ascent = int(fa); //int(fa*f+0.5f);
    descent = int(fd); //Size - ascent;
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
#endif//__LP64__
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  }
#endif
}

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
  if (this == fl_graphics_driver->font_descriptor()) fl_graphics_driver->font_descriptor(NULL);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  if (fl_mac_os_version >= 100500)  {
    CFRelease(fontref);
    for (unsigned i = 0; i < sizeof(width)/sizeof(float*); i++) {
      if (width[i]) free(width[i]);
      }
  }
#endif
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
  if (wlen >= utfWlen)
  {
    utfWlen = wlen + 100;
	if (utfWbuf) free(utfWbuf);
    utfWbuf = (UniChar*)malloc((utfWlen)*sizeof(UniChar));
	wlen = fl_utf8toUtf16(txt, len, (unsigned short*)utfWbuf, utfWlen);
  }
  *new_len = wlen;
  return utfWbuf;
} // mac_Utf8_to_Utf16

Fl_Fontdesc* fl_fonts = built_in_table;

static Fl_Font_Descriptor* find(Fl_Font fnum, Fl_Fontsize size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_Font_Descriptor* f;
  for (f = s->first; f; f = f->next)
    if (f->size == size) return f;
  f = new Fl_Font_Descriptor(s->name, size);
  f->next = s->first;
  s->first = f;
  return f;
}

////////////////////////////////////////////////////////////////
// Public interface:

void Fl_Quartz_Graphics_Driver::font(Fl_Font fnum, Fl_Fontsize size) {
  if (fnum==-1) {
    Fl_Graphics_Driver::font(0, 0);
    return;
  }
  Fl_Graphics_Driver::font(fnum, size);
  this->font_descriptor( find(fnum, size) );
}

int Fl_Quartz_Graphics_Driver::height() {
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  Fl_Font_Descriptor *fl_fontsize = font_descriptor();
  return fl_fontsize->ascent + fl_fontsize->descent;
}

int Fl_Quartz_Graphics_Driver::descent() {
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  Fl_Font_Descriptor *fl_fontsize = font_descriptor();
  return fl_fontsize->descent+1;
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
// returns width of a pair of UniChar's in the surrogate range
static CGFloat surrogate_width(const UniChar *txt, Fl_Font_Descriptor *fl_fontsize)
{
  CTFontRef font2 = fl_fontsize->fontref;
  bool must_release = false;
  CGGlyph glyphs[2];
  bool b = CTFontGetGlyphsForCharacters(font2, txt, glyphs, 2);
  CGSize a;
  if(!b) { // the current font doesn't contain this char
    CFStringRef str = CFStringCreateWithCharactersNoCopy(NULL, txt, 2, kCFAllocatorNull);
    // find a font that contains it
    font2 = CTFontCreateForString(font2, str, CFRangeMake(0,2));
    must_release = true;
    CFRelease(str);
    b = CTFontGetGlyphsForCharacters(font2, txt, glyphs, 2);
  }
  if (b) CTFontGetAdvancesForGlyphs(font2, kCTFontHorizontalOrientation, glyphs, &a, 1);
  else a.width = fl_fontsize->q_width;
  if(must_release) CFRelease(font2);
  return a.width;
}
#endif

static double fl_mac_width(const UniChar* txt, int n, Fl_Font_Descriptor *fl_fontsize) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if (fl_mac_os_version >= 100500) {
  double retval = 0;
  UniChar uni;
  int i;
  for (i = 0; i < n; i++) { // loop over txt
    uni = txt[i];
    if (uni >= 0xD800 && uni <= 0xDBFF) { // handles the surrogate range
      retval += surrogate_width(&txt[i], fl_fontsize);
      i++; // because a pair of UniChar's represent a single character
      continue;
    }
    const int block = 0x10000 / (sizeof(fl_fontsize->width)/sizeof(float*)); // block size
    // r: index of the character block containing uni
    unsigned int r = uni >> 7; // change 7 if sizeof(width) is changed
    if (!fl_fontsize->width[r]) { // this character block has not been hit yet
//fprintf(stderr,"r=%d size=%d name=%s\n",r,fl_fontsize->size, fl_fontsize->q_name);
      // allocate memory to hold width of each character in the block
      fl_fontsize->width[r] = (float*) malloc(sizeof(float) * block);
      UniChar ii = r * block;
      CGSize advance_size;
      CGGlyph glyph;
      for (int j = 0; j < block; j++) { // loop over the block
	CTFontRef font2 = fl_fontsize->fontref;
	bool must_release = false;
	// ii spans all characters of this block
	bool b = CTFontGetGlyphsForCharacters(font2, &ii, &glyph, 1);
	if (!b) { // the current font doesn't contain this char
	  CFStringRef str = CFStringCreateWithCharactersNoCopy(NULL, &ii, 1, kCFAllocatorNull);
	  // find a font that contains it
	  font2 = CTFontCreateForString(font2, str, CFRangeMake(0,1));
	  must_release = true;
	  CFRelease(str);
	  b = CTFontGetGlyphsForCharacters(font2, &ii, &glyph, 1);
	  }
	if (b) CTFontGetAdvancesForGlyphs(font2, kCTFontHorizontalOrientation, &glyph, &advance_size, 1);
	else advance_size.width = 0.;
	// the width of one character of this block of characters
	fl_fontsize->width[r][j] = advance_size.width;
	if (must_release) CFRelease(font2);
	ii++;
      }
    }
    // sum the widths of all characters of txt
    retval += fl_fontsize->width[r][uni & (block-1)];
  }
  return retval;
} else {
#endif
#if ! __LP64__
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
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  }
#endif
  return 0;
}

double Fl_Quartz_Graphics_Driver::width(const char* txt, int n) {
  int wc_len = n;
  UniChar *uniStr = mac_Utf8_to_Utf16(txt, n, &wc_len);
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  return fl_mac_width(uniStr, wc_len, font_descriptor());
}

double Fl_Quartz_Graphics_Driver::width(unsigned int wc) {
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);

  UniChar utf16[3];
  int l = 1;
  if (wc <= 0xFFFF) {
    *utf16 = wc;
  }
  else {
//    char buf[4];
//    l = fl_utf8encode(wc, buf);
//    l = (int)fl_utf8toUtf16(buf, l, utf16, 3);
    l = (int)fl_ucs_to_Utf16(wc, utf16, 3);
  }
  return fl_mac_width(utf16, l, font_descriptor());
}

// text extent calculation
void Fl_Quartz_Graphics_Driver::text_extents(const char *str8, int n, int &dx, int &dy, int &w, int &h) {
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  Fl_Font_Descriptor *fl_fontsize = font_descriptor();
  UniChar *txt = mac_Utf8_to_Utf16(str8, n, &n);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if (fl_mac_os_version >= 100500) {
  CFStringRef str16 = CFStringCreateWithCharactersNoCopy(NULL, txt, n,  kCFAllocatorNull);
  CFDictionarySetValue (attributes, kCTFontAttributeName, fl_fontsize->fontref);
  CFAttributedStringRef mastr = CFAttributedStringCreate(kCFAllocatorDefault, str16, attributes);
  CFRelease(str16);
  CTLineRef ctline = CTLineCreateWithAttributedString(mastr);
  CFRelease(mastr);
  CGContextSetTextPosition(fl_gc, 0, 0);
  CGContextSetShouldAntialias(fl_gc, true);
  CGRect rect = CTLineGetImageBounds(ctline, fl_gc);
  CGContextSetShouldAntialias(fl_gc, false);
  CFRelease(ctline);
  dx = floor(rect.origin.x + 0.5);
  dy = floor(- rect.origin.y - rect.size.height + 0.5);
  w = rect.size.width + 0.5;
  h = rect.size.height + 0.5;
  }
else {
#endif
#if ! __LP64__
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
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  }
#endif
  return;
} // fl_text_extents

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
static CGColorRef flcolortocgcolor(Fl_Color i)
{
  uchar r, g, b;
  Fl::get_color(i, r, g, b);
  CGFloat components[4] = {r/255.0f, g/255.0f, b/255.0f, 1.};
  static CGColorSpaceRef cspace = NULL;
  if (cspace == NULL) {
    cspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    }
  return CGColorCreate(cspace, components);
}
#endif

static void fl_mac_draw(const char *str, int n, float x, float y, Fl_Graphics_Driver *driver) {
  // convert to UTF-16 first
  UniChar *uniStr = mac_Utf8_to_Utf16(str, n, &n);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  if (fl_mac_os_version >= 100500) {
    CFStringRef str16 = CFStringCreateWithCharactersNoCopy(NULL, uniStr, n,  kCFAllocatorNull);
    if (str16 == NULL) return; // shd not happen
    CGColorRef color = flcolortocgcolor(driver->color());
    CFDictionarySetValue (attributes, kCTFontAttributeName, driver->font_descriptor()->fontref);
    CFDictionarySetValue (attributes, kCTForegroundColorAttributeName, color);
    CFAttributedStringRef mastr = CFAttributedStringCreate(kCFAllocatorDefault, str16, attributes);
    CFRelease(str16);
    CFRelease(color);
    CTLineRef ctline = CTLineCreateWithAttributedString(mastr);
    CFRelease(mastr);
    CGContextSetTextMatrix(fl_gc, font_mx);
    CGContextSetTextPosition(fl_gc, x, y);
    CGContextSetShouldAntialias(fl_gc, true);
    CTLineDraw(ctline, fl_gc);
    CGContextSetShouldAntialias(fl_gc, false);
    CFRelease(ctline);
  } else {
#endif
#if ! __LP64__
  OSStatus err;
  // now collect our ATSU resources
  ATSUTextLayout layout = driver->font_descriptor()->layout;

  ByteCount iSize = sizeof(CGContextRef);
  ATSUAttributeTag iTag = kATSUCGContextTag;
  ATSUAttributeValuePtr iValuePtr=&fl_gc;
  ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  CGContextSetShouldAntialias(fl_gc, true);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x), FloatToFixed(y));
  CGContextSetShouldAntialias(fl_gc, false);
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  }
#endif
}

void Fl_Quartz_Graphics_Driver::draw(const char *str, int n, float x, float y) {
  // avoid a crash if no font has been selected by user yet !
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  fl_mac_draw(str, n, x, y, this);
}

void Fl_Quartz_Graphics_Driver::draw(const char* str, int n, int x, int y) {
  // avoid a crash if no font has been selected by user yet !
  if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
  fl_mac_draw(str, n, (float)x-0.0f, (float)y+0.5f, this);
}

void Fl_Quartz_Graphics_Driver::draw(int angle, const char *str, int n, int x, int y) {
  CGContextSaveGState(fl_gc);
  CGContextTranslateCTM(fl_gc, x, y);
  CGContextRotateCTM(fl_gc, - angle*(M_PI/180) );
  draw(str, n, 0, 0);
  CGContextRestoreGState(fl_gc);
}

void Fl_Quartz_Graphics_Driver::rtl_draw(const char* c, int n, int x, int y) {
  int dx, dy, w, h;
  text_extents(c, n, dx, dy, w, h);
  draw(c, n, x - w - dx, y);
}

//
// End of "$Id: fl_font_mac.cxx 8597 2011-04-17 13:18:55Z ianmacarthur $".
//
