//
// "$Id: fl_rect.cxx 8630 2011-05-01 12:45:29Z AlbrechtS $"
//
// Rectangle drawing routines for the Fast Light Tool Kit (FLTK).
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

/**
  \file fl_rect.cxx
  \brief Drawing and clipping routines for rectangles.
*/

// These routines from fl_draw.H are used by the standard boxtypes
// and thus are always linked into an fltk program.
// Also all fl_clip routines, since they are always linked in so
// that minimal update works.

#include <config.h>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Printer.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

// fl_line_width_ must contain the absolute value of the current
// line width to be used for X11 clipping (see below).
// This is defined in src/fl_line_style.cxx
extern int fl_line_width_;

#ifdef __APPLE_QUARTZ__
extern float fl_quartz_line_width_;
#define USINGQUARTZPRINTER  (Fl_Surface_Device::surface()->class_name() == Fl_Printer::class_id)
#endif

#ifdef USE_X11

#ifndef SHRT_MAX
#define SHRT_MAX (32767)
#endif

/*
  We need to check some coordinates for areas for clipping before we
  use X functions, because X can't handle coordinates outside the 16-bit
  range. Since all windows use relative coordinates > 0, we do also
  check for negative values. X11 only, see also STR #2304.
  
  Note that this is only necessary for large objects, where only a
  part of the object is visible. The draw() functions (e.g. box
  drawing) must be clipped correctly. This is usually only a matter
  for large container widgets. The individual child widgets will be
  clipped completely.

  We define the usable X coordinate space as [ -LW : SHRT_MAX - LW ]
  where LW = current line width for drawing. This is done so that
  horizontal and vertical line drawing works correctly, even in real
  border cases, e.g. drawing a rectangle slightly outside the top left
  window corner, but with a line width so that a part of the line should
  be visible (in this case 2 of 5 pixels):

    fl_line_style (FL_SOLID,5);	// line width = 5
    fl_rect (-1,-1,100,100);	// top/left: 2 pixels visible
  
  In this example case, no clipping would be done, because X can
  handle it and clip unneeded pixels.
  
  Note that we must also take care of the case where fl_line_width_
  is zero (maybe unitialized). If this is the case, we assume a line
  width of 1.

  Todo: Arbitrary line drawings (e.g. polygons) and clip regions
  are not yet done.

  Note:

  We could use max. screen coordinates instead of SHRT_MAX, but that
  would need more work and would probably be slower. We assume that
  all window coordinates are >= 0 and that no window extends up to
  32767 - LW (where LW = current line width). Thus it is safe to clip
  all coordinates to this range before calling X functions. If this
  is not true, then clip_to_short() and clip_x() must be redefined.

  It would be somewhat easier if we had fl_clip_w and fl_clip_h, as
  defined in FLTK 2.0 (for the upper clipping bounds)...
*/

/*
  clip_to_short() returns 1, if the area is invisible (clipped),
  because ...

    (a) w or h are <= 0		i.e. nothing is visible
    (b) x+w or y+h are < kmin	i.e. left of or above visible area
    (c) x or y are > kmax	i.e. right of or below visible area

  kmin and kmax are the minimal and maximal X coordinate values,
  as defined above. In this case x, y, w, and h are not changed.

  It returns 0, if the area is potentially visible and X can handle
  clipping. x, y, w, and h may have been adjusted to fit into the
  X coordinate space.

  Use this for clipping rectangles, as used in fl_rect() and
  fl_rectf().
*/

static int clip_to_short(int &x, int &y, int &w, int &h) {

  int lw = (fl_line_width_ > 0) ? fl_line_width_ : 1;
  int kmin = -lw;
  int kmax = SHRT_MAX - lw;

  if (w <= 0 || h <= 0) return 1;		// (a)
  if (x+w < kmin || y+h < kmin) return 1;	// (b)
  if (x > kmax || y > kmax) return 1;		// (c)

  if (x < kmin) { w -= (kmin-x); x = kmin; }
  if (y < kmin) { h -= (kmin-y); y = kmin; }
  if (x+w > kmax) w = kmax - x;
  if (y+h > kmax) h = kmax - y;

  return 0;
}

/*
  clip_x() returns a coordinate value clipped to the 16-bit coordinate
  space (see above). This can be used to draw horizontal and vertical
  lines that can be handled by X11. Each single coordinate value can
  be clipped individually, and the result can be used directly, e.g.
  in fl_xyline() and fl_yxline(). Note that this can't be used for
  arbitrary lines (not horizontal or vertical).
*/
static int clip_x (int x) {

  int lw = (fl_line_width_ > 0) ? fl_line_width_ : 1;
  int kmin = -lw;
  int kmax = SHRT_MAX - lw;

  if (x < kmin)
    x = kmin;
  else if (x > kmax)
    x = kmax;
  return x;
}

#endif	// USE_X11


void Fl_Graphics_Driver::rect(int x, int y, int w, int h) {

  if (w<=0 || h<=0) return;
#if defined(USE_X11)
  if (!clip_to_short(x, y, w, h))
    XDrawRectangle(fl_display, fl_window, fl_gc, x, y, w-1, h-1);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x+w-1, y);
  LineTo(fl_gc, x+w-1, y+h-1);
  LineTo(fl_gc, x, y+h-1);
  LineTo(fl_gc, x, y);
#elif defined(__APPLE_QUARTZ__)
  if ( (!USINGQUARTZPRINTER) && fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGRect rect = CGRectMake(x, y, w-1, h-1);
  CGContextStrokeRect(fl_gc, rect);
  if ( (!USINGQUARTZPRINTER) && fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::rectf(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
#if defined(USE_X11)
  if (!clip_to_short(x, y, w, h))
    XFillRectangle(fl_display, fl_window, fl_gc, x, y, w, h);
#elif defined(WIN32)
  RECT rect;
  rect.left = x; rect.top = y;  
  rect.right = x + w; rect.bottom = y + h;
  FillRect(fl_gc, &rect, fl_brush());
#elif defined(__APPLE_QUARTZ__)
  CGFloat delta_size =  0.9;
  CGFloat delta_ori = 0;
  if (USINGQUARTZPRINTER) {
    delta_size = 0;
    delta_ori = 0.5;
    }
  CGRect  rect = CGRectMake(x - delta_ori, y - delta_ori, w - delta_size , h - delta_size);
  CGContextFillRect(fl_gc, rect);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::xyline(int x, int y, int x1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, clip_x(x), clip_x(y), clip_x(x1), clip_x(y));
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); LineTo(fl_gc, x1+1, y);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::xyline(int x, int y, int x1, int y2) {
#if defined (USE_X11)
  XPoint p[3];
  p[0].x = clip_x(x);  p[0].y = p[1].y = clip_x(y);
  p[1].x = p[2].x = clip_x(x1); p[2].y = clip_x(y2);
  XDrawLines(fl_display, fl_window, fl_gc, p, 3, 0);
#elif defined(WIN32)
  if (y2 < y) y2--;
  else y2++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y);
  LineTo(fl_gc, x1, y2);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextAddLineToPoint(fl_gc, x1, y2);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
#error unsupported platform
#endif
}

void Fl_Graphics_Driver::xyline(int x, int y, int x1, int y2, int x3) {
#if defined(USE_X11)
  XPoint p[4];
  p[0].x = clip_x(x);  p[0].y = p[1].y = clip_x(y);
  p[1].x = p[2].x = clip_x(x1); p[2].y = p[3].y = clip_x(y2);
  p[3].x = clip_x(x3);
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  if(x3 < x1) x3--;
  else x3++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y);
  LineTo(fl_gc, x1, y2);
  LineTo(fl_gc, x3, y2);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextAddLineToPoint(fl_gc, x1, y2);
  CGContextAddLineToPoint(fl_gc, x3, y2);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::yxline(int x, int y, int y1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, clip_x(x), clip_x(y), clip_x(x), clip_x(y1));
#elif defined(WIN32)
  if (y1 < y) y1--;
  else y1++;
  MoveToEx(fl_gc, x, y, 0L); LineTo(fl_gc, x, y1);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::yxline(int x, int y, int y1, int x2) {
#if defined(USE_X11)
  XPoint p[3];
  p[0].x = p[1].x = clip_x(x);  p[0].y = clip_x(y);
  p[1].y = p[2].y = clip_x(y1); p[2].x = clip_x(x2);
  XDrawLines(fl_display, fl_window, fl_gc, p, 3, 0);
#elif defined(WIN32)
  if (x2 > x) x2++;
  else x2--;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x, y1);
  LineTo(fl_gc, x2, y1);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextAddLineToPoint(fl_gc, x2, y1);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::yxline(int x, int y, int y1, int x2, int y3) {
#if defined(USE_X11)
  XPoint p[4];
  p[0].x = p[1].x = clip_x(x);  p[0].y = clip_x(y);
  p[1].y = p[2].y = clip_x(y1); p[2].x = p[3].x = clip_x(x2);
  p[3].y = clip_x(y3);
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  if(y3<y1) y3--;
  else y3++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x, y1);
  LineTo(fl_gc, x2, y1);
  LineTo(fl_gc, x2, y3);
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextAddLineToPoint(fl_gc, x2, y1);
  CGContextAddLineToPoint(fl_gc, x2, y3);
  CGContextStrokePath(fl_gc);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::line(int x, int y, int x1, int y1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x1, y1);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y1);
  // Draw the last point *again* because the GDI line drawing
  // functions will not draw the last point ("it's a feature!"...)
  SetPixel(fl_gc, x1, y1, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::line(int x, int y, int x1, int y1, int x2, int y2) {
#if defined(USE_X11)
  XPoint p[3];
  p[0].x = x;  p[0].y = y;
  p[1].x = x1; p[1].y = y1;
  p[2].x = x2; p[2].y = y2;
  XDrawLines(fl_display, fl_window, fl_gc, p, 3, 0);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y1);
  LineTo(fl_gc, x2, y2);
  // Draw the last point *again* because the GDI line drawing
  // functions will not draw the last point ("it's a feature!"...)
  SetPixel(fl_gc, x2, y2, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::loop(int x, int y, int x1, int y1, int x2, int y2) {
#if defined(USE_X11)
  XPoint p[4];
  p[0].x = x;  p[0].y = y;
  p[1].x = x1; p[1].y = y1;
  p[2].x = x2; p[2].y = y2;
  p[3].x = x;  p[3].y = y;
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y1);
  LineTo(fl_gc, x2, y2);
  LineTo(fl_gc, x, y);
#elif defined(__APPLE_QUARTZ__)
  CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextClosePath(fl_gc);
  CGContextStrokePath(fl_gc);
  CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::loop(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
#if defined(USE_X11)
  XPoint p[5];
  p[0].x = x;  p[0].y = y;
  p[1].x = x1; p[1].y = y1;
  p[2].x = x2; p[2].y = y2;
  p[3].x = x3; p[3].y = y3;
  p[4].x = x;  p[4].y = y;
  XDrawLines(fl_display, fl_window, fl_gc, p, 5, 0);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y1);
  LineTo(fl_gc, x2, y2);
  LineTo(fl_gc, x3, y3);
  LineTo(fl_gc, x, y);
#elif defined(__APPLE_QUARTZ__)
  CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextAddLineToPoint(fl_gc, x3, y3);
  CGContextClosePath(fl_gc);
  CGContextStrokePath(fl_gc);
  CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::polygon(int x, int y, int x1, int y1, int x2, int y2) {
  XPoint p[4];
  p[0].x = x;  p[0].y = y;
  p[1].x = x1; p[1].y = y1;
  p[2].x = x2; p[2].y = y2;
#if defined (USE_X11)
  p[3].x = x;  p[3].y = y;
  XFillPolygon(fl_display, fl_window, fl_gc, p, 3, Convex, 0);
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  SelectObject(fl_gc, fl_brush());
  Polygon(fl_gc, p, 3);
#elif defined(__APPLE_QUARTZ__)
  CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
  CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::polygon(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
  XPoint p[5];
  p[0].x = x;  p[0].y = y;
  p[1].x = x1; p[1].y = y1;
  p[2].x = x2; p[2].y = y2;
  p[3].x = x3; p[3].y = y3;
#if defined(USE_X11)
  p[4].x = x;  p[4].y = y;
  XFillPolygon(fl_display, fl_window, fl_gc, p, 4, Convex, 0);
  XDrawLines(fl_display, fl_window, fl_gc, p, 5, 0);
#elif defined(WIN32)
  SelectObject(fl_gc, fl_brush());
  Polygon(fl_gc, p, 4);
#elif defined(__APPLE_QUARTZ__)
  CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextAddLineToPoint(fl_gc, x3, y3);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
  CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::point(int x, int y) {
#if defined(USE_X11)
  XDrawPoint(fl_display, fl_window, fl_gc, clip_x(x), clip_x(y));
#elif defined(WIN32)
  SetPixel(fl_gc, x, y, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  CGContextFillRect(fl_gc, CGRectMake(x - 0.5, y - 0.5, 1, 1) );
#else
# error unsupported platform
#endif
}

////////////////////////////////////////////////////////////////

#if !defined(WIN32) && !defined(__APPLE__)
// Missing X call: (is this the fastest way to init a 1-rectangle region?)
// MSWindows equivalent exists, implemented inline in win32.H
Fl_Region XRectangleRegion(int x, int y, int w, int h) {
  XRectangle R;
  clip_to_short(x, y, w, h);
  R.x = x; R.y = y; R.width = w; R.height = h;
  Fl_Region r = XCreateRegion();
  XUnionRectWithRegion(&R, r, r);
  return r;
}
#endif

void Fl_Graphics_Driver::restore_clip() {
  fl_clip_state_number++;
  Fl_Region r = rstack[rstackptr];
#if defined(USE_X11)
  if (r) XSetRegion(fl_display, fl_gc, r);
  else XSetClipMask(fl_display, fl_gc, 0);
#elif defined(WIN32)
  SelectClipRgn(fl_gc, r); //if r is NULL, clip is automatically cleared
#elif defined(__APPLE_QUARTZ__)
  if ( fl_window ) { // clipping for a true window
    Fl_X::q_clear_clipping();
    Fl_X::q_fill_context();//flip coords if bitmap context
    //apply program clip
    if (r) {
      CGContextClipToRects(fl_gc, r->rects, r->count);
    }
  } else if (fl_gc) { // clipping for an offscreen drawing world (CGBitmap)
    Fl_X::q_clear_clipping();
    Fl_X::q_fill_context();
    if (r) {
      CGContextClipToRects(fl_gc, r->rects, r->count);
    }
  }
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::clip_region(Fl_Region r) {
  Fl_Region oldr = rstack[rstackptr];
  if (oldr) XDestroyRegion(oldr);
  rstack[rstackptr] = r;
  fl_restore_clip();
}

Fl_Region Fl_Graphics_Driver::clip_region() {
  return rstack[rstackptr];
}

void Fl_Graphics_Driver::push_clip(int x, int y, int w, int h) {
  Fl_Region r;
  if (w > 0 && h > 0) {
    r = XRectangleRegion(x,y,w,h);
    Fl_Region current = rstack[rstackptr];
    if (current) {
#if defined(USE_X11)
      Fl_Region temp = XCreateRegion();
      XIntersectRegion(current, r, temp);
      XDestroyRegion(r);
      r = temp;
#elif defined(WIN32)
      CombineRgn(r,r,current,RGN_AND);
#elif defined(__APPLE_QUARTZ__)
      XDestroyRegion(r);
      r = Fl_X::intersect_region_and_rect(current, x,y,w,h);
#else
# error unsupported platform
#endif
    }
  } else { // make empty clip region:
#if defined(USE_X11)
    r = XCreateRegion();
#elif defined(WIN32)
    r = CreateRectRgn(0,0,0,0);
#elif defined(__APPLE_QUARTZ__)
    r = XRectangleRegion(0,0,0,0);
#else
# error unsupported platform
#endif
  }
  if (rstackptr < region_stack_max) rstack[++rstackptr] = r;
  else Fl::warning("fl_push_clip: clip stack overflow!\n");
  fl_restore_clip();
}

// make there be no clip (used by fl_begin_offscreen() only!)
void Fl_Graphics_Driver::push_no_clip() {
  if (rstackptr < region_stack_max) rstack[++rstackptr] = 0;
  else Fl::warning("fl_push_no_clip: clip stack overflow!\n");
  fl_restore_clip();
}

// pop back to previous clip:
void Fl_Graphics_Driver::pop_clip() {
  if (rstackptr > 0) {
    Fl_Region oldr = rstack[rstackptr--];
    if (oldr) XDestroyRegion(oldr);
  } else Fl::warning("fl_pop_clip: clip stack underflow!\n");
  fl_restore_clip();
}

int Fl_Graphics_Driver::not_clipped(int x, int y, int w, int h) {
  if (x+w <= 0 || y+h <= 0) return 0;
  Fl_Region r = rstack[rstackptr];
  if (!r) return 1;
#if defined (USE_X11)
  // get rid of coordinates outside the 16-bit range the X calls take.
  if (clip_to_short(x,y,w,h)) return 0;	// clipped
  return XRectInRegion(r, x, y, w, h);
#elif defined(WIN32)
  RECT rect;
  if (Fl_Surface_Device::surface()->class_name() == Fl_Printer::class_id) { // in case of print context, convert coords from logical to device
    POINT pt[2] = { {x, y}, {x + w, y + h} };
    LPtoDP(fl_gc, pt, 2);
    rect.left = pt[0].x; rect.top = pt[0].y; rect.right = pt[1].x; rect.bottom = pt[1].y;
  } else {
    rect.left = x; rect.top = y; rect.right = x+w; rect.bottom = y+h;
  }
  return RectInRegion(r,&rect);
#elif defined(__APPLE_QUARTZ__)
  CGRect arg = fl_cgrectmake_cocoa(x, y, w, h);
  for (int i = 0; i < r->count; i++) {
    CGRect test = CGRectIntersection(r->rects[i], arg);
    if (!CGRectIsEmpty(test)) return 1;
  }
  return 0;
#else
# error unsupported platform
#endif
}

// return rectangle surrounding intersection of this rectangle and clip:
int Fl_Graphics_Driver::clip_box(int x, int y, int w, int h, int& X, int& Y, int& W, int& H){
  X = x; Y = y; W = w; H = h;
  Fl_Region r = rstack[rstackptr];
  if (!r) return 0;
#if defined(USE_X11)
  switch (XRectInRegion(r, x, y, w, h)) {
  case 0: // completely outside
    W = H = 0;
    return 2;
  case 1: // completely inside:
    return 0;
  default: // partial:
    break;
  }
  Fl_Region rr = XRectangleRegion(x,y,w,h);
  Fl_Region temp = XCreateRegion();
  XIntersectRegion(r, rr, temp);
  XRectangle rect;
  XClipBox(temp, &rect);
  X = rect.x; Y = rect.y; W = rect.width; H = rect.height;
  XDestroyRegion(temp);
  XDestroyRegion(rr);
  return 1;
#elif defined(WIN32)
// The win32 API makes no distinction between partial and complete
// intersection, so we have to check for partial intersection ourselves.
// However, given that the regions may be composite, we have to do
// some voodoo stuff...
  Fl_Region rr = XRectangleRegion(x,y,w,h);
  Fl_Region temp = CreateRectRgn(0,0,0,0);
  int ret;
  if (CombineRgn(temp, rr, r, RGN_AND) == NULLREGION) { // disjoint
    W = H = 0;
    ret = 2;
  } else if (EqualRgn(temp, rr)) { // complete
    ret = 0;
  } else {	// partial intersection
    RECT rect;
    GetRgnBox(temp, &rect);
    if(Fl_Surface_Device::surface()->class_name() == Fl_Printer::class_id) { // if print context, convert coords from device to logical
      POINT pt[2] = { {rect.left, rect.top}, {rect.right, rect.bottom} };
      DPtoLP(fl_gc, pt, 2);
      X = pt[0].x; Y = pt[0].y; W = pt[1].x - X; H = pt[1].y - Y;
    }
    else {
      X = rect.left; Y = rect.top; W = rect.right - X; H = rect.bottom - Y;
      }
    ret = 1;
  }
  DeleteObject(temp);
  DeleteObject(rr);
  return ret;
#elif defined(__APPLE_QUARTZ__)
  CGRect arg = fl_cgrectmake_cocoa(x, y, w, h);
  CGRect u = CGRectMake(0,0,0,0);
  CGRect test;
  for(int i = 0; i < r->count; i++) {
    test = CGRectIntersection(r->rects[i], arg);
    if( ! CGRectIsEmpty(test) ) {
      if(CGRectIsEmpty(u)) u = test;
      else u = CGRectUnion(u, test);
    }
  }
  X = int(u.origin.x);
  Y = int(u.origin.y);
  W = int(u.size.width + 1);
  H = int(u.size.height + 1);
  if(CGRectIsEmpty(u)) W = H = 0;
  return ! CGRectEqualToRect(arg, u);
#else
# error unsupported platform
#endif
}

//
// End of "$Id: fl_rect.cxx 8630 2011-05-01 12:45:29Z AlbrechtS $".
//
