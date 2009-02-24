//
// "$Id: fl_set_fonts_xft.cxx 5505 2006-10-03 02:35:12Z mike $"
//
// More font utilities for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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

#include <X11/Xft/Xft.h>

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
    int type;
    switch (p[0]) {
    case 'B': type = FL_BOLD; break;
    case 'I': type = FL_ITALIC; break;
    case 'P': type = FL_BOLD | FL_ITALIC; break;
    default:  type = 0; break;
    }
  
  // NOTE: This can cause duplications in fonts that already have Bold or Italic in 
  // their "name". Maybe we need to find a cleverer way?
    strlcpy(f->fontname, p+1, ENDOFBUFFER);
    if (type & FL_BOLD) strlcat(f->fontname, " bold", ENDOFBUFFER);
    if (type & FL_ITALIC) strlcat(f->fontname, " italic", ENDOFBUFFER);
    f->fontname[ENDOFBUFFER] = (char)type;
  }
  if (ap) *ap = f->fontname[ENDOFBUFFER];
  return f->fontname;
}

///////////////////////////////////////////////////////////
#define LOCAL_RAW_NAME_MAX 256

extern "C" {
// sort returned fontconfig font names
static int name_sort(const void *aa, const void *bb) {
  // What should we do here? Just do a string compare for now...
  // NOTE: This yeilds some oddities - in particular a Blah Bold font will be 
  // listed before Blah...
  // Also - the fontconfig listing returns some faces that are effectively duplicates
  // as far as fltk is concerned, e.g. where there are ko or ja variants that we 
  // can't distinguish (since we are not yet fully UTF-*) - should we strip them here?
  return strcasecmp(*(char**)aa, *(char**)bb);
} // end of name_sort
} // end of extern C section


// Read the "pretty" name we have derived from fontconfig then convert
// it into the format fltk uses internally for Xft names...
// This is just a mess - I should have tokenised the strings and gone from there,
// but I really thought this would be easier!
static void make_raw_name(char *raw, char *pretty)
{
  // Input name will be "Some Name:style = Bold Italic" or whatever
  // The plan is this:
  // - the first char in the "raw" name becomes either I, B, P or " " for
  //   italic, bold, bold italic or normal - this seems to be the fltk way...

  char *style = strchr(pretty, ':');
  char *last = style + strlen(style) - 2;

  if (style)
  {
    *style = 0; // Terminate "name" string
    style ++;   // point to start of style section
  }
  raw[0] = ' '; raw[1] = 0; // Default start of "raw name" text
  strncat(raw, pretty, LOCAL_RAW_NAME_MAX);
  // At this point, the name is "marked" as regular...
  if (style)
  {
#define PLAIN   0
#define BOLD    1
#define ITALIC  2
#define BITALIC (BOLD | ITALIC)
    int mods = PLAIN;
    // Now try and parse the style string - look for the "=" sign
    style = strchr(style, '=');
    while ((style) && (style < last))
    {
      int type;
      while ((*style == '=') || (*style == ' ') || (*style == '\t'))
      {
        style++; // Start of Style string
        if ((style >= last) || (*style == 0)) continue;
      }
      type = toupper(style[0]);
      switch (type)
      {
      // Things we might see: Regular Normal Bold Italic Oblique (??what??) Medium
      // Roman Light Demi Sans SemiCondensed SuperBold Book... etc...
      // Things we actually care about: Bold Italic Oblique SuperBold - Others???
      case 'I':
        if (strncasecmp(style, "Italic", 6) == 0)
        {
          mods |= ITALIC;
        }
        goto NEXT_STYLE;
        
      case 'B':
        if (strncasecmp(style, "Bold", 4) == 0)
        {
          mods |= BOLD;
        }
        goto NEXT_STYLE;
        
      case 'O':
        if (strncasecmp(style, "Oblique", 7) == 0)
        {
          mods |= ITALIC;
        }
        goto NEXT_STYLE;
          
      case 's':
        if (strncasecmp(style, "SuperBold", 9) == 0)
        {
          mods |= BOLD;
        }
        goto NEXT_STYLE;
          
      default: // find the next gap
        goto NEXT_STYLE;
      } // switch end
NEXT_STYLE:
      while ((*style != ' ') && (*style != '\t'))
      {
        style++;
        if ((style >= last) || (*style == 0)) goto STYLE_DONE;
      }
    }
STYLE_DONE:
    // Set the "modifier" character in the raw string
    switch(mods)
    {
    case BOLD: raw[0] = 'B';
      break;
    case ITALIC: raw[0] = 'I';
      break;
    case BITALIC: raw[0] = 'P';
      break;
    default: raw[0] = ' ';
      break;
    }
  }
} // make_raw_name

///////////////////////////////////////////////////////////

static int fl_free_font = FL_FREE_FONT;

// Uses the fontconfig lib to construct a list of all installed fonts.
// I tried using XftListFonts for this, but the API is tricky - and when
// I looked at the XftList* code, it calls the Fc* functions anyway, so...
//
// Also, for now I'm ignoring the "pattern_name" and just getting everything...
// AND I don't try and skip the fonts we've already loaded in the defaults.
// Blimey! What a hack!
Fl_Font Fl::set_fonts(const char* pattern_name)
{
  FcFontSet  *fnt_set;     // Will hold the list of fonts we find
  FcPattern   *fnt_pattern; // Holds the generic "match all names" pattern
  FcObjectSet *fnt_obj_set = 0; // Holds the generic "match all objects" 
  
  int j; // loop iterator variable
  int font_count; // Total number of fonts found to process
  char **full_list; // The list of font names we build

  if (fl_free_font > FL_FREE_FONT) // already been here
    return (Fl_Font)fl_free_font;
  
  fl_open_display(); // Just in case...
    
  // Make sure fontconfig is ready... is this necessary? The docs say it is
  // safe to call it multiple times, so just go for it anyway!
  if (!FcInit()) 
  {
    // What to do? Just return defaults...
    return FL_FREE_FONT;
  }

  // Create a search pattern that will match every font name - I think this
  // does the Right Thing, but am not certain...
  //
  // This could possibly be "enhanced" to pay attention to the requested
  // "pattern_name"?
  fnt_pattern = FcPatternCreate();
  fnt_obj_set = FcObjectSetBuild(FC_FAMILY, FC_STYLE, (void *)0);
    
  // Hopefully, this is a set of all the fonts...
  fnt_set = FcFontList(0, fnt_pattern, fnt_obj_set);
  
  // We don't need the fnt_pattern any more, release it
  FcPatternDestroy(fnt_pattern);

  // Now, if we got any fonts, iterate through them...
  if (fnt_set)
  {
    char *stop;
    char *start;
    char *first;
    
    font_count = fnt_set->nfont; // How many fonts?
    
    // Allocate array of char*'s to hold the name strings
    full_list = (char **)malloc(sizeof(char *) * font_count);
    
    // iterate through all the font patterns and get the names out...
      for (j = 0; j < font_count; j++)
      {
      // NOTE: FcChar8 is a typedef of "unsigned char"...
      FcChar8 *font; // String to hold the font's name
            
      // Convert from fontconfig internal pattern to human readable name
      // NOTE: This WILL malloc storage, so we need to free it later...
      font = FcNameUnparse(fnt_set->fonts[j]);
            
      // The returned strings look like this...
      // Century Schoolbook:style=Bold Italic,fed kursiv,Fett Kursiv,...
      // So the bit we want is up to the first comma - BUT some strings have
      // more than one name, separated by, guess what?, a comma...
      stop = start = first = 0;
      stop = strchr((const char *)font, ',');
      start = strchr((const char *)font, ':');
      if ((stop) && (start) && (stop < start))
      {
        first = stop + 1; // discard first version of name
        // find first comma *after* the end of the name
        stop = strchr((const char *)start, ',');
      }
      else
      {
        first = (char *)font; // name is just what was returned
      }
      // Truncate the name after the (english) modifiers description
      if (stop)
      {
        *stop = 0; // Terminate the string at the first comma, if there is one
      }

      // Copy the font description into our list
      if (first == (char *)font)
      { // The listed name is still OK
        full_list[j] = (char *)font;
      }
      else
      { // The listed name has been modified
        full_list[j] = strdup(first);
        // Free the font name storage
        free (font);
      }
      // replace "style=Regular" so strcmp sorts it first
      if (start) {
        char *reg = strstr(full_list[j], "=Regular");
        if (reg) reg[1]='.';
      }
    }
        
    // Release the fnt_set - we don't need it any more
    FcFontSetDestroy(fnt_set);
        
    // Sort the list into alphabetic order
    qsort(full_list, font_count, sizeof(*full_list), name_sort);
    
    // Now let us add the names we got to fltk's font list...
    for (j = 0; j < font_count; j++)
    {
      if (full_list[j])
      {
        char xft_name[LOCAL_RAW_NAME_MAX];
        char *stored_name;
        // Parse the strings into FLTK-XFT style..
        make_raw_name(xft_name, full_list[j]);
        // NOTE: This just adds on AFTER the default fonts - no attempt is made
        // to identify already loaded fonts. Is this bad?
        stored_name = strdup(xft_name);
        Fl::set_font((Fl_Font)(j + FL_FREE_FONT), stored_name);
        fl_free_font ++;
        
        free(full_list[j]); // release that name from our internal array
      }
    }
    // Now we are done with the list, release it fully
    free(full_list);
  }
  return (Fl_Font)fl_free_font;
} // ::set_fonts
////////////////////////////////////////////////////////////////


extern "C" {
static int int_sort(const void *aa, const void *bb) {
  return (*(int*)aa)-(*(int*)bb);
}
}

////////////////////////////////////////////////////////////////

// Return all the point sizes supported by this font:
// Suprisingly enough Xft works exactly like fltk does and returns
// the same list. Except there is no way to tell if the font is scalable.
int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0

  fl_open_display();
  XftFontSet* fs = XftListFonts(fl_display, fl_screen,
                                XFT_FAMILY, XftTypeString, s->name+1,
				(void *)0,
                                XFT_PIXEL_SIZE,
				(void *)0);
  static int* array = 0;
  static int array_size = 0;
  if (fs->nfont >= array_size) {
    delete[] array;
    array = new int[array_size = fs->nfont+1];
  }
  array[0] = 0; int j = 1; // claim all fonts are scalable
  for (int i = 0; i < fs->nfont; i++) {
    double v;
    if (XftPatternGetDouble(fs->fonts[i], XFT_PIXEL_SIZE, 0, &v) == XftResultMatch) {
      array[j++] = int(v);
    }
  }
  qsort(array+1, j-1, sizeof(int), int_sort);
  XftFontSetDestroy(fs);
  sizep = array;
  return j;
}

//
// End of "$Id: fl_set_fonts_xft.cxx 5505 2006-10-03 02:35:12Z mike $".
//
