//
// "$Id: fl_vertex.cxx 6616 2009-01-01 21:28:26Z matt $"
//
// Portable drawing routines for the Fast Light Tool Kit (FLTK).
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
  \file fl_vertex.cxx
  \brief  Portable drawing code for drawing arbitrary shapes with
          simple 2D transformations.
*/

// Portable drawing code for drawing arbitrary shapes with
// simple 2D transformations.  See also fl_arc.cxx

// matt: the Quartz implementation purposely doesn't use the Quartz matrix
//       operations for reasons of compatibility and maintainability

#include <config.h>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/math.h>
#include <stdlib.h>

struct matrix {double a, b, c, d, x, y;};

static matrix m = {1, 0, 0, 1, 0, 0};

static matrix stack[32];
static int sptr = 0;

/**
  Saves the current transformation matrix on the stack. 
  The maximum depth of the stack is 4.
*/
void fl_push_matrix() {
  if (sptr==32)
    Fl::error("fl_push_matrix(): matrix stack overflow.");
  else
    stack[sptr++] = m;
}

/**
  Restores the current transformation matrix from the stack.
*/
void fl_pop_matrix() {
  if (sptr==0)
    Fl::error("fl_pop_matrix(): matrix stack underflow.");
  else 
    m = stack[--sptr];
}

/**
  Concatenates another transformation onto the current one.

  \param[in] a,b,c,d,x,y transformation matrix elements such that
             <tt> X' = aX + cY + x </tt> and <tt> Y' = bX +dY + y </tt>
*/
void fl_mult_matrix(double a, double b, double c, double d, double x, double y) {
  matrix o;
  o.a = a*m.a + b*m.c;
  o.b = a*m.b + b*m.d;
  o.c = c*m.a + d*m.c;
  o.d = c*m.b + d*m.d;
  o.x = x*m.a + y*m.c + m.x;
  o.y = x*m.b + y*m.d + m.y;
  m = o;
}

/**
  Concatenates scaling transformation onto the current one.
  \param[in] x,y scale factors in x-direction and y-direction
*/
void fl_scale(double x,double y) {fl_mult_matrix(x,0,0,y,0,0);}

/**
  Concatenates scaling transformation onto the current one.
  \param[in] x scale factor in both x-direction and y-direction
*/
void fl_scale(double x) {fl_mult_matrix(x,0,0,x,0,0);}

/**
  Concatenates translation transformation onto the current one.
  \param[in] x,y translation factor in x-direction and y-direction
*/
void fl_translate(double x,double y) {fl_mult_matrix(1,0,0,1,x,y);}

/**
  Concatenates rotation transformation onto the current one.
  \param[in] d - rotation angle, counter-clockwise in degrees (not radians)
*/
void fl_rotate(double d) {
  if (d) {
    double s, c;
    if (d == 0) {s = 0; c = 1;}
    else if (d == 90) {s = 1; c = 0;}
    else if (d == 180) {s = 0; c = -1;}
    else if (d == 270 || d == -90) {s = -1; c = 0;}
    else {s = sin(d*M_PI/180); c = cos(d*M_PI/180);}
    fl_mult_matrix(c,-s,s,c,0,0);
  }
}

// typedef what the x,y fields in a point are:
#ifdef WIN32
typedef int COORD_T;
#  define XPOINT XPoint
#elif defined(__APPLE_QUARTZ__)
typedef float COORD_T;
typedef struct { float x; float y; } QPoint;
#  define XPOINT QPoint
extern float fl_quartz_line_width_;
#else
typedef short COORD_T;
#  define XPOINT XPoint
#endif

static XPOINT *p = (XPOINT *)0;

static int p_size;
static int n;
static int what;
enum {LINE, LOOP, POLYGON, POINT_};

/**
  Starts drawing a list of points. Points are added to the list with fl_vertex()
*/
void fl_begin_points() {n = 0; what = POINT_;}

/**
  Starts drawing a list of lines.
*/
void fl_begin_line() {n = 0; what = LINE;}

/**
  Starts drawing a closed sequence of lines.
*/
void fl_begin_loop() {n = 0; what = LOOP;}

/**
  Starts drawing a convex filled polygon.
*/
void fl_begin_polygon() {n = 0; what = POLYGON;}

/**
  Transforms coordinate using the current transformation matrix.
  \param[in] x,y coordinate
*/
double fl_transform_x(double x, double y) {return x*m.a + y*m.c + m.x;}

/**
  Transform coordinate using the current transformation matrix.
  \param[in] x,y coordinate
*/
double fl_transform_y(double x, double y) {return x*m.b + y*m.d + m.y;}

/**
  Transforms distance using current transformation matrix.
  \param[in] x,y coordinate
*/
double fl_transform_dx(double x, double y) {return x*m.a + y*m.c;}

/**
  Transforms distance using current transformation matrix.
  \param[in] x,y coordinate
*/
double fl_transform_dy(double x, double y) {return x*m.b + y*m.d;}

static void fl_transformed_vertex(COORD_T x, COORD_T y) {
  if (!n || x != p[n-1].x || y != p[n-1].y) {
    if (n >= p_size) {
      p_size = p ? 2*p_size : 16;
      p = (XPOINT*)realloc((void*)p, p_size*sizeof(*p));
    }
    p[n].x = x;
    p[n].y = y;
    n++;
  }
}

/**
  Adds coordinate pair to the vertex list without further transformations.
  \param[in] xf,yf transformed coordinate
*/
void fl_transformed_vertex(double xf, double yf) {
#ifdef __APPLE_QUARTZ__
  fl_transformed_vertex(COORD_T(xf), COORD_T(yf));
#else
  fl_transformed_vertex(COORD_T(rint(xf)), COORD_T(rint(yf)));
#endif
}

/**
  Adds a single vertex to the current path.
  \param[in] x,y coordinate
*/
void fl_vertex(double x,double y) {
  fl_transformed_vertex(x*m.a + y*m.c + m.x, x*m.b + y*m.d + m.y);
}

/**
  Ends list of points, and draws.
*/
void fl_end_points() {
#if defined(USE_X11)
  if (n>1) XDrawPoints(fl_display, fl_window, fl_gc, p, n, 0);
#elif defined(WIN32)
  for (int i=0; i<n; i++) SetPixel(fl_gc, p[i].x, p[i].y, fl_RGB());
#elif defined(__APPLE_QUARTZ__)
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
  for (int i=0; i<n; i++) { 
    CGContextMoveToPoint(fl_gc, p[i].x, p[i].y);
    CGContextAddLineToPoint(fl_gc, p[i].x, p[i].y);
    CGContextStrokePath(fl_gc);
  }
  if (fl_quartz_line_width_==1.0f) CGContextSetShouldAntialias(fl_gc, false);
#else
# error unsupported platform
#endif
}

/**
  Ends list of lines, and draws.
*/
void fl_end_line() {
  if (n < 2) {
    fl_end_points();
    return;
  }
#if defined(USE_X11)
  if (n>1) XDrawLines(fl_display, fl_window, fl_gc, p, n, 0);
#elif defined(WIN32)
  if (n>1) Polyline(fl_gc, p, n);
#elif defined(__APPLE_QUARTZ__)
  if (n<=1) return;
  CGContextMoveToPoint(fl_gc, p[0].x, p[0].y);
  for (int i=1; i<n; i++)
    CGContextAddLineToPoint(fl_gc, p[i].x, p[i].y);
  CGContextStrokePath(fl_gc);
#else
# error unsupported platform
#endif
}

static void fixloop() {  // remove equal points from closed path
  while (n>2 && p[n-1].x == p[0].x && p[n-1].y == p[0].y) n--;
}

/**
  Ends closed sequence of lines, and draws.
*/
void fl_end_loop() {
  fixloop();
  if (n>2) fl_transformed_vertex((COORD_T)p[0].x, (COORD_T)p[0].y);
  fl_end_line();
}

/**
  Ends convex filled polygon, and draws.
*/
void fl_end_polygon() {
  fixloop();
  if (n < 3) {
    fl_end_line();
    return;
  }
#if defined(USE_X11)
  if (n>2) XFillPolygon(fl_display, fl_window, fl_gc, p, n, Convex, 0);
#elif defined(WIN32)
  if (n>2) {
    SelectObject(fl_gc, fl_brush());
    Polygon(fl_gc, p, n);
  }
#elif defined(__APPLE_QUARTZ__)
  if (n<=1) return;
  CGContextMoveToPoint(fl_gc, p[0].x, p[0].y);
  for (int i=1; i<n; i++) 
    CGContextAddLineToPoint(fl_gc, p[i].x, p[i].y);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
#else
# error unsupported platform
#endif
}

static int gap;
#if defined(WIN32)
static int counts[20];
static int numcount;
#endif

/**
  Starts drawing a complex filled polygon.

  The polygon may be concave, may have holes in it, or may be several
  disconnected pieces. Call fl_gap() to separate loops of the path.

  To outline the polygon, use fl_begin_loop() and replace each fl_gap()
  with fl_end_loop();fl_begin_loop() pairs.

  \note
  For portability, you should only draw polygons that appear the same
  whether "even/odd" or "non-zero" winding rules are used to fill them.
  Holes should be drawn in the opposite direction to the outside loop.
*/
void fl_begin_complex_polygon() {
  fl_begin_polygon();
  gap = 0;
#if defined(WIN32)
  numcount = 0;
#endif
}

/**
  Call fl_gap() to separate loops of the path.

  It is unnecessary but harmless to call fl_gap() before the first vertex,
  after the last vertex, or several times in a row.
*/
void fl_gap() {
  while (n>gap+2 && p[n-1].x == p[gap].x && p[n-1].y == p[gap].y) n--;
  if (n > gap+2) {
    fl_transformed_vertex((COORD_T)p[gap].x, (COORD_T)p[gap].y);
#if defined(WIN32)
    counts[numcount++] = n-gap;
#endif
    gap = n;
  } else {
    n = gap;
  }
}

/**
  Ends complex filled polygon, and draws.
*/
void fl_end_complex_polygon() {
  fl_gap();
  if (n < 3) {
    fl_end_line();
    return;
  }
#if defined(USE_X11)
  if (n>2) XFillPolygon(fl_display, fl_window, fl_gc, p, n, 0, 0);
#elif defined(WIN32)
  if (n>2) {
    SelectObject(fl_gc, fl_brush());
    PolyPolygon(fl_gc, p, counts, numcount);
  }
#elif defined(__APPLE_QUARTZ__)
  if (n<=1) return;
  CGContextMoveToPoint(fl_gc, p[0].x, p[0].y);
  for (int i=1; i<n; i++)
    CGContextAddLineToPoint(fl_gc, p[i].x, p[i].y);
  CGContextClosePath(fl_gc);
  CGContextFillPath(fl_gc);
#else
# error unsupported platform
#endif
}

// shortcut the closed circles so they use XDrawArc:
// warning: these do not draw rotated ellipses correctly!
// See fl_arc.c for portable version.

/**
  fl_circle() is equivalent to fl_arc(x,y,r,0,360), but may be faster.

  It must be the \e only thing in the path: if you want a circle as part of
  a complex polygon you must use fl_arc()
  \param[in] x,y,r center and radius of circle
*/
void fl_circle(double x, double y,double r) {
  double xt = fl_transform_x(x,y);
  double yt = fl_transform_y(x,y);
  double rx = r * (m.c ? sqrt(m.a*m.a+m.c*m.c) : fabs(m.a));
  double ry = r * (m.b ? sqrt(m.b*m.b+m.d*m.d) : fabs(m.d));
  int llx = (int)rint(xt-rx);
  int w = (int)rint(xt+rx)-llx;
  int lly = (int)rint(yt-ry);
  int h = (int)rint(yt+ry)-lly;

#if defined(USE_X11)
  (what == POLYGON ? XFillArc : XDrawArc)
    (fl_display, fl_window, fl_gc, llx, lly, w, h, 0, 360*64);
#elif defined(WIN32)
  if (what==POLYGON) {
    SelectObject(fl_gc, fl_brush());
    Pie(fl_gc, llx, lly, llx+w, lly+h, 0,0, 0,0); 
  } else
    Arc(fl_gc, llx, lly, llx+w, lly+h, 0,0, 0,0); 
#elif defined(__APPLE_QUARTZ__)
  // Quartz warning : circle won't scale to current matrix!
  CGContextAddArc(fl_gc, xt, yt, (w+h)*0.25f, 0, 2.0f*M_PI, 1);
  (what == POLYGON ? CGContextFillPath : CGContextStrokePath)(fl_gc);
#else
# error unsupported platform
#endif
}

//
// End of "$Id: fl_vertex.cxx 6616 2009-01-01 21:28:26Z matt $".
//
