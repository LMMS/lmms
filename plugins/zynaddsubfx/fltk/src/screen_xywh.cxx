//
// "$Id: screen_xywh.cxx 8783 2011-06-06 09:37:21Z AlbrechtS $"
//
// Screen/monitor bounding box API for the Fast Light Tool Kit (FLTK).
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


#include <FL/Fl.H>
#include <FL/x.H>
#include <config.h>


// Number of screens returned by multi monitor aware API; -1 before init
static int num_screens = -1;

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
static float dpi[16][2];

static BOOL CALLBACK screen_cb(HMONITOR mon, HDC, LPRECT r, LPARAM) {
  if (num_screens >= 16) return TRUE;

  MONITORINFOEX mi;
  mi.cbSize = sizeof(mi);

//  GetMonitorInfo(mon, &mi);
//  (but we use our self-aquired function pointer instead)
  if (fl_gmi(mon, &mi)) {
    screens[num_screens] = mi.rcMonitor;
    
    // find the pixel size
    if (mi.cbSize == sizeof(mi)) {
      HDC screen = CreateDC(mi.szDevice, NULL, NULL, NULL);
      if (screen) {
        dpi[num_screens][0] = (float)GetDeviceCaps(screen, LOGPIXELSX);
        dpi[num_screens][1] = (float)GetDeviceCaps(screen, LOGPIXELSY);
      }
      ReleaseDC(0L, screen);
    }
    
    num_screens ++;
  }
  return TRUE;
}

static void screen_init() {
  num_screens = 0;
  // Since not all versions of Windows include multiple monitor support,
  // we do a run-time check for the required functions...
  HMODULE hMod = GetModuleHandle("USER32.DLL");

  if (hMod) {
    // check that EnumDisplayMonitors is available
    fl_edm_func fl_edm = (fl_edm_func)GetProcAddress(hMod, "EnumDisplayMonitors");

    if (fl_edm) {
      // We do have EnumDisplayMonitors, so lets find out how many monitors...
      num_screens = GetSystemMetrics(SM_CMONITORS);

//      if (num_screens > 1) {
        // If there is more than 1 monitor, enumerate them...
        fl_gmi = (fl_gmi_func)GetProcAddress(hMod, "GetMonitorInfoA");

        if (fl_gmi) {
          // We have GetMonitorInfoA, enumerate all the screens...
//        EnumDisplayMonitors(0,0,screen_cb,0);
//        (but we use our self-aquired function pointer instead)
          fl_edm(0, 0, screen_cb, 0);
          return;
        }
//      }
    }
  }

  // If we get here, assume we have 1 monitor...
  num_screens = 1;
  screens[0].top      = 0;
  screens[0].left      = 0;
  screens[0].right  = GetSystemMetrics(SM_CXSCREEN);
  screens[0].bottom = GetSystemMetrics(SM_CYSCREEN);
}
#elif defined(__APPLE__)
static XRectangle screens[16];
static float dpi_h[16];
static float dpi_v[16];

static void screen_init() {
  CGDirectDisplayID displays[16];
  CGDisplayCount count, i;
  CGRect r;
  CGGetActiveDisplayList(16, displays, &count);
  for( i = 0; i < count; i++) {
    r = CGDisplayBounds(displays[i]);
    screens[i].x      = int(r.origin.x);
    screens[i].y      = int(r.origin.y);
    screens[i].width  = int(r.size.width);
    screens[i].height = int(r.size.height);
    CGSize s = CGDisplayScreenSize(displays[i]);
    dpi_h[i] = screens[i].width / (s.width/25.4);
    dpi_v[i] = screens[i].height / (s.height/25.4);
  }
  num_screens = count;
}
#elif HAVE_XINERAMA
#  include <X11/extensions/Xinerama.h>

// Screen data...
static XineramaScreenInfo *screens;
static float dpi[16][2];

static void screen_init() {
  if (!fl_display) fl_open_display();

  if (XineramaIsActive(fl_display)) {
    screens = XineramaQueryScreens(fl_display, &num_screens);
    int i;
    // Xlib and Xinerama may disagree on the screen count. Sigh...
    // Use the minimum of the reported counts.
    // Use the previous screen's info for non-existent ones.
    int sc = ScreenCount(fl_display); // Xlib screen count
    for (i=0; i<num_screens; i++) {
      int mm = (i < sc) ? DisplayWidthMM(fl_display, i) : 0;
      dpi[i][0] = mm ? screens[i].width*25.4f/mm : (i > 0) ? dpi[i-1][0] : 0.0f;
      mm = (i < sc) ? DisplayHeightMM(fl_display, i) : 0;
      dpi[i][1] = mm ? screens[i].height*25.4f/mm : (i > 0) ? dpi[i-1][1] : 0.0f;
    }
  } else { // ! XineramaIsActive()
    num_screens = 1;
    int mm = DisplayWidthMM(fl_display, fl_screen);
    dpi[0][0] = mm ? Fl::w()*25.4f/mm : 0.0f;
    mm = DisplayHeightMM(fl_display, fl_screen);
    dpi[0][1] = mm ? Fl::h()*25.4f/mm : dpi[0][0];
  }
}
#else
static float dpi[2];
static void screen_init() {
  num_screens = 1;
  if (!fl_display) fl_open_display();
  int mm = DisplayWidthMM(fl_display, fl_screen);
  dpi[0] = mm ? Fl::w()*25.4f/mm : 0.0f;
  mm = DisplayHeightMM(fl_display, fl_screen);
  dpi[1] = mm ? Fl::h()*25.4f/mm : dpi[0];  
}
#endif // WIN32


/**
  Gets the number of available screens.
*/
int Fl::screen_count() {
  if (num_screens < 0) screen_init();

  return num_screens ? num_screens : 1;
}

/**
  Gets the bounding box of a screen 
  that contains the specified screen position \p mx, \p my
  \param[out]  X,Y,W,H the corresponding screen bounding box
  \param[in] mx, my the absolute screen position
*/
void Fl::screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my) {
  int screen = 0;
  int i;

  if (num_screens < 0) screen_init();

  for (i = 0; i < num_screens; i ++) {
    int sx, sy, sw, sh;
    Fl::screen_xywh(sx, sy, sw, sh, i);
    if ((mx >= sx) && (mx < (sx+sw)) && (my >= sy) && (my < (sy+sh))) {
      screen = i;
      break;
    }
  }

  screen_xywh(X, Y, W, H, screen);
}

/**
  Gets the screen bounding rect for the given screen. 
  \param[out]  X,Y,W,H the corresponding screen bounding box
  \param[in] n the screen number (0 to Fl::screen_count() - 1)
  \see void screen_xywh(int &x, int &y, int &w, int &h, int mx, int my) 
*/
void Fl::screen_xywh(int &X, int &Y, int &W, int &H, int n) {
  if (num_screens < 0) screen_init();

  if ((n < 0) || (n >= num_screens))
    n = 0;

#ifdef WIN32
  if (num_screens > 0) {
    X = screens[n].left;
    Y = screens[n].top;
    W = screens[n].right - screens[n].left;
    H = screens[n].bottom - screens[n].top;
  } else {
    /* Fallback if something is broken... */
    X = 0;
    Y = 0;
    W = GetSystemMetrics(SM_CXSCREEN);
    H = GetSystemMetrics(SM_CYSCREEN);
  }
#elif defined(__APPLE__)
  if (num_screens > 0) {
    X = screens[n].x;
    Y = screens[n].y;
    W = screens[n].width;
    H = screens[n].height;
  } else {
    /* Fallback if something is broken... */
    X = Fl::x();
    Y = Fl::y();
    W = Fl::w();
    H = Fl::h();
  }
#else
#if HAVE_XINERAMA
  if (num_screens > 0 && screens) {
    X = screens[n].x_org;
    Y = screens[n].y_org;
    W = screens[n].width;
    H = screens[n].height;
  } else
#endif // HAVE_XINERAMA
  {
    /* Fallback if something is broken (or no Xinerama)... */
    X = 0;
    Y = 0;
    W = DisplayWidth(fl_display, fl_screen);
    H = DisplayHeight(fl_display, fl_screen);
  }
#endif // WIN32
}

static inline float fl_intersection(int x1, int y1, int w1, int h1,
                        int x2, int y2, int w2, int h2) {
  if(x1+w1 < x2 || x2+w2 < x1 || y1+h1 < y2 || y2+h2 < y1)
    return 0.;
  int int_left = x1 > x2 ? x1 : x2;
  int int_right = x1+w1 > x2+w2 ? x2+w2 : x1+w1;
  int int_top = y1 > y2 ? y1 : y2;
  int int_bottom = y1+h1 > y2+h2 ? y2+h2 : y1+h1;
  return (float)(int_right - int_left) * (int_bottom - int_top);
}

/**
  Gets the screen bounding rect for the screen
  which intersects the most with the rectangle
  defined by \p mx, \p my, \p mw, \p mh.
  \param[out]  X,Y,W,H the corresponding screen bounding box
  \param[in] mx, my, mw, mh the rectangle to search for intersection with
  \see void screen_xywh(int &X, int &Y, int &W, int &H, int n)
  */
void Fl::screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my, int mw, int mh) {
  int best_screen = 0;
  float best_intersection = 0.;
  for(int i = 0; i < Fl::screen_count(); i++) {
    int sx, sy, sw, sh;
    Fl::screen_xywh(sx, sy, sw, sh, i);
    float sintersection = fl_intersection(mx, my, mw, mh, sx, sy, sw, sh);
    if(sintersection > best_intersection) {
      best_screen = i;
      best_intersection = sintersection;
    }
  }
  screen_xywh(X, Y, W, H, best_screen);
}
  


/**
 Gets the screen resolution in dots-per-inch for the given screen. 
 \param[out]  h, v  horizontal and vertical resolution
 \param[in]   n     the screen number (0 to Fl::screen_count() - 1)
 \see void screen_xywh(int &x, int &y, int &w, int &h, int mx, int my) 
 */
void Fl::screen_dpi(float &h, float &v, int n)
{
  if (num_screens < 0) screen_init();
  h = v = 0.0f;
  
#ifdef WIN32
  if (n >= 0 && n < num_screens) {
    h = float(dpi[n][0]);
    v = float(dpi[n][1]);
  }
#elif defined(__APPLE__)
  if (n >= 0 && n < num_screens) {
    h = dpi_h[n];
    v = dpi_v[n];
  }
#elif HAVE_XINERAMA
  if (n >= 0 && n < num_screens) {
    h = dpi[n][0];
    v = dpi[n][1];
  }
#else
  if (n >= 0 && n < num_screens) {
    h = dpi[0];
    v = dpi[1];
  }
#endif // WIN32
}



//
// End of "$Id: screen_xywh.cxx 8783 2011-06-06 09:37:21Z AlbrechtS $".
//
