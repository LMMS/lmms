//
// "$Id: Fl_visual.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Visual support for the Fast Light Tool Kit (FLTK).
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

// Set the default visual according to passed switches:

#include <config.h>
#include <FL/Fl.H>
#include <FL/x.H>

/** \fn  Fl::visual(int flags)
    Selects a visual so that your graphics are drawn correctly.  This is
    only allowed before you call show() on any windows.  This does nothing
    if the default visual satisfies the capabilities, or if no visual
    satisfies the capabilities, or on systems that don't have such
    brain-dead notions.
    
    <P>Only the following combinations do anything useful:
    
    <UL>
    <LI>Fl::visual(FL_RGB)
    <BR>Full/true color (if there are several depths FLTK chooses  the
    largest).  Do this if you use fl_draw_image
    for much better (non-dithered)  output.
    <BR>&nbsp; </LI>
    <LI>Fl::visual(FL_RGB8)
    <BR>Full color with at least 24 bits of color. FL_RGB will
    always  pick this if available, but if not it will happily return a
    less-than-24 bit deep visual.  This call fails if 24 bits are not
    available.
    <BR>&nbsp; </LI>
    <LI>Fl::visual(FL_DOUBLE|FL_INDEX)
    <BR>Hardware double buffering.  Call this if you are going to use 
    Fl_Double_Window.
    <BR>&nbsp; </LI>
    <LI>Fl::visual(FL_DOUBLE|FL_RGB)</LI>
    <LI>Fl::visual(FL_DOUBLE|FL_RGB8)
    <BR>Hardware double buffering and full color.
    </UL>
    
    <P>This returns true if the system has the capabilities by default or
    FLTK suceeded in turing them on.  Your program will still work even if
    this returns false (it just won't look as good).
*/
#ifdef WIN32
int Fl::visual(int flags) {
  fl_GetDC(0);
  if (flags & FL_DOUBLE) return 0;
  if (!(flags & FL_INDEX) &&
    GetDeviceCaps(fl_gc,BITSPIXEL) <= 8) return 0;
  if ((flags & FL_RGB8) && GetDeviceCaps(fl_gc,BITSPIXEL)<24) return 0;
  return 1;
}
#elif defined(__APPLE__)

// \todo Mac : need to implement Visual flags
int Fl::visual(int flags) {
  (void)flags;
  return 1;
}

#else

#if USE_XDBE
#include <X11/extensions/Xdbe.h>
#endif

static int test_visual(XVisualInfo& v, int flags) {
  if (v.screen != fl_screen) return 0;
#if USE_COLORMAP
  if (!(flags & FL_INDEX)) {
    if (v.c_class != StaticColor && v.c_class != TrueColor) return 0;
    if (v.depth <= 8) return 0; // fltk will work better in colormap mode
  }
  if (flags & FL_RGB8) {
    if (v.depth < 24) return 0;
  }
  // for now, fltk does not like colormaps of more than 8 bits:
  if ((v.c_class&1) && v.depth > 8) return 0;
#else
  // simpler if we can't use colormapped visuals at all:
  if (v.c_class != StaticColor && v.c_class != TrueColor) return 0;
#endif
#if USE_XDBE
  if (flags & FL_DOUBLE) {
    static XdbeScreenVisualInfo *xdbejunk;
    if (!xdbejunk) {
      int event_base, error_base;
      if (!XdbeQueryExtension(fl_display, &event_base, &error_base)) return 0;
      Drawable root = RootWindow(fl_display,fl_screen);
      int numscreens = 1;
      xdbejunk = XdbeGetVisualInfo(fl_display,&root,&numscreens);
      if (!xdbejunk) return 0;
    }
    for (int j = 0; ; j++) {
      if (j >= xdbejunk->count) return 0;
      if (xdbejunk->visinfo[j].visual == v.visualid) break;
    }
  }
#endif
  return 1;
}

int Fl::visual(int flags) {
#if USE_XDBE == 0
  if (flags & FL_DOUBLE) return 0;
#endif
  fl_open_display();
  // always use default if possible:
  if (test_visual(*fl_visual, flags)) return 1;
  // get all the visuals:
  XVisualInfo vTemplate;
  int num;
  XVisualInfo *visualList = XGetVisualInfo(fl_display, 0, &vTemplate, &num);
  // find all matches, use the one with greatest depth:
  XVisualInfo *found = 0;
  for (int i=0; i<num; i++) if (test_visual(visualList[i], flags)) {
    if (!found || found->depth < visualList[i].depth)
      found = &visualList[i];
  }
  if (!found) {XFree((void*)visualList); return 0;}
  fl_visual = found;
  fl_colormap = XCreateColormap(fl_display, RootWindow(fl_display,fl_screen),
				fl_visual->visual, AllocNone);
  return 1;
}

#endif

//
// End of "$Id: Fl_visual.cxx 7903 2010-11-28 21:06:39Z matt $".
//
