//
// "$Id: fl_font_xft.cxx 8442 2011-02-18 13:39:48Z manolo $"
//
// Xft font code for the Fast Light Tool Kit (FLTK).
//
// Copyright 2001-2011 Bill Spitzak and others.
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

//
// Draw fonts using Keith Packard's Xft library to provide anti-
// aliased text. Yow!
//
// Many thanks to Carl for making the original version of this.
//
// This font code only requires libXft to work.  Contrary to popular
// belief there is no need to have FreeType, or the Xrender extension
// available to use this code.  You will just get normal Xlib fonts
// (Xft calls them "core" fonts) The Xft algorithms for choosing
// these is about as good as the FLTK ones (I hope to fix it so it is
// exactly as good...), plus it can cache its results and share them
// between programs, so using this should be a win in all cases. Also
// it should be obvious by comparing this file and fl_font_x.cxx that
// it is a lot easier to program with Xft than with Xlib.
//
// Also, Xft supports UTF-8 text rendering directly, which will allow
// us to support UTF-8 on all platforms more easily.
//
// To actually get antialiasing you need the following:
//
//     1. You have XFree86 4
//     2. You have the XRender extension
//     3. Your X device driver supports the render extension
//     4. You have libXft
//     5. Your libXft has FreeType2 support compiled in
//     6. You have the FreeType2 library
//
// Distributions that have XFree86 4.0.3 or later should have all of this...
//
// Unlike some other Xft packages, I tried to keep this simple and not
// to work around the current problems in Xft by making the "patterns"
// complicated. I believe doing this defeats our ability to improve Xft
// itself. You should edit the ~/.xftconfig file to "fix" things, there
// are several web pages of information on how to do this.
//
#ifndef FL_DOXYGEN

#include <X11/Xft/Xft.h>

#include <math.h>

// The predefined fonts that FLTK has:
static Fl_Fontdesc built_in_table[] = {
#if 1
{" sans"},
{"Bsans"},
{"Isans"},
{"Psans"},
{" mono"},
{"Bmono"},
{"Imono"},
{"Pmono"},
{" serif"},
{"Bserif"},
{"Iserif"},
{"Pserif"},
{" symbol"},
{" screen"},
{"Bscreen"},
{" zapf dingbats"},
#else
{" helvetica"},
{"Bhelvetica"},
{"Ihelvetica"},
{"Phelvetica"},
{" courier"},
{"Bcourier"},
{"Icourier"},
{"Pcourier"},
{" times"},
{"Btimes"},
{"Itimes"},
{"Ptimes"},
{" symbol"},
{" lucidatypewriter"},
{"Blucidatypewriter"},
{" zapf dingbats"},
#endif
};

Fl_Fontdesc* fl_fonts = built_in_table;

Fl_XFont_On_Demand fl_xfont;
void *fl_xftfont = 0;
//const char* fl_encoding_ = "iso8859-1";
const char* fl_encoding_ = "iso10646-1";

static void fl_xft_font(Fl_Xlib_Graphics_Driver *driver, Fl_Font fnum, Fl_Fontsize size, int angle) {
  if (fnum==-1) { // special case to stop font caching
    driver->Fl_Graphics_Driver::font(0, 0);
    return;
  }
  Fl_Font_Descriptor* f = driver->font_descriptor();
  if (fnum == driver->Fl_Graphics_Driver::font() && size == driver->size() && f && f->angle == angle)
    return;
  driver->Fl_Graphics_Driver::font(fnum, size);
  Fl_Fontdesc *font = fl_fonts + fnum;
  // search the fontsizes we have generated already
  for (f = font->first; f; f = f->next) {
    if (f->size == size && f->angle == angle)// && !strcasecmp(f->encoding, fl_encoding_))
      break;
  }
  if (!f) {
    f = new Fl_Font_Descriptor(font->name, size, angle);
    f->next = font->first;
    font->first = f;
  }
  driver->font_descriptor(f);
#if XFT_MAJOR < 2
  fl_xfont    = f->font->u.core.font;
#else
  fl_xfont    = NULL; // invalidate
#endif // XFT_MAJOR < 2
  fl_xftfont = (void*)f->font;
}

void Fl_Xlib_Graphics_Driver::font(Fl_Font fnum, Fl_Fontsize size) {
  fl_xft_font(this,fnum,size,0);
}

static XftFont* fontopen(const char* name, Fl_Fontsize size, bool core, int angle) {
  // Check: does it look like we have been passed an old-school XLFD fontname?
  bool is_xlfd = false;
  int hyphen_count = 0;
  int comma_count = 0;
  unsigned len = strlen(name);
  if (len > 512) len = 512; // ensure we are not passed an unbounded font name
  for(unsigned idx = 0; idx < len; idx++) {
    if(name[idx] == '-') hyphen_count++; // check for XLFD hyphens
    if(name[idx] == ',') comma_count++;  // are there multiple names?
  }
  if(hyphen_count >= 14) is_xlfd = true; // Not a robust check, but good enough?

  fl_open_display();

  if(!is_xlfd) { // Not an XLFD - open as a XFT style name
    XftFont *the_font; // the font we will return;
    XftPattern *fnt_pat = XftPatternCreate(); // the pattern we will use for matching
    int slant = XFT_SLANT_ROMAN;
    int weight = XFT_WEIGHT_MEDIUM;

    /* This "converts" FLTK-style font names back into "regular" names, extracting
     * the BOLD and ITALIC codes as it does so - all FLTK font names are prefixed
     * by 'I' (italic) 'B' (bold) 'P' (bold italic) or ' ' (regular) modifiers.
     * This gives a fairly limited font selection ability, but is retained for
     * compatibility reasons. If you really need a more complex choice, you are best
     * calling Fl::set_fonts(*) then selecting the font by font-index rather than by
     * name anyway. Probably.
     * If you want to load a font who's name does actually begin with I, B or P, you
     * MUST use a leading space OR simply use lowercase for the name...
     */
    /* This may be efficient, but it is non-obvious. */
    switch (*name++) {
    case 'I': slant = XFT_SLANT_ITALIC; break; // italic
    case 'P': slant = XFT_SLANT_ITALIC;        // bold-italic (falls-through)
    case 'B': weight = XFT_WEIGHT_BOLD; break; // bold
    case ' ': break;                           // regular
    default: name--;                           // no prefix, restore name
    }

    if(comma_count) { // multiple comma-separated names were passed
      char *local_name = strdup(name); // duplicate the full name so we can edit the copy
      char *curr = local_name; // points to first name in string
      char *nxt; // next name in string
      do {
        nxt = strchr(curr, ','); // find comma separator
        if (nxt) {
          *nxt = 0; // terminate first name
          nxt++; // first char of next name
        }

	// Add the current name to the match pattern
	XftPatternAddString(fnt_pat, XFT_FAMILY, curr);

        if(nxt) curr = nxt; // move onto next name (if it exists)
	// Now do a cut-down version of the FLTK name conversion.
	// NOTE: we only use the slant and weight of the first name,
	// subsequent names we ignore this for... But we still need to do the check.
        switch (*curr++) {
        case 'I': break; // italic
        case 'P':        // bold-italic (falls-through)
        case 'B': break; // bold
        case ' ': break; // regular
        default: curr--; // no prefix, restore name
        }

        comma_count--; // decrement name sections count
      } while (comma_count >= 0);
      free(local_name); // release our local copy of font names
    }
    else { // single name was passed - add it directly
      XftPatternAddString(fnt_pat, XFT_FAMILY, name);
    }

    // Construct a match pattern for the font we want...
    XftPatternAddInteger(fnt_pat, XFT_WEIGHT, weight);
    XftPatternAddInteger(fnt_pat, XFT_SLANT, slant);
    XftPatternAddDouble (fnt_pat, XFT_PIXEL_SIZE, (double)size);
    XftPatternAddString (fnt_pat, XFT_ENCODING, fl_encoding_);

    // rotate font if angle!=0
    if (angle !=0) {
      XftMatrix m;
      XftMatrixInit(&m);
      XftMatrixRotate(&m,cos(M_PI*angle/180.),sin(M_PI*angle/180.));
      XftPatternAddMatrix (fnt_pat, XFT_MATRIX,&m);
    }

    if (core) {
      XftPatternAddBool(fnt_pat, XFT_CORE, FcTrue);
      XftPatternAddBool(fnt_pat, XFT_RENDER, FcFalse);
    }

    XftPattern *match_pat;  // the best available match on the system
    XftResult match_result; // the result of our matching attempt

    // query the system to find a match for this font
    match_pat = XftFontMatch(fl_display, fl_screen, fnt_pat, &match_result);

#if 0 // the XftResult never seems to get set to anything... abandon this code?
    switch(match_result) { // how good a match is this font for our request?
      case XftResultMatch:
	puts("Object exists with the specified ID");
	break;

      case XftResultTypeMismatch:
	puts("Object exists, but the type does not match");
	break;

      case XftResultNoId:
	puts("Object exists, but has fewer values than specified");
	break;

      case FcResultOutOfMemory:
	puts("FcResult: Malloc failed");
	break;

      case XftResultNoMatch:
	puts("Object does not exist at all");
	break;

      default:
	printf("Invalid XftResult status %d \n", match_result);
	break;
    }
#endif

#if 0 // diagnostic to print the "full name" of the font we matched. This works.
    FcChar8 *picked_name =  FcNameUnparse(match_pat);
    printf("Match: %s\n", picked_name);
    free(picked_name);
#endif

    if (!match_pat) {
      // last chance, just open any font in the right size
      the_font = XftFontOpen (fl_display, fl_screen,
                        XFT_FAMILY, XftTypeString, "sans",
                        XFT_SIZE, XftTypeDouble, (double)size,
                        NULL);
      XftPatternDestroy(fnt_pat);
      if (!the_font) {
        Fl::error("Unable to find fonts. Check your FontConfig configuration.\n");
        exit(1);
      }
      return the_font;
    }

    // open the matched font
    the_font = XftFontOpenPattern(fl_display, match_pat);

#if 0 // diagnostic to print the "full name" of the font we actually opened. This works.
    FcChar8 *picked_name2 =  FcNameUnparse(the_font->pattern);
    printf("Open : %s\n", picked_name2);
    free(picked_name2);
#endif

    XftPatternDestroy(fnt_pat);
//  XftPatternDestroy(match_pat); // FontConfig will destroy this resource for us. We must not!

    return the_font;
  }
  else { // We were passed a font name in XLFD format
    /* OksiD's X font code could handle being passed a comma separated list
     * of XLFD's. It then attempted to find which font was "best" from this list.
     * But XftFontOpenXlfd can not do this, so if a list is passed, we just
     * terminate it at the first comma.
     * A "better" solution might be to use XftXlfdParse() on each of the passed
     * XLFD's to construct a "super-pattern" that incorporates attributes from all
     * XLFD's and use that to perform a XftFontMatch(). Maybe...
     */
    char *local_name = strdup(name);
    if(comma_count) { // This means we were passed multiple XLFD's
      char *pc = strchr(local_name, ',');
      *pc = 0; // terminate the XLFD at the first comma
    }
    XftFont *the_font = XftFontOpenXlfd(fl_display, fl_screen, local_name);
    free(local_name);
#if 0 // diagnostic to print the "full name" of the font we actually opened. This works.
puts("Font Opened"); fflush(stdout);
    FcChar8 *picked_name2 =  FcNameUnparse(the_font->pattern);
    printf("Open : %s\n", picked_name2); fflush(stdout);
    free(picked_name2);
#endif
   return the_font;
  }
} // end of fontopen

Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize fsize, int fangle) {
//  encoding = fl_encoding_;
  size = fsize;
  angle = fangle;
#if HAVE_GL
  listbase = 0;
#endif // HAVE_GL
  font = fontopen(name, fsize, false, angle);
}

Fl_Font_Descriptor::~Fl_Font_Descriptor() {
  if (this == fl_graphics_driver->font_descriptor()) fl_graphics_driver->font_descriptor(NULL);
//  XftFontClose(fl_display, font);
}

/* decodes the input UTF-8 string into a series of wchar_t characters.
 n is set upon return to the number of characters.
 Don't deallocate the returned memory.
 */
static const wchar_t *utf8reformat(const char *str, int& n)
{
  static const wchar_t empty[] = {0};
  static wchar_t *buffer;
  static int lbuf = 0;
  int newn;
  if (n == 0) return empty;
  newn = fl_utf8towc(str, n, (wchar_t*)buffer, lbuf);
  if (newn >= lbuf) {
    lbuf = newn + 100;
    if (buffer) free(buffer);
    buffer = (wchar_t*)malloc(lbuf * sizeof(wchar_t));
    n = fl_utf8towc(str, n, (wchar_t*)buffer, lbuf);
  } else {
    n = newn;
  }
  return buffer;
}

static void utf8extents(Fl_Font_Descriptor *desc, const char *str, int n, XGlyphInfo *extents)
{
  memset(extents, 0, sizeof(XGlyphInfo));
  const wchar_t *buffer = utf8reformat(str, n);
#ifdef __CYGWIN__
    XftTextExtents16(fl_display, desc->font, (XftChar16 *)buffer, n, extents);
#else
    XftTextExtents32(fl_display, desc->font, (XftChar32 *)buffer, n, extents);
#endif
}

int Fl_Xlib_Graphics_Driver::height() {
  if (font_descriptor()) return font_descriptor()->font->ascent + font_descriptor()->font->descent;
  else return -1;
}

int Fl_Xlib_Graphics_Driver::descent() {
  if (font_descriptor()) return font_descriptor()->font->descent;
  else return -1;
}

double Fl_Xlib_Graphics_Driver::width(const char* str, int n) {
  if (!font_descriptor()) return -1.0;
  XGlyphInfo i;
  utf8extents(font_descriptor(), str, n, &i);
  return i.xOff;
}

/*double fl_width(uchar c) {
  return fl_graphics_driver->width((const char *)(&c), 1);
}*/

static double fl_xft_width(Fl_Font_Descriptor *desc, FcChar32 *str, int n) {
  if (!desc) return -1.0;
  XGlyphInfo i;
  XftTextExtents32(fl_display, desc->font, str, n, &i);
  return i.xOff;
}

double Fl_Xlib_Graphics_Driver::width(unsigned int c) {
  return fl_xft_width(font_descriptor(), (FcChar32 *)(&c), 1);
}

void Fl_Xlib_Graphics_Driver::text_extents(const char *c, int n, int &dx, int &dy, int &w, int &h) {
  if (!font_descriptor()) {
    w = h = 0;
    dx = dy = 0;
    return;
  }
  XGlyphInfo gi;
  utf8extents(font_descriptor(), c, n, &gi);

  w = gi.width;
  h = gi.height;
  dx = -gi.x;
  dy = -gi.y;
} // fl_text_extents


/* This code is used (mainly by opengl) to get a bitmapped font. The
 * original XFT-1 code used XFT's "core" fonts methods to load an XFT
 * font that was actually a X-bitmap font, that could then be readily
 * used with GL.  But XFT-2 does not provide that ability, and there
 * is no easy method to use an XFT font directly with GL. So...
*/

#  if XFT_MAJOR > 1
// This function attempts, on XFT2 systems, to find a suitable "core" Xfont
// for GL or other bitmap font needs (we dont have an XglUseXftFont(...) function.)
// There's probably a better way to do this. I can't believe it is this hard...
// Anyway... This code attempts to make an XLFD out of the fltk-style font
// name it is passed, then tries to load that font. Surprisingly, this quite
// often works - boxes that have XFT generally also have a fontserver that
// can serve TTF and other fonts to X, and so the font name that fltk makes
// from the XFT name often also "exists" as an "core" X font...
// If this code fails to load the requested font, it falls back through a
// series of tried 'n tested alternatives, ultimately resorting to what the
// original fltk code did.
// NOTE: On my test boxes (FC6, FC7, FC8, ubuntu8.04, 9.04, 9.10) this works 
//       well for the fltk "built-in" font names.
static XFontStruct* load_xfont_for_xft2(Fl_Graphics_Driver *driver) {
  XFontStruct* xgl_font = 0;
  int size = driver->size();
  int fnum = driver->font();
  const char *wt_med = "medium";
  const char *wt_bold = "bold";
  const char *weight = wt_med; // no specifc weight requested - accept any
  char slant = 'r';   // regular non-italic by default
  char xlfd[128];     // we will put our synthetic XLFD in here
  char *pc = strdup(fl_fonts[fnum].name); // what font were we asked for?
  const char *name = pc;    // keep a handle to the original name for freeing later
  // Parse the "fltk-name" of the font
  switch (*name++) {
  case 'I': slant = 'i'; break;       // italic
  case 'P': slant = 'i';              // bold-italic (falls-through)
  case 'B': weight = wt_bold; break;  // bold
  case ' ': break;                    // regular
  default: name--;                    // no prefix, restore name
  }

  // first, we do a query with no prefered size, to see if the font exists at all
  snprintf(xlfd, 128, "-*-%s-%s-%c-*--*-*-*-*-*-*-*-*", name, weight, slant); // make up xlfd style name
  xgl_font = XLoadQueryFont(fl_display, xlfd);
  if(xgl_font) { // the face exists, but can we get it in a suitable size?
    XFreeFont(fl_display, xgl_font); // release the non-sized version
    snprintf(xlfd, 128, "-*-%s-%s-%c-*--*-%d-*-*-*-*-*-*", name, weight, slant, (size*10));
    xgl_font = XLoadQueryFont(fl_display, xlfd); // attempt to load the font at the right size
  }
//puts(xlfd);

  // try alternative names
  if (!xgl_font) {
    if (!strcmp(name, "sans")) {
      name = "helvetica";
    } else if (!strcmp(name, "mono")) {
      name = "courier";
    } else if (!strcmp(name, "serif")) {
      name = "times";
    } else if (!strcmp(name, "screen")) {
      name = "lucidatypewriter";
    } else if (!strcmp(name, "dingbats")) {
      name = "zapf dingbats";
    }
    snprintf(xlfd, 128, "-*-*%s*-%s-%c-*--*-%d-*-*-*-*-*-*", name, weight, slant, (size*10));
    xgl_font = XLoadQueryFont(fl_display, xlfd);
  }  
  free(pc); // release our copy of the font name
  
  // if we have nothing loaded, try a generic proportional font
  if(!xgl_font) {
    snprintf(xlfd, 128, "-*-helvetica-*-%c-*--*-%d-*-*-*-*-*-*", slant, (size*10));
    xgl_font = XLoadQueryFont(fl_display, xlfd);
  }
  // If that still didn't work, try this instead
  if(!xgl_font) {
    snprintf(xlfd, 128, "-*-courier-medium-%c-*--*-%d-*-*-*-*-*-*", slant, (size*10));
    xgl_font = XLoadQueryFont(fl_display, xlfd);
  }
//printf("glf: %d\n%s\n%s\n", size, xlfd, fl_fonts[fl_font_].name);
//if(xgl_font) puts("ok");

  // Last chance fallback - this usually loads something...
  if (!xgl_font) xgl_font = XLoadQueryFont(fl_display, "fixed");

  return xgl_font;
} // end of load_xfont_for_xft2
#  endif

static XFontStruct* fl_xxfont(Fl_Graphics_Driver *driver) {
#  if XFT_MAJOR > 1
  // kludge! XFT 2 and later does not provide core fonts for us to use with GL
  // try to load a bitmap X font instead
  static XFontStruct* xgl_font = 0;
  static int glsize = 0;
  static int glfont = -1;
  // Do we need to load a new font?
  if ((!xgl_font) || (glsize != driver->size()) || (glfont != driver->font())) {
    // create a dummy XLFD for some font of the appropriate size...
    if (xgl_font) XFreeFont(fl_display, xgl_font); // font already loaded, free it - this *might* be a Bad Idea
    glsize = driver->size(); // record current font size
    glfont = driver->font(); // and face
    xgl_font = load_xfont_for_xft2(driver);
  }
  return xgl_font;
#  else // XFT-1 provides a means to load a "core" font directly
  if (driver->font_descriptor()->font->core) {
    return driver->font_descriptor()->font->u.core.font; // is the current font a "core" font? If so, use it.
    }
  static XftFont* xftfont;
  if (xftfont) XftFontClose (fl_display, xftfont);
  xftfont = fontopen(fl_fonts[driver->font()].name, driver->size(), true, 0); // else request XFT to load a suitable "core" font instead.
  return xftfont->u.core.font;
#  endif // XFT_MAJOR > 1
}

XFontStruct* Fl_XFont_On_Demand::value() {
  if (!ptr) ptr = fl_xxfont(fl_graphics_driver);
  return ptr;
}

#if USE_OVERLAY
// Currently Xft does not work with colormapped visuals, so this probably
// does not work unless you have a true-color overlay.
extern bool fl_overlay;
extern Colormap fl_overlay_colormap;
extern XVisualInfo* fl_overlay_visual;
#endif

// For some reason Xft produces errors if you destroy a window whose id
// still exists in an XftDraw structure. It would be nice if this is not
// true, a lot of junk is needed to try to stop this:

static XftDraw* draw_;
static Window draw_window;
#if USE_OVERLAY
static XftDraw* draw_overlay;
static Window draw_overlay_window;
#endif

void fl_destroy_xft_draw(Window id) {
  if (id == draw_window)
    XftDrawChange(draw_, draw_window = fl_message_window);
#if USE_OVERLAY
  if (id == draw_overlay_window)
    XftDrawChange(draw_overlay, draw_overlay_window = fl_message_window);
#endif
}

void Fl_Xlib_Graphics_Driver::draw(const char *str, int n, int x, int y) {
  if ( !this->font_descriptor() ) {
    this->font(FL_HELVETICA, FL_NORMAL_SIZE);
  }
#if USE_OVERLAY
  XftDraw*& draw_ = fl_overlay ? draw_overlay : ::draw_;
  if (fl_overlay) {
    if (!draw_)
      draw_ = XftDrawCreate(fl_display, draw_overlay_window = fl_window,
			   fl_overlay_visual->visual, fl_overlay_colormap);
    else //if (draw_overlay_window != fl_window)
      XftDrawChange(draw_, draw_overlay_window = fl_window);
  } else
#endif
  if (!draw_)
    draw_ = XftDrawCreate(fl_display, draw_window = fl_window,
			 fl_visual->visual, fl_colormap);
  else //if (draw_window != fl_window)
    XftDrawChange(draw_, draw_window = fl_window);

  Region region = fl_clip_region();
  if (region && XEmptyRegion(region)) return;
  XftDrawSetClip(draw_, region);

  // Use fltk's color allocator, copy the results to match what
  // XftCollorAllocValue returns:
  XftColor color;
  color.pixel = fl_xpixel(Fl_Graphics_Driver::color());
  uchar r,g,b; Fl::get_color(Fl_Graphics_Driver::color(), r,g,b);
  color.color.red   = ((int)r)*0x101;
  color.color.green = ((int)g)*0x101;
  color.color.blue  = ((int)b)*0x101;
  color.color.alpha = 0xffff;
  
  const wchar_t *buffer = utf8reformat(str, n);
#ifdef __CYGWIN__
  XftDrawString16(draw_, &color, font_descriptor()->font, x, y, (XftChar16 *)buffer, n);
#else
  XftDrawString32(draw_, &color, font_descriptor()->font, x, y, (XftChar32 *)buffer, n);
#endif
}

void Fl_Xlib_Graphics_Driver::draw(int angle, const char *str, int n, int x, int y) {
  fl_xft_font(this, this->Fl_Graphics_Driver::font(), this->size(), angle);
  this->draw(str, n, (int)x, (int)y);
  fl_xft_font(this, this->Fl_Graphics_Driver::font(), this->size(), 0);
}

static void fl_drawUCS4(Fl_Graphics_Driver *driver, const FcChar32 *str, int n, int x, int y) {
#if USE_OVERLAY
  XftDraw*& draw_ = fl_overlay ? draw_overlay : ::draw_;
  if (fl_overlay) {
    if (!draw_)
      draw_ = XftDrawCreate(fl_display, draw_overlay_window = fl_window,
			   fl_overlay_visual->visual, fl_overlay_colormap);
    else //if (draw_overlay_window != fl_window)
      XftDrawChange(draw_, draw_overlay_window = fl_window);
  } else
#endif
  if (!draw_)
    draw_ = XftDrawCreate(fl_display, draw_window = fl_window,
			 fl_visual->visual, fl_colormap);
  else //if (draw_window != fl_window)
    XftDrawChange(draw_, draw_window = fl_window);

  Region region = fl_clip_region();
  if (region && XEmptyRegion(region)) return;
  XftDrawSetClip(draw_, region);

  // Use fltk's color allocator, copy the results to match what
  // XftCollorAllocValue returns:
  XftColor color;
  color.pixel = fl_xpixel(driver->color());
  uchar r,g,b; Fl::get_color(driver->color(), r,g,b);
  color.color.red   = ((int)r)*0x101;
  color.color.green = ((int)g)*0x101;
  color.color.blue  = ((int)b)*0x101;
  color.color.alpha = 0xffff;

  XftDrawString32(draw_, &color, driver->font_descriptor()->font, x, y, (FcChar32 *)str, n);
}


void Fl_Xlib_Graphics_Driver::rtl_draw(const char* c, int n, int x, int y) {

#if defined(__GNUC__)
// FIXME: warning Need to improve this XFT right to left draw function
#endif /*__GNUC__*/

// This actually draws LtoR, but aligned to R edge with the glyph order reversed...
// but you can't just byte-rev a UTF-8 string, that isn't valid.
// You can reverse a UCS4 string though...
  int num_chars, wid, utf_len = strlen(c);
  FcChar8 *u8 = (FcChar8 *)c;
  FcBool valid = FcUtf8Len(u8, utf_len, &num_chars, &wid);
  if (!valid)
  {
    // badly formed Utf-8 input string
    return;
  }
  if (num_chars < n) n = num_chars; // limit drawing to usable characters in input array
  FcChar32 *ucs_txt = new FcChar32[n+1];
  FcChar32* pu;
  int in, out, sz;
  ucs_txt[n] = 0;
  in = 0; out = n-1;
  while ((out >= 0) && (utf_len > 0))
  {
    pu = &ucs_txt[out];
    sz = FcUtf8ToUcs4(u8, pu, utf_len);
    utf_len = utf_len - sz;
    u8 = u8 + sz;
    out = out - 1;
  }
  // Now we have a UCS4 version of the input text, reversed, in ucs_txt
  int offs = (int)fl_xft_width(font_descriptor(), ucs_txt, n);
  fl_drawUCS4(this, ucs_txt, n, (x-offs), y);

  delete[] ucs_txt;
}
#endif

//
// End of "$Id: fl_font_xft.cxx 8442 2011-02-18 13:39:48Z manolo $"
//
