//
// "$Id: fl_set_fonts_mac.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// MacOS font utilities for the Fast Light Tool Kit (FLTK).
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

// This function fills in the fltk font table with all the fonts that
// are found on the X server.  It tries to place the fonts into families
// and to sort them so the first 4 in a family are normal, bold, italic,
// and bold italic.

// Bug: older versions calculated the value for *ap as a side effect of
// making the name, and then forgot about it. To avoid having to change
// the header files I decided to store this value in the last character
// of the font name array.
#define ENDOFBUFFER 127 // sizeof(Fl_Font.fontname)-1

// turn a stored font name into a pretty name:
const char* Fl::get_font_name(Fl_Font fnum, int* ap) {
#ifdef __APPLE_QD__
  Fl_Fontdesc *f = fl_fonts + fnum;
  if (!f->fontname[0]) {
    const char* p = f->name;
    if (!p || !*p) {if (ap) *ap = 0; return "";}
    int type;
    switch (*p) {
    case 'B': type = FL_BOLD; break;
    case 'I': type = FL_ITALIC; break;
    case 'P': type = FL_BOLD | FL_ITALIC; break;
    default:  type = 0; break;
    }
    strlcpy(f->fontname, p+1, ENDOFBUFFER);
    if (type & FL_BOLD) strlcat(f->fontname, " bold", ENDOFBUFFER);
    if (type & FL_ITALIC) strlcat(f->fontname, " italic", ENDOFBUFFER);
    f->fontname[ENDOFBUFFER] = (char)type;
  }
  if (ap) *ap = f->fontname[ENDOFBUFFER];
  return f->fontname;
#elif defined(__APPLE_QUARTZ__)
  Fl_Fontdesc *f = fl_fonts + fnum;
  if (!f->fontname[0]) {
    const char* p = f->name;
    if (!p || !*p) {if (ap) *ap = 0; return "";}
    strlcpy(f->fontname, p, ENDOFBUFFER);
    int type = 0;
    if (strstr(f->name, "Bold")) type |= FL_BOLD;
    if (strstr(f->name, "Italic")) type |= FL_ITALIC;
    f->fontname[ENDOFBUFFER] = (char)type;
  }
  if (ap) *ap = f->fontname[ENDOFBUFFER];
  return f->fontname;
#endif
}

static int fl_free_font = FL_FREE_FONT;

Fl_Font Fl::set_fonts(const char* xstarname) {
#pragma unused ( xstarname )
#ifdef __APPLE_QD__
  if (fl_free_font != FL_FREE_FONT) 
    return (Fl_Font)fl_free_font;
  static char styleLU[] = " BIP";
  FMFontFamilyInstanceIterator ffiIterator;
  FMFontFamilyIterator ffIterator;
  FMFontFamily family;
  FMFont font;
  FMFontStyle style; // bits 0..6: bold, italic underline, outline, shadow, condens, extended (FLTK supports 0 and 1 )
  FMFontSize size;
  //FMFilter filter; // do we need to set a specific (or multiple) filter(s) to get ALL fonts?
  
  Str255 buf;
  //filter.format = kFMCurrentFilterFormat;
  //filter.selector = kFMGenerationFilterSelector;
  //filter.filter.generationFilter = 
  FMCreateFontFamilyIterator( NULL, NULL, kFMUseGlobalScopeOption, &ffIterator );
  OSStatus listFamilies, listInstances;
  for (;;)
  {
    listFamilies = FMGetNextFontFamily( &ffIterator, &family );
    if ( listFamilies != 0 ) break;
    FMGetFontFamilyName( family, buf );
    buf[ buf[0]+1 ] = 0;
    //printf( "Font Family: %s\n", buf+1 );
    int i;
    for (i=0; i<FL_FREE_FONT; i++) // skip if one of our built-in fonts
      if (!strcmp(Fl::get_font_name((Fl_Font)i),(char*)buf+1)) break;
    if ( i < FL_FREE_FONT ) continue;
    FMCreateFontFamilyInstanceIterator( family, &ffiIterator );
    char pStyle = 0, nStyle;
    for (;;)
    {
      listInstances = FMGetNextFontFamilyInstance( &ffiIterator, &font, &style, &size );
      if ( listInstances != 0 ) break;
      // printf(" %d %d %d\n", font, style, size );
      nStyle = styleLU[style&0x03];
      if ( ( pStyle & ( 1<<(style&0x03) ) ) == 0 )
      {
        buf[0] = nStyle;
        Fl::set_font((Fl_Font)(fl_free_font++), strdup((char*)buf));
        pStyle |= ( 1<<(style&0x03) );
      }
    }
    FMDisposeFontFamilyInstanceIterator( &ffiIterator );
  }
  FMDisposeFontFamilyIterator( &ffIterator );
  return (Fl_Font)fl_free_font;
#elif defined(__APPLE_QUARTZ__)
  ATSFontIterator it;
  ATSFontIteratorCreate(kATSFontContextGlobal, 0L, 0L, kATSOptionFlagsRestrictedScope, &it);  
  for (;;) {
    ATSFontRef font;
    CFStringRef fname = 0;
    OSStatus err = ATSFontIteratorNext(it, &font);
    if (err!=noErr) break;
    ATSFontGetName(font, kATSOptionFlagsDefault, &fname);
    char buf[1024];
    CFStringGetCString(fname, buf, 1024, kCFStringEncodingASCII);
    int i;
    for (i=0; i<FL_FREE_FONT; i++) // skip if one of our built-in fonts
      if (!strcmp(Fl::get_font_name((Fl_Font)i),buf)) break;
    if ( i < FL_FREE_FONT ) continue;
    Fl::set_font((Fl_Font)(fl_free_font++), strdup((char*)buf));
  }
  ATSFontIteratorRelease(&it);
  return (Fl_Font)fl_free_font;
#endif
}

static int array[128];
int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0
  int cnt = 0;

#ifdef __APPLE_QD__
  Str255 name;
  int len = strlen( s->name );
  memcpy(((char*)name)+1, s->name+1, len );
  name[0] = len-1;
  FMFontFamily family = FMGetFontFamilyFromName( name );
  if ( family == kInvalidFontFamily ) return 0;

  sizep = array;
  FMFont font;
  FMFontStyle style, fStyle;
  switch ( s->name[0] ) {
    default :
      fStyle=0;
      break;
    case 'B' : 
      fStyle=1;
      break;
    case 'I' : 
      fStyle=2;
      break;
    case 'P' :
      fStyle=3;
      break;
  }
  FMFontSize size, pSize = -1;
  FMFontFamilyInstanceIterator ffiIterator;
  FMCreateFontFamilyInstanceIterator( family, &ffiIterator );
  OSStatus listInstances;
  for (;;)
  {
    listInstances = FMGetNextFontFamilyInstance( &ffiIterator, &font, &style, &size );
    if ( listInstances != 0 ) break;
    if ( style==fStyle )
    {
      if ( size>pSize ) 
      {
        array[ cnt++ ] = size;
        pSize = size;
      }
    }
  }
  FMDisposeFontFamilyInstanceIterator( &ffiIterator );
#elif defined(__APPLE_QUARTZ__)
  // ATS supports all font size 
  array[0] = 0;
  sizep = array;
  cnt = 1;
#endif

  return cnt;
}

//
// End of "$Id: fl_set_fonts_mac.cxx 5190 2006-06-09 16:16:34Z mike $".
//
