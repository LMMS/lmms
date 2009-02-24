//
// "$Id: Fl_grab.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Grab/release code for the Fast Light Tool Kit (FLTK).
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
#include <FL/Fl.H>
#include <FL/x.H>

////////////////////////////////////////////////////////////////
// "Grab" is done while menu systems are up.  This has several effects:
// Events are all sent to the "grab window", which does not even
// have to be displayed (and in the case of Fl_Menu.cxx it isn't).
// The system is also told to "grab" events and send them to this app.
// This also modifies how Fl_Window::show() works, on X it turns on
// override_redirect, it does similar things on WIN32.

extern void fl_fix_focus(); // in Fl.cxx

#ifdef WIN32
// We have to keep track of whether we have captured the mouse, since
// MSWindows shows little respect for this... Grep for fl_capture to
// see where and how this is used.
extern HWND fl_capture;
#endif

#ifdef __APPLE__
// MacOS Carbon does not seem to have a mechanism to grab the mouse pointer
extern WindowRef fl_capture;
#endif

void Fl::grab(Fl_Window* win) {
  if (win) {
    if (!grab_) {
#ifdef WIN32
      SetActiveWindow(fl_capture = fl_xid(first_window()));
      SetCapture(fl_capture);
#elif defined(__APPLE__)
      fl_capture = fl_xid( first_window() );
      SetUserFocusWindow( fl_capture );
#else
      XGrabPointer(fl_display,
		   fl_xid(first_window()),
		   1,
		   ButtonPressMask|ButtonReleaseMask|
		   ButtonMotionMask|PointerMotionMask,
		   GrabModeAsync,
		   GrabModeAsync, 
		   None,
		   0,
		   fl_event_time);
      XGrabKeyboard(fl_display,
		    fl_xid(first_window()),
		    1,
		    GrabModeAsync,
		    GrabModeAsync, 
		    fl_event_time);
#endif
    }
    grab_ = win;
  } else {
    if (grab_) {
#ifdef WIN32
      fl_capture = 0;
      ReleaseCapture();
#elif defined(__APPLE__)
      fl_capture = 0;
      SetUserFocusWindow( (WindowRef)kUserFocusAuto );
#else
      XUngrabKeyboard(fl_display, fl_event_time);
      XUngrabPointer(fl_display, fl_event_time);
      // this flush is done in case the picked menu item goes into
      // an infinite loop, so we don't leave the X server locked up:
      XFlush(fl_display);
#endif
      grab_ = 0;
      fl_fix_focus();
    }
  }
}

//
// End of "$Id: Fl_grab.cxx 5190 2006-06-09 16:16:34Z mike $".
//
