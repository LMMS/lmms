//
// "$Id: Fl_arg.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Optional argument initialization code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

// OPTIONAL initialization code for a program using fltk.
// You do not need to call this!  Feel free to make up your own switches.

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tooltip.H>
#include <FL/filename.H>
#include <FL/fl_draw.H>
#include <ctype.h>
#include "flstring.h"

#if defined(WIN32) || defined(__APPLE__)
int XParseGeometry(const char*, int*, int*, unsigned int*, unsigned int*);
#  define NoValue	0x0000
#  define XValue  	0x0001
#  define YValue	0x0002
#  define WidthValue  	0x0004
#  define HeightValue  	0x0008
#  define AllValues 	0x000F
#  define XNegative 	0x0010
#  define YNegative 	0x0020
#endif

static int fl_match(const char *a, const char *s, int atleast = 1) {
  const char *b = s;
  while (*a && (*a == *b || tolower(*a) == *b)) {a++; b++;}
  return !*a && b >= s+atleast;
}

// flags set by previously parsed arguments:
extern char fl_show_iconic; // in Fl_x.cxx
static char arg_called;
static char return_i;
static const char *name;
static const char *geometry;
static const char *title;
// these are in Fl_get_system_colors and are set by the switches:
extern const char *fl_fg;
extern const char *fl_bg;
extern const char *fl_bg2;

/**
  Parse a single switch from \p argv, starting at word \p i.
  Returns the number of words eaten (1 or 2, or 0 if it is not
  recognized) and adds the same value to \p i. 
  
  This is the default argument handler used internally by Fl::args(...),
  but you can use this function if you prefer to step through the
  standard FLTK switches yourself.
  
  All standard FLTK switches except -bg2 may be abbreviated to just
  one letter and case is ignored:
  
  \li -bg color or -background color
  <br>
  Sets the background color using Fl::background().
  
  \li -bg2 color or -background2 color
  <br>
  Sets the secondary background color using Fl::background2().
  
  \li -display host:n.n
  <br>
  Sets the X display to use; this option is silently
  ignored under WIN32 and MacOS.
  
  \li -dnd and -nodnd
  <br>
  Enables or disables drag and drop text operations
  using Fl::dnd_text_ops().
  
  \li -fg color or -foreground color
  <br>
  Sets the foreground color using Fl::foreground().
  
  \li -geometry WxH+X+Y
  <br>
  Sets the initial window position and size according
  to the standard X geometry string.
  
  \li -iconic
  <br>
  Iconifies the window using Fl_Window::iconize().
  
  \li -kbd and -nokbd
  <br>
  Enables or disables visible keyboard focus for
  non-text widgets using Fl::visible_focus().
  
  \li -name string
  <br>
  Sets the window class using Fl_Window::xclass().
  
  \li -scheme string
  <br>
  Sets the widget scheme using Fl::scheme().
  
  \li -title string
  <br>
  Sets the window title using Fl_Window::label().
  
  \li -tooltips and -notooltips
  <br>
  Enables or disables tooltips using Fl_Tooltip::enable().
  
  
  If your program requires other switches in addition to the standard
  FLTK options, you will need to pass your own argument handler to
  Fl::args(int,char**,int&,Fl_Args_Handler) explicitly.
*/
int Fl::arg(int argc, char **argv, int &i) {
  arg_called = 1;
  const char *s = argv[i];

  if (!s) {i++; return 1;}	// something removed by calling program?

  // a word that does not start with '-', or a word after a '--', or
  // the word '-' by itself all start the "non-switch arguments" to
  // a program.  Return 0 to indicate that we don't understand the
  // word, but set a flag (return_i) so that args() will return at
  // that point:
  if (s[0] != '-' || s[1] == '-' || !s[1]) {return_i = 1; return 0;}
  s++; // point after the dash

  if (fl_match(s, "iconic")) {
    fl_show_iconic = 1;
    i++;
    return 1;
  } else if (fl_match(s, "kbd")) {
    Fl::visible_focus(1);
    i++;
    return 1;
  } else if (fl_match(s, "nokbd", 3)) {
    Fl::visible_focus(0);
    i++;
    return 1;
  } else if (fl_match(s, "dnd", 2)) {
    Fl::dnd_text_ops(1);
    i++;
    return 1;
  } else if (fl_match(s, "nodnd", 3)) {
    Fl::dnd_text_ops(0);
    i++;
    return 1;
  } else if (fl_match(s, "tooltips", 2)) {
    Fl_Tooltip::enable();
    i++;
    return 1;
  } else if (fl_match(s, "notooltips", 3)) {
    Fl_Tooltip::disable();
    i++;
    return 1;
  }
#ifdef __APPLE__
  // The Finder application in MacOS X passes the "-psn_N_NNNNN" option
  // to all apps...
  else if (strncmp(s, "psn_", 4) == 0) {
    i++;
    return 1;
  }
#endif // __APPLE__

  const char *v = argv[i+1];
  if (i >= argc-1 || !v)
    return 0;	// all the rest need an argument, so if missing it is an error

  if (fl_match(s, "geometry")) {

    int flags, gx, gy; unsigned int gw, gh;
    flags = XParseGeometry(v, &gx, &gy, &gw, &gh);
    if (!flags) return 0;
    geometry = v;

#if !defined(WIN32) && !defined(__APPLE__)
  } else if (fl_match(s, "display", 2)) {
    Fl::display(v);
#endif

  } else if (fl_match(s, "title", 2)) {
    title = v;

  } else if (fl_match(s, "name", 2)) {
    name = v;

  } else if (fl_match(s, "bg2", 3) || fl_match(s, "background2", 11)) {
    fl_bg2 = v;

  } else if (fl_match(s, "bg", 2) || fl_match(s, "background", 10)) {
    fl_bg = v;

  } else if (fl_match(s, "fg", 2) || fl_match(s, "foreground", 10)) {
    fl_fg = v;

  } else if (fl_match(s, "scheme", 1)) {
    Fl::scheme(v);

  } else return 0; // unrecognized

  i += 2;
  return 2;
}


/**
  Parse command line switches using the \p cb argument handler.
  
  Returns 0 on error, or the number of words processed.
  
  FLTK provides this as an <i>entirely optional</i> command line
  switch parser. You don't have to call it if you don't want to.
  Everything it can do can be done with other calls to FLTK.
  
  To use the switch parser, call Fl::args(...) near the start
  of your program.  This does \b not open the display, instead
  switches that need the display open are stashed into static
  variables. Then you \b must display your first window by calling
  <tt>window->show(argc,argv)</tt>, which will do anything stored
  in the static variables.
  
  Providing an argument handler callback \p cb lets you define
  your own switches. It is called with the same \p argc and \p argv,
  and with \p i set to the index of the switch to be processed.
  The \p cb handler should return zero if the switch is unrecognized,
  and not change \p i. It should return non-zero to indicate the
  number of words processed if the switch is recognized, i.e. 1 for
  just the switch, and more than 1 for the switch plus associated
  parameters. \p i should be incremented by the same amount.
  
  The \p cb handler is called \b before any other tests, so
  <i>you can also override any standard FLTK switch</i>
  (this is why FLTK can use very short switches instead of
  the long ones all other toolkits force you to use).
  See Fl::arg() for descriptions of the standard switches.
 
  On return \p i is set to the index of the first non-switch.
  This is either:
  
  \li The first word that does not start with '-'.
  \li The word '-' (used by many programs to name stdin as a file)
  \li The first unrecognized switch (return value is 0).
  \li \p argc
  
  The return value is \p i unless an unrecognized switch is found,
  in which case it is zero. If your program takes no arguments other
  than switches you should produce an error if the return value is less
  than \p argc.
  
  
  A usage string is displayed if Fl::args() detects an invalid argument
  on the command-line. You can change the message by setting the
  Fl::help pointer.
  
  A very simple command line parser can be found in <tt>examples/howto-parse-args.cxx</tt>
  
  The simpler Fl::args(int argc, char **argv) form is useful if your program
  does not have command line switches of its own.
*/

int Fl::args(int argc, char** argv, int& i, Fl_Args_Handler cb) {
  arg_called = 1;
  i = 1; // skip argv[0]
  while (i < argc) {
    if (cb && cb(argc,argv,i)) continue;
    if (!arg(argc,argv,i)) return return_i ? i : 0;
  }
  return i;
}

// show a main window, use any parsed arguments
void Fl_Window::show(int argc, char **argv) {
  if (argc && !arg_called) Fl::args(argc,argv);

  Fl::get_system_colors();

#if !defined(WIN32) && !defined(__APPLE__)
  // Get defaults for drag-n-drop and focus...
  const char *key = 0, *val;

  if (Fl::first_window()) key = Fl::first_window()->xclass();
  if (!key) key = "fltk";

  val = XGetDefault(fl_display, key, "dndTextOps");
  if (val) Fl::dnd_text_ops(strcasecmp(val, "true") == 0 ||
                            strcasecmp(val, "on") == 0 ||
                            strcasecmp(val, "yes") == 0);

  val = XGetDefault(fl_display, key, "tooltips");
  if (val) Fl_Tooltip::enable(strcasecmp(val, "true") == 0 ||
                              strcasecmp(val, "on") == 0 ||
                              strcasecmp(val, "yes") == 0);

  val = XGetDefault(fl_display, key, "visibleFocus");
  if (val) Fl::visible_focus(strcasecmp(val, "true") == 0 ||
                             strcasecmp(val, "on") == 0 ||
                             strcasecmp(val, "yes") == 0);
#endif // !WIN32 && !__APPLE__

  // set colors first, so background_pixel is correct:
  static char beenhere;
  if (!beenhere) {
    if (geometry) {
      int fl = 0, gx = x(), gy = y(); unsigned int gw = w(), gh = h();
      fl = XParseGeometry(geometry, &gx, &gy, &gw, &gh);
      if (fl & XNegative) gx = Fl::w()-w()+gx;
      if (fl & YNegative) gy = Fl::h()-h()+gy;
      //  int mw,mh; minsize(mw,mh);
      //  if (mw > gw) gw = mw;
      //  if (mh > gh) gh = mh;
      Fl_Widget *r = resizable();
      if (!r) resizable(this);
      // for WIN32 we assume window is not mapped yet:
      if (fl & (XValue | YValue))
	x(-1), resize(gx,gy,gw,gh);
      else
	size(gw,gh);
      resizable(r);
    }
  }

  // set the class, which is used by X version of get_system_colors:
  if (name) {xclass(name); name = 0;}
  else if (!xclass()) xclass(fl_filename_name(argv[0]));

  if (title) {label(title); title = 0;}
  else if (!label()) label(xclass());

  if (!beenhere) {
    beenhere = 1;
    Fl::scheme(Fl::scheme()); // opens display!  May call Fl::fatal()
  }

  // Show the window AFTER we have set the colors and scheme.
  show();

#if !defined(WIN32) && !defined(__APPLE__)
  // set the command string, used by state-saving window managers:
  int j;
  int n=0; for (j=0; j<argc; j++) n += strlen(argv[j])+1;
  char *buffer = new char[n];
  char *p = buffer;
  for (j=0; j<argc; j++) for (const char *q = argv[j]; (*p++ = *q++););
  XChangeProperty(fl_display, fl_xid(this), XA_WM_COMMAND, XA_STRING, 8, 0,
		  (unsigned char *)buffer, p-buffer-1);
  delete[] buffer;
#endif // !WIN32 && !__APPLE__
}

// Calls useful for simple demo programs, with automatic help message:

static const char * const helpmsg =
"options are:\n"
" -bg2 color\n"
" -bg color\n"
" -di[splay] host:n.n\n"
" -dn[d]\n"
" -fg color\n"
" -g[eometry] WxH+X+Y\n"
" -i[conic]\n"
" -k[bd]\n"
" -na[me] classname\n"
" -nod[nd]\n"
" -nok[bd]\n"
" -not[ooltips]\n"
" -s[cheme] scheme\n"
" -ti[tle] windowtitle\n"
" -to[oltips]";

const char * const Fl::help = helpmsg+13;

/**
 Parse all command line switches matching standard FLTK options only.
 
 It parses all the switches, and if any are not recognized it calls
 Fl::abort(Fl::help), i.e. unlike the long form, an unrecognized
 switch generates an error message and causes the program to exit.
 
 */
void Fl::args(int argc, char **argv) {
  int i; if (Fl::args(argc,argv,i) < argc) Fl::error(helpmsg);
}

#if defined(WIN32) || defined(__APPLE__)

/* the following function was stolen from the X sources as indicated. */

/* Copyright 	Massachusetts Institute of Technology  1985, 1986, 1987 */
/* $XConsortium: XParseGeom.c,v 11.18 91/02/21 17:23:05 rws Exp $ */

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

/*
 *    XParseGeometry parses strings of the form
 *   "=<width>x<height>{+-}<xoffset>{+-}<yoffset>", where
 *   width, height, xoffset, and yoffset are unsigned integers.
 *   Example:  "=80x24+300-49"
 *   The equal sign is optional.
 *   It returns a bitmask that indicates which of the four values
 *   were actually found in the string.  For each value found,
 *   the corresponding argument is updated;  for each value
 *   not found, the corresponding argument is left unchanged. 
 */

static int ReadInteger(char* string, char** NextString)
{
  register int Result = 0;
  int Sign = 1;
    
  if (*string == '+')
    string++;
  else if (*string == '-') {
    string++;
    Sign = -1;
  }
  for (; (*string >= '0') && (*string <= '9'); string++) {
    Result = (Result * 10) + (*string - '0');
  }
  *NextString = string;
  if (Sign >= 0)
    return (Result);
  else
    return (-Result);
}

int XParseGeometry(const char* string, int* x, int* y,
		   unsigned int* width, unsigned int* height)
{
  int mask = NoValue;
  register char *strind;
  unsigned int tempWidth = 0, tempHeight = 0;
  int tempX = 0, tempY = 0;
  char *nextCharacter;

  if ( (string == NULL) || (*string == '\0')) return(mask);
  if (*string == '=')
    string++;  /* ignore possible '=' at beg of geometry spec */

  strind = (char *)string;
  if (*strind != '+' && *strind != '-' && *strind != 'x') {
    tempWidth = ReadInteger(strind, &nextCharacter);
    if (strind == nextCharacter) 
      return (0);
    strind = nextCharacter;
    mask |= WidthValue;
  }

  if (*strind == 'x' || *strind == 'X') {	
    strind++;
    tempHeight = ReadInteger(strind, &nextCharacter);
    if (strind == nextCharacter)
      return (0);
    strind = nextCharacter;
    mask |= HeightValue;
  }

  if ((*strind == '+') || (*strind == '-')) {
    if (*strind == '-') {
      strind++;
      tempX = -ReadInteger(strind, &nextCharacter);
      if (strind == nextCharacter)
	return (0);
      strind = nextCharacter;
      mask |= XNegative;

    } else {
      strind++;
      tempX = ReadInteger(strind, &nextCharacter);
      if (strind == nextCharacter)
	return(0);
      strind = nextCharacter;
      }
    mask |= XValue;
    if ((*strind == '+') || (*strind == '-')) {
      if (*strind == '-') {
	strind++;
	tempY = -ReadInteger(strind, &nextCharacter);
	if (strind == nextCharacter)
	  return(0);
	strind = nextCharacter;
	mask |= YNegative;

      } else {
	strind++;
	tempY = ReadInteger(strind, &nextCharacter);
	if (strind == nextCharacter)
	  return(0);
	strind = nextCharacter;
      }
      mask |= YValue;
    }
  }
	
  /* If strind isn't at the end of the string the it's an invalid
     geometry specification. */

  if (*strind != '\0') return (0);

  if (mask & XValue)
    *x = tempX;
  if (mask & YValue)
    *y = tempY;
  if (mask & WidthValue)
    *width = tempWidth;
  if (mask & HeightValue)
    *height = tempHeight;
  return (mask);
}

#endif // ifdef WIN32

//
// End of "$Id: Fl_arg.cxx 7903 2010-11-28 21:06:39Z matt $".
//
