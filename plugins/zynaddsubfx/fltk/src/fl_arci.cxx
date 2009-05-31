//
// "$Id: fl_arci.cxx 6716 2009-03-24 01:40:44Z fabien $"
//
// Arc (integer) drawing functions for the Fast Light Tool Kit (FLTK).
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
  \file fl_arci.cxx
  \brief Utility functions for drawing circles using integers
*/

// "integer" circle drawing functions.  These draw the limited
// circle types provided by X and NT graphics.  The advantage of
// these is that small ones draw quite nicely (probably due to stored
// hand-drawn bitmaps of small circles!) and may be implemented by
// hardware and thus are fast.

// Probably should add fl_chord.

// 3/10/98: created

#include <FL/fl_draw.H>
#include <FL/x.H>
#ifdef WIN32
#  include <FL/math.h>
#endif
#include <config.h>

/**
  Draw ellipse sections using integer coordinates.
  
  These functions match the rather limited circle drawing code provided by X
  and WIN32. The advantage over using fl_arc with floating point coordinates
  is that they are faster because they often use the hardware, and they draw
  much nicer small circles, since the small sizes are often hard-coded bitmaps.

  If a complete circle is drawn it will fit inside the passed bounding box.
  The two angles are measured in degrees counterclockwise from 3 o'clock and
  are the starting and ending angle of the arc, \p a2 must be greater or equal
  to \p a1.

  fl_arc() draws a series of lines to approximate the arc. Notice that the
  integer version of fl_arc() has a different number of arguments than the
  double version fl_arc(double x, double y, double r, double start, double a)

  \param[in] x,y,w,h bounding box of complete circle
  \param[in] a1,a2 start and end angles of arc measured in degrees
             counter-clockwise from 3 o'clock. \p a2 must be greater
	     than or equal to \p a1.
*/
void fl_arc(int x,int y,int w,int h,double a1,double a2) {
  if (w <= 0 || h <= 0) return;

#if defined(USE_X11)
  XDrawArc(fl_display, fl_window, fl_gc, x,y,w-1,h-1, int(a1*64),int((a2-a1)*64));
#elif defined(WIN32)
  int xa = x+w/2+int(w*cos(a1/180.0*M_PI));
  int ya = y+h/2-int(h*sin(a1/180.0*M_PI));
  int xb = x+w/2+int(w*cos(a2/180.0*M_PI));
  int yb = y+h/2-int(h*sin(a2/180.0*M_PI));
  if (fabs(a1 - a2) < 90) {
    if (xa == xb && ya == yb) SetPixel(fl_gc, xa, ya, fl_RGB());
    else Arc(fl_gc, x, y, x+w, y+h, xa, ya, xb, yb);
  } else Arc(fl_gc, x, y, x+w, y+h, xa, ya, xb, yb);
#elif defined(__APPLE_QUARTZ__)
  a1 = (-a1)/180.0f*M_PI; a2 = (-a2)/180.0f*M_PI;
  float cx = x + 0.5f*w - 0.5f, cy = y + 0.5f*h - 0.5f;
  if (w!=h) {
    CGContextSaveGState(fl_gc);
    CGContextTranslateCTM(fl_gc, cx, cy);
    CGContextScaleCTM(fl_gc, w-1.0f, h-1.0f);
    CGContextAddArc(fl_gc, 0, 0, 0.5, a1, a2, 1);
    CGContextRestoreGState(fl_gc);
  } else {
    float r = (w+h)*0.25f-0.5f;
    CGContextAddArc(fl_gc, cx, cy, r, a1, a2, 1);
  }
  CGContextStrokePath(fl_gc);
#else
# error unsupported platform
#endif
}

/**
  Draw filled ellipse sections using integer coordinates.
  
  Like fl_arc(), but fl_pie() draws a filled-in pie slice.
  This slice may extend outside the line drawn by fl_arc();
  to avoid this use w - 1 and h - 1.

  \param[in] x,y,w,h bounding box of complete circle
  \param[in] a1,a2 start and end angles of arc measured in degrees
             counter-clockwise from 3 o'clock. \p a2 must be greater
	     than or equal to \p a1.
*/
void fl_pie(int x,int y,int w,int h,double a1,double a2) {
  if (w <= 0 || h <= 0) return;

#if defined(USE_X11)
  XFillArc(fl_display, fl_window, fl_gc, x,y,w-1,h-1, int(a1*64),int((a2-a1)*64));
#elif defined(WIN32)
  if (a1 == a2) return;
  int xa = x+w/2+int(w*cos(a1/180.0*M_PI));
  int ya = y+h/2-int(h*sin(a1/180.0*M_PI));
  int xb = x+w/2+int(w*cos(a2/180.0*M_PI));
  int yb = y+h/2-int(h*sin(a2/180.0*M_PI));
  SelectObject(fl_gc, fl_brush());
  if (fabs(a1 - a2) < 90) {
    if (xa == xb && ya == yb) {
      MoveToEx(fl_gc, x+w/2, y+h/2, 0L); 
      LineTo(fl_gc, xa, ya);
      SetPixel(fl_gc, xa, ya, fl_RGB());
    } else Pie(fl_gc, x, y, x+w, y+h, xa, ya, xb, yb);
  } else Pie(fl_gc, x, y, x+w, y+h, xa, ya, xb, yb); 
#elif defined(__APPLE_QUARTZ__)
  a1 = (-a1)/180.0f*M_PI; a2 = (-a2)/180.0f*M_PI;
  float cx = x + 0.5f*w - 0.5f, cy = y + 0.5f*h - 0.5f;
  if (w!=h) {
    CGContextSaveGState(fl_gc);
    CGContextTranslateCTM(fl_gc, cx, cy);
    CGContextScaleCTM(fl_gc, w, h);
    CGContextAddArc(fl_gc, 0, 0, 0.5, a1, a2, 1);
    CGContextAddLineToPoint(fl_gc, 0, 0);
    CGContextClosePath(fl_gc);
    CGContextRestoreGState(fl_gc);
  } else {
    float r = (w+h)*0.25f;
    CGContextAddArc(fl_gc, cx, cy, r, a1, a2, 1);
    CGContextAddLineToPoint(fl_gc, cx, cy);
    CGContextClosePath(fl_gc);
  }
  CGContextFillPath(fl_gc);
#else
# error unsupported platform
#endif
}

//
// End of "$Id: fl_arci.cxx 6716 2009-03-24 01:40:44Z fabien $".
//
