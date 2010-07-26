//
// "$Id: screen_xywh.cxx 7351 2010-03-29 10:35:00Z matt $"
//
// Screen/monitor bounding box API for the Fast Light Tool Kit (FLTK).
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


#include <FL/Fl.H>
#include <FL/x.H>
#include <config.h>


// Number of screens...
static int num_screens = 0;

#ifdef WIN32
#  if !defined(HMONITOR_DECLARED) && (_WIN32_WINNT < 0x0500)
#    define COMPILE_MULTIMON_STUBS
#    include <multimon.h>
#  endif // !HMONITOR_DECLARED && _WIN32_WINNT < 0x0500

// We go the much more difficult route of individually picking some multi-screen
// functions from the USER32.DLL . If these functions are not available, we
// will gracefully fall back to single monitor support.
//
// If we were to insist on the existence of "EnumDisplayMonitors" and 
// "GetMonitorInfoA", it would be impossible to use FLTK on Windows 2000
// before SP2 or earlier.

// BOOL EnumDisplayMonitors(HDC, LPCRECT, MONITORENUMPROC, LPARAM)
typedef BOOL (WINAPI* fl_edm_func)(HDC, LPCRECT, MONITORENUMPROC, LPARAM);
// BOOL GetMonitorInfo(HMONITOR, LPMONITORINFO)
typedef BOOL (WINAPI* fl_gmi_func)(HMONITOR, LPMONITORINFO);

static fl_gmi_func fl_gmi = NULL; // used to get a proc pointer for GetMonitorInfoA

static RECT screens[16];

static BOOL CALLBACK screen_cb(HMONITOR mon, HDC, LPRECT r, LPARAM) {
  if (num_screens >= 16) return TRUE;

  MONITORINFO mi;
  mi.cbSize = sizeof(mi);

//  GetMonitorInfo(mon, &mi);
//  (but we use our self-aquired function pointer instead)
  if (fl_gmi(mon, &mi)) {
    screens[num_screens] = mi.rcWork;
    num_screens ++;
  }
  return TRUE;
}

static void screen_init() {
  // Since not all versions of Windows include multiple monitor support,
  // we do a run-time check for the required functions...
  HMODULE hMod = GetModuleHandle("USER32.DLL");

  if (hMod) {
    // check that EnumDisplayMonitors is available
    fl_edm_func fl_edm = (fl_edm_func)GetProcAddress(hMod, "EnumDisplayMonitors");

    if (fl_edm) {
      // We do have EnumDisplayMonitors, so lets find out how many monitors...
      num_screens = GetSystemMetrics(SM_CMONITORS);

      if (num_screens > 1) {
        // If there is more than 1 monitor, enumerate them...
        fl_gmi = (fl_gmi_func)GetProcAddress(hMod, "GetMonitorInfoA");

        if (fl_gmi) {
          // We have GetMonitorInfoA, enumerate all the screens...
          num_screens = 0;
//        EnumDisplayMonitors(0,0,screen_cb,0);
//        (but we use our self-aquired function pointer instead)
          fl_edm(0, 0, screen_cb, 0);
          return;
        }
      }
    }
  }

  // If we get here, assume we have 1 monitor...
  num_screens = 1;
}
#elif defined(__APPLE__)
XRectangle screens[16];

extern int MACscreen_init(XRectangle screens[]);
static void screen_init() {
  num_screens = MACscreen_init(screens);
}
#elif HAVE_XINERAMA
#  include <X11/extensions/Xinerama.h>

// Screen data...
static XineramaScreenInfo *screens;

static void screen_init() {
  if (!fl_display) fl_open_display();

  if (XineramaIsActive(fl_display)) {
    screens = XineramaQueryScreens(fl_display, &num_screens);
  } else num_screens = 1;
}
#else
static void screen_init() {
  num_screens = 1;
}
#endif // WIN32


/**
  Gets the number of available screens.
*/
int Fl::screen_count() {
  if (!num_screens) screen_init();

  return num_screens;
}

/**
  Gets the bounding box of a screen 
  that contains the specified screen position \p mx, \p my
  \param[out]  X,Y,W,H the corresponding screen bounding box
  \param[in] mx, my the absolute screen position
*/
void Fl::screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my) {
  if (!num_screens) screen_init();

#ifdef WIN32
  if (num_screens > 1) {
    int i;

    for (i = 0; i < num_screens; i ++) {
      if (mx >= screens[i].left && mx < screens[i].right &&
	  my >= screens[i].top && my < screens[i].bottom) {
	X = screens[i].left;
	Y = screens[i].top;
	W = screens[i].right - screens[i].left;
	H = screens[i].bottom - screens[i].top;
	return;
      }
    }
  }
#elif defined(__APPLE__)
  if (num_screens > 1) {
    int i;

    for (i = 0; i < num_screens; i ++) {
      if (mx >= screens[i].x &&
	  mx < (screens[i].x + screens[i].width) &&
	  my >= screens[i].y &&
	  my < (screens[i].y + screens[i].height)) {
	X = screens[i].x;
	Y = screens[i].y;
	W = screens[i].width;
	H = screens[i].height;
	return;
      }
    }
  }
#elif HAVE_XINERAMA
  if (num_screens > 1) {
    int i;

    for (i = 0; i < num_screens; i ++) {
      if (mx >= screens[i].x_org &&
	  mx < (screens[i].x_org + screens[i].width) &&
	  my >= screens[i].y_org &&
	  my < (screens[i].y_org + screens[i].height)) {
	X = screens[i].x_org;
	Y = screens[i].y_org;
	W = screens[i].width;
	H = screens[i].height;
	return;
      }
    }
  }
#else
  (void)mx;
  (void)my;
#endif // WIN32

  X = Fl::x();
  Y = Fl::y();
  W = Fl::w();
  H = Fl::h();
}

/**
  Gets the screen bounding rect for the given screen. 
  \param[out]  X,Y,W,H the corresponding screen bounding box
  \param[in] n the screen number (0 to Fl::screen_count() - 1)
  \see void screen_xywh(int &x, int &y, int &w, int &h, int mx, int my) 
*/
void Fl::screen_xywh(int &X, int &Y, int &W, int &H, int n) {
  if (!num_screens) screen_init();

#ifdef WIN32
  if (num_screens > 1 && n >= 0 && n < num_screens) {
    X = screens[n].left;
    Y = screens[n].top;
    W = screens[n].right - screens[n].left;
    H = screens[n].bottom - screens[n].top;
    return;
  }
#elif defined(__APPLE__)
  if (num_screens > 1 && n >= 0 && n < num_screens) {
    X = screens[n].x;
    Y = screens[n].y;
    W = screens[n].width;
    H = screens[n].height;
    return;
  }
#elif HAVE_XINERAMA
  if (num_screens > 1 && n >= 0 && n < num_screens) {
    X = screens[n].x_org;
    Y = screens[n].y_org;
    W = screens[n].width;
    H = screens[n].height;
    return;
  }
#else
  (void)n;
#endif // WIN32

  X = Fl::x();
  Y = Fl::y();
  W = Fl::w();
  H = Fl::h();
}


//
// End of "$Id: screen_xywh.cxx 7351 2010-03-29 10:35:00Z matt $".
//
