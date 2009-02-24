//
// "$Id: fl_scroll_area.cxx 5714 2007-02-25 00:00:49Z matt $"
//
// Scrolling routines for the Fast Light Tool Kit (FLTK).
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

// Drawing function to move the contents of a rectangle.  This is passed
// a "callback" which is called to draw rectangular areas that are moved
// into the drawing area.

#include <config.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>

// scroll a rectangle and redraw the newly exposed portions:
void fl_scroll(int X, int Y, int W, int H, int dx, int dy,
	       void (*draw_area)(void*, int,int,int,int), void* data)
{
  if (!dx && !dy) return;
  if (dx <= -W || dx >= W || dy <= -H || dy >= H) {
    // no intersection of old an new scroll
    draw_area(data,X,Y,W,H);
    return;
  }
  int src_x, src_w, dest_x, clip_x, clip_w;
  if (dx > 0) {
    src_x = X;
    dest_x = X+dx;
    src_w = W-dx;
    clip_x = X;
    clip_w = dx;
  } else {
    src_x = X-dx;
    dest_x = X;
    src_w = W+dx;
    clip_x = X+src_w;
    clip_w = W-src_w;
  }
  int src_y, src_h, dest_y, clip_y, clip_h;
  if (dy > 0) {
    src_y = Y;
    dest_y = Y+dy;
    src_h = H-dy;
    clip_y = Y;
    clip_h = dy;
  } else {
    src_y = Y-dy;
    dest_y = Y;
    src_h = H+dy;
    clip_y = Y+src_h;
    clip_h = H-src_h;
  }
#ifdef WIN32
  typedef int (WINAPI* fl_GetRandomRgn_func)(HDC, HRGN, INT);
  static fl_GetRandomRgn_func fl_GetRandomRgn = 0L;
  static char first_time = 1;

  // We will have to do some Region magic now, so let's see if the 
  // required function is available (and it should be staring w/Win95)
  if (first_time) {
    HMODULE hMod = GetModuleHandle("GDI32.DLL");
    if (hMod) {
      fl_GetRandomRgn = (fl_GetRandomRgn_func)GetProcAddress(hMod, "GetRandomRgn");
    }
    first_time = 0;
  }

  // Now check if the source scrolling area is fully visible.
  // If it is, we will do a quick scroll and just update the 
  // newly exposed area. If it is not, we go the safe route and 
  // re-render the full area instead.
  // Note 1: we could go and find the areas that are actually
  // obscured and recursively call fl_scroll for the newly found
  // rectangles. However, this practice would rely on the 
  // elements of the undocumented Rgn structure.
  // Note 2: although this method should take care of most 
  // multi-screen solutions, it will not solve issues scrolling
  // from a different resolution screen onto another.
  // Note 3: this has been tested with image maps, too.
  if (fl_GetRandomRgn) {
    // get the DC region minus all overlapping windows
    HRGN sys_rgn = CreateRectRgn(0, 0, 0, 0);
    fl_GetRandomRgn(fl_gc, sys_rgn, 4);
    // now get the source scrolling rectangle 
    HRGN src_rgn = CreateRectRgn(src_x, src_y, src_x+src_w, src_y+src_h);
    POINT offset = { 0, 0 };
    if (GetDCOrgEx(fl_gc, &offset)) {
      OffsetRgn(src_rgn, offset.x, offset.y);
    }
    // see if all source pixels are available in the system region
    // Note: we could be a bit more merciful and subtract the 
    // scroll destination region as well.
    HRGN dst_rgn = CreateRectRgn(0, 0, 0, 0);
    int r = CombineRgn(dst_rgn, src_rgn, sys_rgn, RGN_DIFF);
    DeleteObject(dst_rgn);
    DeleteObject(src_rgn);
    DeleteObject(sys_rgn);
    if (r!=NULLREGION) {
      draw_area(data,X,Y,W,H);
      return;
    }
  }

  // Great, we can do an accelerated scroll insteasd of re-rendering
  BitBlt(fl_gc, dest_x, dest_y, src_w, src_h, fl_gc, src_x, src_y,SRCCOPY);

#elif defined(__APPLE_QD__)
  Rect src = { src_y, src_x, src_y+src_h, src_x+src_w };
  Rect dst = { dest_y, dest_x, dest_y+src_h, dest_x+src_w };
  static RGBColor bg = { 0xffff, 0xffff, 0xffff }; RGBBackColor( &bg );
  static RGBColor fg = { 0x0000, 0x0000, 0x0000 }; RGBForeColor( &fg );
  CopyBits( GetPortBitMapForCopyBits( GetWindowPort(fl_window) ),
            GetPortBitMapForCopyBits( GetWindowPort(fl_window) ), &src, &dst, srcCopy, 0L);
#elif defined(__APPLE_QUARTZ__)
  // warning: there does not seem to be an equivalent to this function in Quartz
  // ScrollWindowRect is a QuickDraw function and won't work here.
  // Since on OS X all windows are fully double buffered, we need not
  // worry about offscreen or obscured areas
  Rect src = { src_y, src_x, src_y+src_h, src_x+src_w };
  Rect dst = { dest_y, dest_x, dest_y+src_h, dest_x+src_w };
  static RGBColor bg = { 0xffff, 0xffff, 0xffff }; RGBBackColor( &bg );
  static RGBColor fg = { 0x0000, 0x0000, 0x0000 }; RGBForeColor( &fg );
  CopyBits( GetPortBitMapForCopyBits( GetWindowPort(fl_window) ),
            GetPortBitMapForCopyBits( GetWindowPort(fl_window) ), &src, &dst, srcCopy, 0L);
#else
  XCopyArea(fl_display, fl_window, fl_window, fl_gc,
	    src_x, src_y, src_w, src_h, dest_x, dest_y);
  // we have to sync the display and get the GraphicsExpose events! (sigh)
  for (;;) {
    XEvent e; XWindowEvent(fl_display, fl_window, ExposureMask, &e);
    if (e.type == NoExpose) break;
    // otherwise assumme it is a GraphicsExpose event:
    draw_area(data, e.xexpose.x, e.xexpose.y,
	      e.xexpose.width, e.xexpose.height);
    if (!e.xgraphicsexpose.count) break;
  }
#endif
  if (dx) draw_area(data, clip_x, dest_y, clip_w, src_h);
  if (dy) draw_area(data, X, clip_y, W, clip_h);
}

//
// End of "$Id: fl_scroll_area.cxx 5714 2007-02-25 00:00:49Z matt $".
//
