//
// "$Id: fl_font_mac.cxx 7659 2010-07-01 13:21:32Z manolo $"
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

static const CGAffineTransform font_mx = { 1, 0, 0, -1, 0, 0 };
static SInt32 MACsystemVersion = 0;

Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize Size) {
  next = 0;
#  if HAVE_GL
  listbase = 0;
#  endif

//  knowWidths = 0;
    // OpenGL needs those for its font handling
  q_name = strdup(name);
  size = Size;
  minsize = maxsize = Size;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  if(MACsystemVersion == 0) Gestalt(gestaltSystemVersion, &MACsystemVersion);

if(MACsystemVersion >= 0x1050) {//unfortunately, CTFontCreateWithName != NULL on 10.4 also!
  CFStringRef str = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
  fontref = CTFontCreateWithName(str, size, NULL);
  CGGlyph glyph[2];
  const UniChar A[2]={'W','.'};
  CTFontGetGlyphsForCharacters(fontref, A, glyph, 2);
  CGSize advances[2];
  double w;
  CTFontGetAdvancesForGlyphs(fontref, kCTFontHorizontalOrientation, glyph, advances, 2);
  w = advances[0].width;
  if( abs(advances[0].width - advances[1].width) < 1E-2 ) {//this is a fixed-width font
    //slightly rescale fixed-width fonts so the character width has an integral value
    CFRelease(fontref);
    CGFloat fsize = size / ( w/floor(w + 0.5) );
    fontref = CTFontCreateWithName(str, fsize, NULL);
    w = CTFontGetAdvancesForGlyphs(fontref, kCTFontHorizontalOrientation, glyph, NULL, 1);
  }
  CFRelease(str);
  ascent = (short)(CTFontGetAscent(fontref) + 0.5);
  descent = (short)(CTFontGetDescent(fontref) + 0.5);
  q_width = w + 0.5;
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
#endif//__LP64__
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  }
#endif
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
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  if(MACsystemVersion >= 0x1050)  CFRelease(fontref);
#else
	/*  ATSUDisposeTextLayout(layout);
  ATSUDisposeStyle(style); */
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
  // we will use fl_fontsize later to access the required style and layout
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


void Fl_Graphics_Driver::font(Fl_Font fnum, Fl_Fontsize size) {
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
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if(MACsystemVersion >= 0x1050) {
  CTFontRef fontref = fl_fontsize->fontref;
  CFStringRef str = CFStringCreateWithBytes(NULL, (const UInt8*)txt, n * sizeof(UniChar), kCFStringEncodingUTF16, false);
  CFAttributedStringRef astr = CFAttributedStringCreate(NULL, str, NULL);
  CFMutableAttributedStringRef mastr = CFAttributedStringCreateMutableCopy(NULL, 0, astr);
  CFRelease(astr);
  CFAttributedStringSetAttribute(mastr, CFRangeMake(0, CFStringGetLength(str)), kCTFontAttributeName, fontref);
  CFRelease(str);
  CTLineRef ctline = CTLineCreateWithAttributedString(mastr);
  CFRelease(mastr);
  double retval = CTLineGetTypographicBounds(ctline, NULL, NULL, NULL);
  CFRelease(ctline);
  return retval;
  }
else {
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
  return 0; // FIXME: I do not understand the shuffeling of the above ifdef's and why they are here!
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
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if(MACsystemVersion >= 0x1050) {
  CTFontRef fontref = fl_fontsize->fontref;
  CFStringRef str16 = CFStringCreateWithBytes(NULL, (const UInt8*)txt, n *sizeof(UniChar), kCFStringEncodingUTF16, false);
  CFAttributedStringRef astr = CFAttributedStringCreate(NULL, str16, NULL);
  CFMutableAttributedStringRef mastr = CFAttributedStringCreateMutableCopy(NULL, 0, astr);
  CFRelease(astr);
  CFAttributedStringSetAttribute(mastr, CFRangeMake(0, CFStringGetLength(str16)), kCTFontAttributeName, fontref);
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

void fl_text_extents(const char *c, int n, int &dx, int &dy, int &w, int &h) {
  int wc_len = n;
  UniChar *uniStr = mac_Utf8_to_Utf16(c, n, &wc_len);
  fl_text_extents(uniStr, wc_len, dx, dy, w, h);
} // fl_text_extents


void fl_draw(const char *str, int n, float x, float y);

void Fl_Graphics_Driver::draw(const char* str, int n, int x, int y) {
  fl_draw(str, n, (float)x-0.0f, (float)y+0.5f);
}


static CGColorRef flcolortocgcolor(Fl_Color i)
{
  uchar r, g, b;
  Fl::get_color(i, r, g, b);
  CGFloat components[4] = {r/255.0f, g/255.0f, b/255.0f, 1.};
  static CGColorSpaceRef cspace = NULL;
  if(cspace == NULL) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_4
    cspace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
#else
    cspace = CGColorSpaceCreateWithName(kCGColorSpaceUserRGB);
#endif
    }
  return CGColorCreate(cspace, components);
}

void fl_draw(const char *str, int n, float x, float y) {
  
  if(fl_graphics_driver->type() != Fl_Quartz_Graphics_Driver::device_type) {
    fl_graphics_driver->draw(str, n, (int)x, (int)y );
    return;
    }
  // avoid a crash if no font has been selected by user yet !
  check_default_font();
  // convert to UTF-16 first
  UniChar *uniStr = mac_Utf8_to_Utf16(str, n, &n);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  if(MACsystemVersion >= 0x1050) {
    CFStringRef keys[2];
    CFTypeRef values[2];  
    CFStringRef str16 = CFStringCreateWithBytes(NULL, (const UInt8*)uniStr, n * sizeof(UniChar), kCFStringEncodingUTF16, false);
    CGColorRef color = flcolortocgcolor(fl_color());
    keys[0] = kCTFontAttributeName;
    keys[1] = kCTForegroundColorAttributeName;
    values[0] = fl_fontsize->fontref;
    values[1] = color;
    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault,
						    (const void**)&keys,
						    (const void**)&values,
						    2,
						    &kCFTypeDictionaryKeyCallBacks,
						    &kCFTypeDictionaryValueCallBacks);
    CFAttributedStringRef mastr = CFAttributedStringCreate(kCFAllocatorDefault, str16, attributes);
    CFRelease(str16);
    CFRelease(attributes);
    CFRelease(color);
    CTLineRef ctline = CTLineCreateWithAttributedString(mastr);
    CFRelease(mastr);
    CGContextSetTextMatrix(fl_gc, font_mx);
    CGContextSetTextPosition(fl_gc, x, y);
    CGContextSetShouldAntialias(fl_gc, true);
    CTLineDraw(ctline, fl_gc);
    CGContextSetShouldAntialias(fl_gc, false);
    CFRelease(ctline);
  }
  else {
#endif
#if ! __LP64__
  OSStatus err;
  // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;

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

void Fl_Graphics_Driver::draw(int angle, const char *str, int n, int x, int y) {
  CGContextSaveGState(fl_gc);
  CGContextTranslateCTM(fl_gc, x, y);
  CGContextRotateCTM(fl_gc, - angle*(M_PI/180) );
  fl_draw(str, n, (float)0., (float)0.);
  CGContextRestoreGState(fl_gc);
}

void Fl_Graphics_Driver::rtl_draw(const char* c, int n, int x, int y) {
  draw(c, n, x - fl_width(c, n), y);
}

//
// End of "$Id: fl_font_mac.cxx 7659 2010-07-01 13:21:32Z manolo $".
//
