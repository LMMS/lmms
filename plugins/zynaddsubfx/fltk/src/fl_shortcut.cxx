//
// "$Id: fl_shortcut.cxx 8621 2011-04-23 15:46:30Z AlbrechtS $"
//
// Shortcut support routines for the Fast Light Tool Kit (FLTK).
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

// Code to test and parse fltk shortcut numbers.
//
// A shortcut is a keysym or'd with shift flags.  In the simplest
// sense a shortcut is matched if the shift state is exactly as
// given and the key returning that keysym is pressed.
//
// To make it easier to match some things it is more complex:
//
// Only FL_META, FL_ALT, FL_SHIFT, and FL_CTRL must be "off".  A
// zero in the other shift flags indicates "don't care".
//
// It also checks against the first character of Fl::event_text(),
// and zero for FL_SHIFT means "don't care".
// This allows punctuation shortcuts like "#" to work (rather than
// calling it "shift+3" on a US keyboard)

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <ctype.h>
#include "flstring.h"
#if !defined(WIN32) && !defined(__APPLE__)
#include <FL/x.H>
#endif

/**
  Tests the current event, which must be an FL_KEYBOARD or
  FL_SHORTCUT, against a shortcut value (described in Fl_Button).

  Not to be confused with Fl_Widget::test_shortcut().

  \return non-zero if there is a match.
*/
int Fl::test_shortcut(unsigned int shortcut) {
  if (!shortcut) return 0;

  unsigned int v = shortcut & FL_KEY_MASK;
  if (((unsigned)fl_tolower(v))!=v) {
    shortcut |= FL_SHIFT;
  }

  int shift = Fl::event_state();
  // see if any required shift flags are off:
  if ((shortcut&shift) != (shortcut&0x7fff0000)) return 0;
  // record shift flags that are wrong:
  int mismatch = (shortcut^shift)&0x7fff0000;
  // these three must always be correct:
  if (mismatch&(FL_META|FL_ALT|FL_CTRL)) return 0;

  unsigned int key = shortcut & FL_KEY_MASK;

  // if shift is also correct, check for exactly equal keysyms:
  if (!(mismatch&(FL_SHIFT)) && key == (unsigned)Fl::event_key()) return 1;

  // try matching utf8, ignore shift:
  unsigned int firstChar = fl_utf8decode(Fl::event_text(), Fl::event_text()+Fl::event_length(), 0);
  if ( ! (FL_CAPS_LOCK&shift) && key==firstChar) return 1;

  // kludge so that Ctrl+'_' works (as opposed to Ctrl+'^_'):
  if ((shift&FL_CTRL) && key >= 0x3f && key <= 0x5F
      && firstChar==(key^0x40)) return 1; // firstChar should be within a-z
  return 0;
}

// This table must be in numeric order by fltk (X) keysym number:
struct Keyname {unsigned int key; const char* name;};
#if defined(WIN32)
static Keyname table[] = {
  {' ', "Space"},
  {FL_BackSpace, "Backspace"},
  {FL_Tab,	"Tab"},
  {0xff0b/*XK_Clear*/, "Clear"},
  {FL_Enter,	"Enter"}, // X says "Enter"
  {FL_Pause,	"Pause"},
  {FL_Scroll_Lock, "Scroll_Lock"},
  {FL_Escape,	"Escape"},
  {FL_Home,	"Home"},
  {FL_Left,	"Left"},
  {FL_Up,	"Up"},
  {FL_Right,	"Right"},
  {FL_Down,	"Down"},
  {FL_Page_Up,	"Page_Up"}, // X says "Prior"
  {FL_Page_Down,"Page_Down"}, // X says "Next"
  {FL_End,	"End"},
  {FL_Print,	"Print"},
  {FL_Insert,	"Insert"},
  {FL_Menu,	"Menu"},
  {FL_Num_Lock,	"Num_Lock"},
  {FL_KP_Enter,	"KP_Enter"},
  {FL_Shift_L,	"Shift_L"},
  {FL_Shift_R,	"Shift_R"},
  {FL_Control_L,"Control_L"},
  {FL_Control_R,"Control_R"},
  {FL_Caps_Lock,"Caps_Lock"},
  {FL_Meta_L,	"Meta_L"},
  {FL_Meta_R,	"Meta_R"},
  {FL_Alt_L,	"Alt_L"},
  {FL_Alt_R,	"Alt_R"},
  {FL_Delete,	"Delete"}
};
#elif defined(__APPLE__) 
static Keyname table[] = {
                                 // v - this column contains UTF-8 characters
  {' ', "Space"},
  {FL_BackSpace,"\xe2\x8c\xab"}, // erase to the left
  {FL_Tab,	"\xe2\x87\xa5"}, // rightwards arrow to bar
  {0xff0b,      "\xe2\x8c\xa6"}, // erase to the right
  {FL_Enter,	"\xe2\x86\xa9"}, // leftwards arrow with hook
  {FL_Pause,	"Pause"},
  {FL_Scroll_Lock, "Scroll_Lock"},
  {FL_Escape,	"\xe2\x90\x9b"},
  {FL_Home,	"\xe2\x86\x96"}, // north west arrow
  {FL_Left,	"\xe2\x86\x90"}, // leftwards arrow
  {FL_Up,	"\xe2\x86\x91"}, // upwards arrow
  {FL_Right,	"\xe2\x86\x92"}, // rightwards arrow
  {FL_Down,	"\xe2\x86\x93"}, // downwards arrow
  {FL_Page_Up,	"\xe2\x87\x9e"}, // upwards arrow with double stroke
  {FL_Page_Down,"\xe2\x87\x9f"}, // downwards arrow with double stroke
  {FL_End,	"\xe2\x86\x98"}, // south east arrow
  {FL_Print,	"Print"},
  {FL_Insert,	"Insert"},
  {FL_Menu,	"Menu"},
  {FL_Num_Lock,	"Num_Lock"},
  {FL_KP_Enter,	"\xe2\x8c\xa4"}, // up arrow head between two horizontal bars
  {FL_Shift_L,	"Shift_L"},
  {FL_Shift_R,	"Shift_R"},
  {FL_Control_L,"Control_L"},
  {FL_Control_R,"Control_R"},
  {FL_Caps_Lock,"\xe2\x87\xaa"}, // upwards white arrow from bar
  {FL_Meta_L,	"Meta_L"},
  {FL_Meta_R,	"Meta_R"},
  {FL_Alt_L,	"Alt_L"},
  {FL_Alt_R,	"Alt_R"},
  {FL_Delete,	"\xe2\x8c\xa7"}  // x in a rectangle box
};
#endif

/**
  Get a human-readable string from a shortcut value.

  Unparse a shortcut value as used by Fl_Button or Fl_Menu_Item into
  a human-readable string like "Alt+N". This only works if the shortcut
  is a character key or a numbered function key. If the shortcut is
  zero then an empty string is returned. The return value points at
  a static buffer that is overwritten with each call.

  \param [in] shortcut the integer value containing the ascii character or extended keystroke plus modifiers
  \return a pointer to a static buffer containing human readable text for the shortcut
  */
const char* fl_shortcut_label(unsigned int shortcut) {
  return fl_shortcut_label(shortcut, 0L);
}

/** 
  Get a human-readable string from a shortcut value.

  \param [in] shortcut the integer value containing the ascii character or extended keystroke plus modifiers
  \param [in] eom if this pointer is set, it will receive a pointer to the end of the modifier text
  \return a pointer to a static buffer containing human readable text for the shortcut
  \see fl_shortcut_label(unsigned int shortcut)
  */
const char* fl_shortcut_label(unsigned int shortcut, const char **eom) {
  static char buf[20];
  char *p = buf;
  if (eom) *eom = p;
  if (!shortcut) {*p = 0; return buf;}
  // fix upper case shortcuts
  unsigned int v = shortcut & FL_KEY_MASK;
  if (((unsigned)fl_tolower(v))!=v) {
    shortcut |= FL_SHIFT;
  }
#ifdef __APPLE__
  //                        this column contains utf8 characters - v
  if (shortcut & FL_SHIFT) {strcpy(p,"\xe2\x87\xa7"); p += 3;}  // upwards white arrow
  if (shortcut & FL_CTRL)  {strcpy(p,"\xe2\x8c\x83"); p += 3;}  // up arrowhead
  if (shortcut & FL_ALT)   {strcpy(p,"\xe2\x8c\xa5"); p += 3;}  // alternative key symbol
  if (shortcut & FL_META)  {strcpy(p,"\xe2\x8c\x98"); p += 3;}  // place of interest sign
#else
  if (shortcut & FL_META) {strcpy(p,"Meta+"); p += 5;}
  if (shortcut & FL_ALT) {strcpy(p,"Alt+"); p += 4;}
  if (shortcut & FL_SHIFT) {strcpy(p,"Shift+"); p += 6;}
  if (shortcut & FL_CTRL) {strcpy(p,"Ctrl+"); p += 5;}
#endif // __APPLE__
  if (eom) *eom = p;
  unsigned int key = shortcut & FL_KEY_MASK;
#if defined(WIN32) || defined(__APPLE__) // if not X
  if (key >= FL_F && key <= FL_F_Last) {
    *p++ = 'F';
    if (key > FL_F+9) *p++ = (key-FL_F)/10+'0';
    *p++ = (key-FL_F)%10 + '0';
  } else {
    // binary search the table for a match:
    int a = 0;
    int b = sizeof(table)/sizeof(*table);
    while (a < b) {
      int c = (a+b)/2;
      if (table[c].key == key) {
        if (p > buf) {
          strcpy(p,table[c].name); 
          return buf;
        } else {
          const char *sp = table[c].name;
          if (eom) *eom = sp;
          return sp;
        }
      }
      if (table[c].key < key) a = c+1;
      else b = c;
    }
    if (key >= FL_KP && key <= FL_KP_Last) {
      // mark keypad keys with KP_ prefix
      strcpy(p,"KP_"); p += 3;
      *p++ = uchar(key & 127);
    } else {
      // if none found, use the keystroke as a match:
      p += fl_utf8encode(fl_toupper(key), p); 
    }
  }
  *p = 0;
  return buf;
#else
  const char* q;
  if (key == FL_Enter || key == '\r') q="Enter";  // don't use Xlib's "Return":
  else if (key > 32 && key < 0x100) q = 0;
  else q = XKeysymToString(key);
  if (!q) {
    p += fl_utf8encode(fl_toupper(key), p); 
    *p = 0; 
    return buf;
  }
  if (p > buf) {
    strcpy(p,q); 
    return buf;
  } else {
    if (eom) *eom = q;
    return q;
  }
#endif
}

// Emulation of XForms named shortcuts
#include <stdlib.h>
/**
  Emulation of XForms named shortcuts.

  Converts ascii shortcut specifications (eg. "^c") 
  into the FLTK integer equivalent (eg. FL_CTRL+'c')

  These ascii characters are used to specify the various keyboard modifier keys:
  \verbatim
   # - Alt
   + - Shift
   ^ - Control
  \endverbatim
*/
unsigned int fl_old_shortcut(const char* s) {
  if (!s || !*s) return 0;
  unsigned int n = 0;
  if (*s == '#') {n |= FL_ALT; s++;}
  if (*s == '+') {n |= FL_SHIFT; s++;}
  if (*s == '^') {n |= FL_CTRL; s++;}
  if (*s && s[1]) return n | (int)strtol(s,0,0); // allow 0xf00 to get any key
  return n | *s;
}

// Tests for &x shortcuts in button labels:

/** Returns the Unicode value of the '&x' shortcut in a given text.

  The given text \p t (usually a widget's label or a menu text) is
  searched for a '&x' shortcut label, and if found, the Unicode
  value (code point) of the '&x' shortcut is returned.

  \param t text or label to search for '&x' shortcut.

  \return Unicode (UCS-4) value of shortcut in \p t or 0.

  \note Internal use only.
*/
unsigned int Fl_Widget::label_shortcut(const char *t) {
  if (!t) return 0;
  for (;;) {
    if (*t==0) return 0;
    if (*t=='&') {
      unsigned int s = fl_utf8decode(t+1, 0, 0);
      if (s==0) return 0;
      else if (s==(unsigned int)'&') t++;
      else return s;
    }
    t++;
  }
}

/** Returns true if the given text \p t contains the entered '&x' shortcut.

  This method must only be called in handle() methods or callbacks after
  a keypress event (usually FL_KEYDOWN or FL_SHORTCUT). The given text
  \p t (usually a widget's label or menu text) is searched for a '&x'
  shortcut, and if found, this is compared with the entered key value.

  Fl::event_text() is used to get the entered key value.
  Fl::event_state() is used to get the Alt modifier, if \p require_alt
  is true.

  \param t text or label to search for '&x' shortcut.
  \param require_alt if true: match only if Alt key is pressed.

  \return true, if the entered text matches the '&x' shortcut in \p t
	  false (0) otherwise.

  \note Internal use only.
*/
int Fl_Widget::test_shortcut(const char *t, const bool require_alt) {
  if (!t) return 0;
  // for menubars etc. shortcuts must work only if the Alt modifier is pressed
  if (require_alt && Fl::event_state(FL_ALT)==0) return 0;
  unsigned int c = fl_utf8decode(Fl::event_text(), Fl::event_text()+Fl::event_length(), 0);
#ifdef __APPLE__
  // this line makes underline shortcuts work the same way they do on MSWindow
  // and Linux. 
  if (c && Fl::event_state(FL_ALT)) 
    c = Fl::event_key();
#endif
  if (!c) return 0;
  unsigned int ls = label_shortcut(t);
  if (c == ls)
    return 1;
#ifdef __APPLE__
  // On OS X, we need to simulate the upper case keystroke as well
  if (Fl::event_state(FL_ALT) && c<128 && isalpha(c) && (unsigned)toupper(c)==ls)
    return 1;
#endif
  return 0;
}

/** Returns true if the widget's label contains the entered '&x' shortcut.

  This method must only be called in handle() methods or callbacks after
  a keypress event (usually FL_KEYDOWN or FL_SHORTCUT).
  The widget's label is searched for a '&x'
  shortcut, and if found, this is compared with the entered key value.

  Fl::event_text() is used to get the entered key value.

  \return true, if the entered text matches the widget's'&x' shortcut,
	  false (0) otherwise.

  \note Internal use only.
*/

int Fl_Widget::test_shortcut() {
  if (!(flags()&SHORTCUT_LABEL)) return 0;
  return test_shortcut(label());
}

//
// End of "$Id: fl_shortcut.cxx 8621 2011-04-23 15:46:30Z AlbrechtS $".
//
