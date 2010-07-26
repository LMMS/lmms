//
// "$Id: fl_rect.cxx 7617 2010-05-27 17:20:18Z manolo $"
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
#include <FL/Fl_Printer.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#ifdef __APPLE_QUARTZ__
extern float fl_quartz_line_width_;
#define USINGQUARTZPRINTER  (Fl_Surface_Device::surface()->type() == Fl_Printer::device_type)
#endif

void Fl_Graphics_Driver::rect(int x, int y, int w, int h) {

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
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGRect rect = CGRectMake(x, y, w-1, h-1);
  CGContextStrokeRect(fl_gc, rect);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::rectf(int x, int y, int w, int h) {
  if (w<=0 || h<=0) return;
#if defined(USE_X11)
  if (w && h) XFillRectangle(fl_display, fl_window, fl_gc, x, y, w, h);
#elif defined(WIN32)
  RECT rect;
  rect.left = x; rect.top = y;  
  rect.right = x + w; rect.bottom = y + h;
  FillRect(fl_gc, &rect, fl_brush());
#elif defined(__APPLE_QUARTZ__)
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGRect rect = CGRectMake(x, y, w-1, h-1);
  CGContextFillRect(fl_gc, rect);
  if (USINGQUARTZPRINTER || fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

void Fl_Graphics_Driver::xyline(int x, int y, int x1) {
#if defined(USE_X11)
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x1, y);
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
  XDrawLine(fl_display, fl_window, fl_gc, x, y, x, y1);
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
  XDrawPoint(fl_display, fl_window, fl_gc, x, y);
#elif defined(WIN32)
  SetPixel(fl_gc, x, y, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, true);
  CGContextMoveToPoint(fl_gc, x-.5, y); // Quartz needs a line that is one pixel long, or it will not draw anything
  CGContextAddLineToPoint(fl_gc, x+.5, y);
  CGContextStrokePath(fl_gc);
  if (fl_quartz_line_width_ > 1.5f) CGContextSetShouldAntialias(fl_gc, false);
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

void fl_restore_clip() {
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

void fl_clip_region(Fl_Region r) {
  Fl_Region oldr = rstack[rstackptr];
  if (oldr) XDestroyRegion(oldr);
  rstack[rstackptr] = r;
  fl_restore_clip();
}

Fl_Region fl_clip_region() {
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
      r = MacRectRegionIntersect(current, x,y,w,h);
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
  if (rstackptr < STACK_MAX) rstack[++rstackptr] = r;
  else Fl::warning("fl_push_clip: clip stack overflow!\n");
  fl_restore_clip();
}

// make there be no clip (used by fl_begin_offscreen() only!)
void Fl_Graphics_Driver::push_no_clip() {
  if (rstackptr < STACK_MAX) rstack[++rstackptr] = 0;
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
#if defined (USE_X11)
  return r ? XRectInRegion(r, x, y, w, h) : 1;
#elif defined(WIN32)
  if (!r) return 1;
  RECT rect;
  if (Fl_Surface_Device::surface()->type() == Fl_Printer::device_type) { // in case of print context, convert coords from logical to device
    POINT pt[2] = { {x, y}, {x + w, y + h} };
    LPtoDP(fl_gc, pt, 2);
    rect.left = pt[0].x; rect.top = pt[0].y; rect.right = pt[1].x; rect.bottom = pt[1].y;
    }
  else {
    rect.left = x; rect.top = y; rect.right = x+w; rect.bottom = y+h;
    }
  return RectInRegion(r,&rect);
#elif defined(__APPLE_QUARTZ__)
  if (!r) return 1;
  CGRect arg = fl_cgrectmake_cocoa(x, y, w, h);
  for(int i = 0; i < r->count; i++) {
    CGRect test = CGRectIntersection(r->rects[i], arg);
    if( ! CGRectIsEmpty(test)) return 1;
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
    if(Fl_Surface_Device::surface()->type() == Fl_Printer::device_type) { // if print context, convert coords from device to logical
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
  X = u.origin.x;
  Y = u.origin.y;
  W = u.size.width + 1;
  H = u.size.height + 1;
  if(CGRectIsEmpty(u)) W = H = 0;
  return ! CGRectEqualToRect(arg, u);
#else
# error unsupported platform
#endif
}

//
// End of "$Id: fl_rect.cxx 7617 2010-05-27 17:20:18Z manolo $".
//
