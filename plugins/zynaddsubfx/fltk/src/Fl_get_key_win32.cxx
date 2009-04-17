//
// "$Id: Fl_get_key_win32.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// WIN32 keyboard state routines for the Fast Light Tool Kit (FLTK).
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

// Return the current state of a key.  Keys are named by fltk symbols,
// which are actually X keysyms.  So this has to translate to MSWindows
// VK_x symbols.

#include <FL/Fl.H>
#include <FL/x.H>

// convert an Fltk (X) keysym to a MSWindows VK symbol:
// See also the inverse converter in Fl_win32.cxx
// This table is in numeric order by Fltk symbol order for binary search:

static const struct {unsigned short vk, fltk;} vktab[] = {
  {VK_SPACE,	' '},
  {'1',		'!'},
  {0xde,	'\"'},
  {'3',		'#'},
  {'4',		'$'},
  {'5',		'%'},
  {'7',		'&'},
  {0xde,	'\''},
  {'9',		'('},
  {'0',		')'},
  {'8',		'*'},
  {0xbb,	'+'},
  {0xbc,	','},
  {0xbd,	'-'},
  {0xbe,	'.'},
  {0xbf,	'/'},
  {0xba,	':'},
  {0xba,	';'},
  {0xbc,	'<'},
  {0xbb,	'='},
  {0xbe,	'>'},
  {0xbf,	'?'},
  {'2',		'@'},
  {0xdb,	'['},
  {0xdc,	'\\'},
  {0xdd,	']'},
  {'6',		'^'},
  {0xbd,	'_'},
  {0xc0,	'`'},
  {0xdb,	'{'},
  {0xdc,	'|'},
  {0xdd,	'}'},
  {0xc0,	'~'},
  {VK_BACK,	FL_BackSpace},
  {VK_TAB,	FL_Tab},
  {VK_CLEAR,	0xff0b/*XK_Clear*/},
  {VK_RETURN,	FL_Enter},
  {VK_PAUSE,	FL_Pause},
  {VK_SCROLL,	FL_Scroll_Lock},
  {VK_ESCAPE,	FL_Escape},
  {VK_HOME,	FL_Home},
  {VK_LEFT,	FL_Left},
  {VK_UP,	FL_Up},
  {VK_RIGHT,	FL_Right},
  {VK_DOWN,	FL_Down},
  {VK_PRIOR,	FL_Page_Up},
  {VK_NEXT,	FL_Page_Down},
  {VK_END,	FL_End},
  {VK_SNAPSHOT,	FL_Print},
  {VK_INSERT,	FL_Insert},
  {VK_APPS,	FL_Menu},
  {VK_NUMLOCK,	FL_Num_Lock},
//{VK_???,	FL_KP_Enter},
  {VK_MULTIPLY,	FL_KP+'*'},
  {VK_ADD,	FL_KP+'+'},
  {VK_SUBTRACT,	FL_KP+'-'},
  {VK_DECIMAL,	FL_KP+'.'},
  {VK_DIVIDE,	FL_KP+'/'},
  {VK_LSHIFT,	FL_Shift_L},
  {VK_RSHIFT,	FL_Shift_R},
  {VK_LCONTROL,	FL_Control_L},
  {VK_RCONTROL,	FL_Control_R},
  {VK_CAPITAL,	FL_Caps_Lock},
  {VK_LWIN,	FL_Meta_L},
  {VK_RWIN,	FL_Meta_R},
  {VK_LMENU,	FL_Alt_L},
  {VK_RMENU,	FL_Alt_R},
  {VK_DELETE,	FL_Delete}
};

static int fltk2ms(int fltk) {
  if (fltk >= '0' && fltk <= '9') return fltk;
  if (fltk >= 'A' && fltk <= 'Z') return fltk;
  if (fltk >= 'a' && fltk <= 'z') return fltk-('a'-'A');
  if (fltk > FL_F && fltk <= FL_F+16) return fltk-(FL_F-(VK_F1-1));
  if (fltk >= FL_KP+'0' && fltk<=FL_KP+'9') return fltk-(FL_KP+'0'-VK_NUMPAD0);
  int a = 0;
  int b = sizeof(vktab)/sizeof(*vktab);
  while (a < b) {
    int c = (a+b)/2;
    if (vktab[c].fltk == fltk) return vktab[c].vk;
    if (vktab[c].fltk < fltk) a = c+1; else b = c;
  }
  return 0;
}

int Fl::event_key(int k) {
  return GetKeyState(fltk2ms(k))&~1;
}

int Fl::get_key(int k) {
  uchar foo[256];
  GetKeyboardState(foo);
  return foo[fltk2ms(k)]&~1;
}

//
// End of "$Id: Fl_get_key_win32.cxx 5190 2006-06-09 16:16:34Z mike $".
//
