//
// "$Id: fl_rect.cxx 6755 2009-04-12 13:48:03Z matt $"
//
// Rectangle drawing routines for the Fast Light Tool Kit (FLTK).
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
#include <FL/fl_draw.H>
#include <FL/x.H>

#ifdef __APPLE_QUARTZ__
extern float fl_quartz_line_width_;
#endif

/**
  Draws a 1-pixel border \e inside the given bounding box
*/
void fl_rect(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
#if defined(USE_X11)
  XDrawRectangle(fl_display, fl_window, fl_gc, x, y, w-1, h-1);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x+w-1, y);
  LineTo(fl_gc, x+w-1, y+h-1);
  LineTo(fl_gc, x, y+h-1);
  LineTo(fl_gc, x, y);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGRect rect = CGRectMake(x, y, w-1, h-1);
  CGContextStrokeRect(fl_gc, rect);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Colors a rectangle that exactly fills the given bounding box
*/
void fl_rectf(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
#if defined(USE_X11)
  if (w && h) XFillRectangle(fl_display, fl_window, fl_gc, x, y, w, h);
#elif defined(WIN32)
  RECT rect;
  rect.left = x; rect.top = y;  
  rect.right = x + w; rect.bottom = y + h;
  FillRect(fl_gc, &rect, fl_brush());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGRect rect = CGRectMake(x, y, w-1, h-1);
  CGContextFillRect(fl_gc, rect);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a horizontal line from (x,y) to (x1,y)
*/
void fl_xyline(int x, int y, int x1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x1, y);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); LineTo(fl_gc, x1+1, y);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a horizontal line from (x,y) to (x1,y), then vertical from (x1,y) to (x1,y2)
*/
void fl_xyline(int x, int y, int x1, int y2) {
#if defined (USE_X11)
  XPoint p[3];
  p[0].x = x;  p[0].y = p[1].y = y;
  p[1].x = p[2].x = x1; p[2].y = y2;
  XDrawLines(fl_display, fl_window, fl_gc, p, 3, 0);
#elif defined(WIN32)
  if (y2 < y) y2--;
  else y2++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y);
  LineTo(fl_gc, x1, y2);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextAddLineToPoint(fl_gc, x1, y2);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
#error unsupported platform
#endif
}

/**
  Draws a horizontal line from (x,y) to (x1,y), then a vertical from (x1,y) to (x1,y2)
  and then another horizontal from (x1,y2) to (x3,y2)
*/
void fl_xyline(int x, int y, int x1, int y2, int x3) {
#if defined(USE_X11)
  XPoint p[4];
  p[0].x = x;  p[0].y = p[1].y = y;
  p[1].x = p[2].x = x1; p[2].y = p[3].y = y2;
  p[3].x = x3;
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  if(x3 < x1) x3--;
  else x3++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y);
  LineTo(fl_gc, x1, y2);
  LineTo(fl_gc, x3, y2);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y);
  CGContextAddLineToPoint(fl_gc, x1, y2);
  CGContextAddLineToPoint(fl_gc, x3, y2);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a vertical line from (x,y) to (x,y1)
*/
void fl_yxline(int x, int y, int y1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x, y1);
#elif defined(WIN32)
  if (y1 < y) y1--;
  else y1++;
  MoveToEx(fl_gc, x, y, 0L); LineTo(fl_gc, x, y1);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a vertical line from (x,y) to (x,y1), then a horizontal from (x,y1) to (x2,y1)
*/
void fl_yxline(int x, int y, int y1, int x2) {
#if defined(USE_X11)
  XPoint p[3];
  p[0].x = p[1].x = x;  p[0].y = y;
  p[1].y = p[2].y = y1; p[2].x = x2;
  XDrawLines(fl_display, fl_window, fl_gc, p, 3, 0);
#elif defined(WIN32)
  if (x2 > x) x2++;
  else x2--;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x, y1);
  LineTo(fl_gc, x2, y1);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextAddLineToPoint(fl_gc, x2, y1);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a vertical line from (x,y) to (x,y1) then a horizontal from (x,y1)
  to (x2,y1), then another vertical from (x2,y1) to (x2,y3)
*/
void fl_yxline(int x, int y, int y1, int x2, int y3) {
#if defined(USE_X11)
  XPoint p[4];
  p[0].x = p[1].x = x;  p[0].y = y;
  p[1].y = p[2].y = y1; p[2].x = p[3].x = x2;
  p[3].y = y3;
  XDrawLines(fl_display, fl_window, fl_gc, p, 4, 0);
#elif defined(WIN32)
  if(y3<y1) y3--;
  else y3++;
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x, y1);
  LineTo(fl_gc, x2, y1);
  LineTo(fl_gc, x2, y3);
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x, y1);
  CGContextAddLineToPoint(fl_gc, x2, y1);
  CGContextAddLineToPoint(fl_gc, x2, y3);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a line from (x,y) to (x1,y1)
*/
void fl_line(int x, int y, int x1, int y1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x1, y1);
#elif defined(WIN32)
  MoveToEx(fl_gc, x, y, 0L); 
  LineTo(fl_gc, x1, y1);
  // Draw the last point *again* because the GDI line drawing
  // functions will not draw the last point ("it's a feature!"...)
  SetPixel(fl_gc, x1, y1, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f ) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Draws a line from (x,y) to (x1,y1) and another from (x1,y1) to (x2,y2)
*/
void fl_line(int x, int y, int x1, int y1, int x2, int y2) {
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
  if (fl_quartz_line_width_==1.0f ) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f ) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

/**
  Outlines a 3-sided polygon with lines
*/
void fl_loop(int x, int y, int x1, int y1, int x2, int y2) {
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
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextClosePath(fl_gc);
  CGContextStrokePath(fl_gc);
#else
# error unsupported platform
#endif
}

/**
  Outlines a 4-sided polygon with lines
*/
void fl_loop(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
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
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextAddLineToPoint(fl_gc, x3, y3);
  CGContextClosePath(fl_gc);
  CGContextStrokePath(fl_gc);
#else
# error unsupported platform
#endif
}

/**
  Fills a 3-sided polygon. The polygon must be convex.
*/
void fl_polygon(int x, int y, int x1, int y1, int x2, int y2) {
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
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
#else
# error unsupported platform
#endif
}

/**
  Fills a 4-sided polygon. The polygon must be convex.
*/
void fl_polygon(int x, int y, int x1, int y1, int x2, int y2, int x3, int y3) {
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
  CGContextMoveToPoint(fl_gc, x, y);
  CGContextAddLineToPoint(fl_gc, x1, y1);
  CGContextAddLineToPoint(fl_gc, x2, y2);
  CGContextAddLineToPoint(fl_gc, x3, y3);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
#else
# error unsupported platform
#endif
}

/**
  Draws a single pixel at the given coordinates
*/
void fl_point(int x, int y) {
#if defined(USE_X11)
  XDrawPoint(fl_display, fl_window, fl_gc, x, y);
#elif defined(WIN32)
  SetPixel(fl_gc, x, y, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  CGContextMoveToPoint(fl_gc, x-.5, y); // Quartz needs a line that is one pixel long, or it will not draw anything
  CGContextAddLineToPoint(fl_gc, x+.5, y);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, true);
#else
# error unsupported platform
#endif
}

////////////////////////////////////////////////////////////////

#define STACK_SIZE 10
#define STACK_MAX (STACK_SIZE - 1)
static Fl_Region rstack[STACK_SIZE];
static int rstackptr=0;
int fl_clip_state_number=0; // used by gl_begin.cxx to update GL clip

#if !defined(WIN32) && !defined(__APPLE__)
// Missing X call: (is this the fastest way to init a 1-rectangle region?)
// MSWindows equivalent exists, implemented inline in win32.H
Fl_Region XRectangleRegion(int x, int y, int w, int h) {
  XRectangle R;
  R.x = x; R.y = y; R.width = w; R.height = h;
  Fl_Region r = XCreateRegion();
  XUnionRectWithRegion(&R, r, r);
  return r;
}
#endif

#if defined(__APPLE_QUARTZ__)
// warning: the Quartz implementation currently uses Quickdraw calls to achieve
//          clipping. A future version should instead use 'CGContectClipToRect'
//          and friends.
extern Fl_Region fl_window_region;
#endif

/** Undoes any clobbering of clip done by your program */
void fl_restore_clip() {
  fl_clip_state_number++;
  Fl_Region r = rstack[rstackptr];
#if defined(USE_X11)
  if (r) XSetRegion(fl_display, fl_gc, r);
  else XSetClipMask(fl_display, fl_gc, 0);
#elif defined(WIN32)
  SelectClipRgn(fl_gc, r); //if r is NULL, clip is automatically cleared
#elif defined(__APPLE_QUARTZ__)
  if ( fl_window ) // clipping for a true window
  {
    GrafPtr port = GetWindowPort( fl_window );
    if ( port ) { 
      RgnHandle portClip = NewRgn();
      CopyRgn( fl_window_region, portClip ); // changed
      if ( r )
        SectRgn( portClip, r, portClip );
      Rect portRect; GetPortBounds(port, &portRect);
      Fl_X::q_clear_clipping();
      ClipCGContextToRegion(fl_gc, &portRect, portClip );
      Fl_X::q_fill_context();
      DisposeRgn( portClip );
    }
  } else if (fl_gc) { // clipping for an offscreen drawing world (CGBitmap)
    Rect portRect;
    portRect.top = 0;
    portRect.left = 0;
    portRect.bottom = CGBitmapContextGetHeight(fl_gc);
    portRect.right = CGBitmapContextGetWidth(fl_gc);
    Fl_X::q_clear_clipping();
    if (r)
      ClipCGContextToRegion(fl_gc, &portRect, r);
    Fl_X::q_fill_context();
  }
#else
# error unsupported platform
#endif
}

/**
  Replaces the top of the clipping stack with a clipping region of any shape.

  Fl_Region is an operating system specific type.
  \param[in] r clipping region
*/
void fl_clip_region(Fl_Region r) {
  Fl_Region oldr = rstack[rstackptr];
  if (oldr) XDestroyRegion(oldr);
  rstack[rstackptr] = r;
  fl_restore_clip();
}

/**
  \returns the current clipping region.
*/
Fl_Region fl_clip_region() {
  return rstack[rstackptr];
}

/**
  Intersects the current clip region with a rectangle and pushes this
  new region onto the stack.
  \param[in] x,y,w,h position and size
*/
void fl_push_clip(int x, int y, int w, int h) {
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
      SectRgn(r, current, r);
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
    r = NewRgn();
    SetEmptyRgn(r);
#else
# error unsupported platform
#endif
  }
  if (rstackptr < STACK_MAX) rstack[++rstackptr] = r;
  else Fl::warning("fl_push_clip: clip stack overflow!\n");
  fl_restore_clip();
}

// make there be no clip (used by fl_begin_offscreen() only!)
/**
  Pushes an empty clip region onto the stack so nothing will be clipped.
*/
void fl_push_no_clip() {
  if (rstackptr < STACK_MAX) rstack[++rstackptr] = 0;
  else Fl::warning("fl_push_no_clip: clip stack overflow!\n");
  fl_restore_clip();
}

// pop back to previous clip:
/**
  Restores the previous clip region.

  You must call fl_pop_clip() once for every time you call fl_push_clip().
  Unpredictable results may occur if the clip stack is not empty when
  you return to FLTK.
*/
void fl_pop_clip() {
  if (rstackptr > 0) {
    Fl_Region oldr = rstack[rstackptr--];
    if (oldr) XDestroyRegion(oldr);
  } else Fl::warning("fl_pop_clip: clip stack underflow!\n");
  fl_restore_clip();
}

/**
  Does the rectangle intersect the current clip region?
  \param[in] x,y,w,h position and size of rectangle
  \returns non-zero if any of the rectangle intersects the current clip
  region. If this returns 0 you don't have to draw the object.

  \note
  Under X this returns 2 if the rectangle is partially clipped, 
  and 1 if it is entirely inside the clip region.
*/
int fl_not_clipped(int x, int y, int w, int h) {
  if (x+w <= 0 || y+h <= 0) return 0;
  Fl_Region r = rstack[rstackptr];
#if defined (USE_X11)
  return r ? XRectInRegion(r, x, y, w, h) : 1;
#elif defined(WIN32)
  if (!r) return 1;
  RECT rect;
  rect.left = x; rect.top = y; rect.right = x+w; rect.bottom = y+h;
  return RectInRegion(r,&rect);
#elif defined(__APPLE_QUARTZ__)
  if (!r) return 1;
  Rect rect;
  rect.left = x; rect.top = y; rect.right = x+w; rect.bottom = y+h;
  return RectInRgn(&rect, r);
#else
# error unsupported platform
#endif
}

// return rectangle surrounding intersection of this rectangle and clip:
/**
  Intersects the rectangle with the current clip region and returns the
  bounding box of the result.

  Returns non-zero if the resulting rectangle is different to the original.
  This can be used to limit the necessary drawing to a rectangle.
  \p W and \p H are set to zero if the rectangle is completely outside
  the region.
  \param[in] x,y,w,h position and size of rectangle
  \param[out] X,Y,W,H position and size of resulting bounding box.
              \p W and \p H are set to zero if the rectangle is
	      completely outside the region.
  \returns Non-zero if the resulting rectangle is different to the original.
*/
int fl_clip_box(int x, int y, int w, int h, int& X, int& Y, int& W, int& H){
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
  } else {	// parital intersection
    RECT rect;
    GetRgnBox(temp, &rect);
    X = rect.left; Y = rect.top; W = rect.right - X; H = rect.bottom - Y;
    ret = 1;
  }
  DeleteObject(temp);
  DeleteObject(rr);
  return ret;
#elif defined(__APPLE_QUARTZ__)
  RgnHandle rr = NewRgn();
  SetRectRgn( rr, x, y, x+w, y+h );
  SectRgn( r, rr, rr );
  Rect rp; GetRegionBounds(rr, &rp);
  X = rp.left;
  Y = rp.top;
  W = rp.right - X;
  H = rp.bottom - Y;
  DisposeRgn( rr );
  if ( H==0 ) return 2;
  if ( h==H && w==W ) return 0;
  return 0;
#else
# error unsupported platform
#endif
}

//
// End of "$Id: fl_rect.cxx 6755 2009-04-12 13:48:03Z matt $".
//
