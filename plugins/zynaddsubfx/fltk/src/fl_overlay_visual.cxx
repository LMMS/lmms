//
// "$Id: fl_overlay_visual.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// X overlay support for the Fast Light Tool Kit (FLTK).
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

// Return an overlay visual, if any.  Also allocate a colormap and
// record the depth for fl_color() to use.
// Another disgusting X interface, based on code extracted and
// purified with great difficulty from XLayerUtil.cxx:

#include <config.h>
#if HAVE_OVERLAY
#include <FL/Fl.H>
#include <FL/x.H>

// SERVER_OVERLAY_VISUALS property element:
struct OverlayInfo {
  long overlay_visual;
  long transparent_type;
  long value;
  long layer;
};

extern Colormap fl_overlay_colormap;
extern XVisualInfo* fl_overlay_visual;
extern ulong fl_transparent_pixel;

XVisualInfo *fl_find_overlay_visual() {
  static char beenhere;
  if (beenhere) return fl_overlay_visual;
  beenhere = 1;

  fl_open_display();
  Atom overlayVisualsAtom =
    XInternAtom(fl_display,"SERVER_OVERLAY_VISUALS",1);
  if (!overlayVisualsAtom) return 0;
  OverlayInfo *overlayInfo;
  ulong sizeData, bytesLeft;
  Atom actualType;
  int actualFormat;
  if (XGetWindowProperty(fl_display, RootWindow(fl_display, fl_screen),
			 overlayVisualsAtom, 0L, 10000L, False,
			 overlayVisualsAtom, &actualType, &actualFormat,
			 &sizeData, &bytesLeft,
			 (unsigned char **) &overlayInfo)) return 0;

  if (actualType == overlayVisualsAtom && actualFormat == 32) {
    int n = int(sizeData/4);
    XVisualInfo* v = 0;
    // find the greatest depth that has a transparent pixel:
    for (int i = 0; i < n; i++) {
      if (overlayInfo[i].transparent_type != 1) continue;
      if (overlayInfo[i].layer <= 0) continue;
      XVisualInfo templt;
      templt.visualid = overlayInfo[i].overlay_visual;
      int num;
      XVisualInfo *v1=XGetVisualInfo(fl_display, VisualIDMask, &templt, &num);
      if (v1->screen == fl_screen && v1->c_class == PseudoColor
	  && (!v || v1->depth > v->depth && v1->depth <= 8)) {
	if (v) XFree((char*)v);
	v = v1;
	fl_transparent_pixel = overlayInfo[i].value;
      } else {
	XFree((char*)v1);
      }
    }
    if (v) {
      fl_overlay_visual = v;
      fl_overlay_colormap = 
	XCreateColormap(fl_display, RootWindow(fl_display, fl_screen),
			v->visual, AllocNone);
    }
  }
  XFree((char*)overlayInfo);
  //printf("overlay visual %ld selected\n", fl_overlay_visual->visualid);
  return fl_overlay_visual;
}

#endif

//
// End of "$Id: fl_overlay_visual.cxx 5190 2006-06-09 16:16:34Z mike $".
//
