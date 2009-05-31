//
// "$Id: fl_set_fonts_mac.cxx 6616 2009-01-01 21:28:26Z matt $"
//
// MacOS font utilities for the Fast Light Tool Kit (FLTK).
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

// #inclde <SFNTTypes.h>

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
}

static int fl_free_font = FL_FREE_FONT;

Fl_Font Fl::set_fonts(const char* xstarname) {
#pragma unused ( xstarname )
#if defined(OLD__APPLE_QUARTZ__)
  ATSFontIterator it;
  ATSFontIteratorCreate(kATSFontContextGlobal, 0L, 0L, kATSOptionFlagsUnRestrictedScope, &it);  
  for (;;) {
    ATSFontRef font;
    CFStringRef fname = 0;
    OSStatus err = ATSFontIteratorNext(it, &font);
    if (err!=noErr) break;
    ATSFontGetName(font, kATSOptionFlagsDefault, &fname);
    char buf[1024];
    CFStringGetCString(fname, buf, 1024, kCFStringEncodingUTF8);
    int i;
    for (i=0; i<FL_FREE_FONT; i++) // skip if one of our built-in fonts
      if (!strcmp(Fl::get_font_name((Fl_Font)i),buf)) break;
    if ( i < FL_FREE_FONT ) continue;
    Fl::set_font((Fl_Font)(fl_free_font++), strdup((char*)buf));
  }
  ATSFontIteratorRelease(&it);
  return (Fl_Font)fl_free_font;
#else
  ItemCount oFontCount, oCountAgain;
  ATSUFontID *oFontIDs;
  // How many fonts?
  ATSUFontCount (&oFontCount);
  // now allocate space for them...
  oFontIDs = (ATSUFontID *)malloc((oFontCount+1) * sizeof(ATSUFontID));
  ATSUGetFontIDs (oFontIDs, oFontCount, &oCountAgain);
  // Now oFontIDs should contain a list of all the available Unicode fonts
  // Iterate through the list to get each font name
  for (ItemCount idx = 0; idx < oFontCount; idx++)
  {
//  ByteCount actualLength = 0;
//	Ptr oName;
    // How to get the name - Apples docs say call this twice, once to get the length, then again 
	// to get the actual name...
//    ATSUFindFontName (oFontIDs[idx], kFontFullName, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage,
//                      0, NULL, &actualLength, NULL);
    // Now actualLength tells us the length of buffer we need
//	oName = (Ptr)malloc(actualLength + 8);
// But who's got time for that nonsense? Let's just hard code a fixed buffer (urgh!)
    ByteCount actualLength = 511;
	char oName[512];
    ATSUFindFontName (oFontIDs[idx], kFontFullName, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage,
                      actualLength, oName, &actualLength, &oCountAgain);
    // bounds check and terminate the returned name
    if(actualLength > 511)
      oName[511] = 0;
    else
      oName[actualLength] = 0;
	Fl::set_font((Fl_Font)(fl_free_font++), strdup(oName));
//	free(oName);
  }
  free(oFontIDs);
  return (Fl_Font)fl_free_font;
#endif
}

static int array[128];
int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0
  int cnt = 0;

  // ATS supports all font size 
  array[0] = 0;
  sizep = array;
  cnt = 1;

  return cnt;
}

//
// End of "$Id: fl_set_fonts_mac.cxx 6616 2009-01-01 21:28:26Z matt $".
//
