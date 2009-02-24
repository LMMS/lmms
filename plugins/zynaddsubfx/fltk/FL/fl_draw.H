//
// "$Id: fl_draw.H 5430 2006-09-15 15:35:16Z matt $"
//
// Portable drawing function header file for the Fast Light Tool Kit (FLTK).
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

#ifndef fl_draw_H
#define fl_draw_H

#include "Enumerations.H"  // for the color names

// Image class...
class Fl_Image;

// Label flags...
FL_EXPORT extern char fl_draw_shortcut;

// Colors:
FL_EXPORT void	fl_color(Fl_Color); // select indexed color
inline void fl_color(int c) {fl_color((Fl_Color)c);} // for back compatability
FL_EXPORT void	fl_color(uchar, uchar, uchar); // select actual color
extern FL_EXPORT Fl_Color fl_color_;
inline Fl_Color fl_color() {return fl_color_;}

// clip:
FL_EXPORT void fl_push_clip(int x, int y, int w, int h);
#define fl_clip fl_push_clip
FL_EXPORT void fl_push_no_clip();
FL_EXPORT void fl_pop_clip();
FL_EXPORT int fl_not_clipped(int x, int y, int w, int h);
FL_EXPORT int fl_clip_box(int, int, int, int, int& x, int& y, int& w, int& h);

// points:
FL_EXPORT void fl_point(int x, int y);

// line type:
FL_EXPORT void fl_line_style(int style, int width=0, char* dashes=0);
enum {
  FL_SOLID	= 0,
  FL_DASH	= 1,
  FL_DOT	= 2,
  FL_DASHDOT	= 3,
  FL_DASHDOTDOT	= 4,

  FL_CAP_FLAT	= 0x100,
  FL_CAP_ROUND	= 0x200,
  FL_CAP_SQUARE	= 0x300,

  FL_JOIN_MITER	= 0x1000,
  FL_JOIN_ROUND	= 0x2000,
  FL_JOIN_BEVEL	= 0x3000
};

// rectangles tweaked to exactly fill the pixel rectangle:
FL_EXPORT void fl_rect(int x, int y, int w, int h);
inline void fl_rect(int x, int y, int w, int h, Fl_Color c) {fl_color(c); fl_rect(x,y,w,h);}
FL_EXPORT void fl_rectf(int x, int y, int w, int h);
inline void fl_rectf(int x, int y, int w, int h, Fl_Color c) {fl_color(c); fl_rectf(x,y,w,h);}

// line segments:
FL_EXPORT void fl_line(int,int, int,int);
FL_EXPORT void fl_line(int,int, int,int, int,int);

// closed line segments:
FL_EXPORT void fl_loop(int,int, int,int, int,int);
FL_EXPORT void fl_loop(int,int, int,int, int,int, int,int);

// filled polygons
FL_EXPORT void fl_polygon(int,int, int,int, int,int);
FL_EXPORT void fl_polygon(int,int, int,int, int,int, int,int);

// draw rectilinear lines, horizontal segment first:
FL_EXPORT void fl_xyline(int x, int y, int x1);
FL_EXPORT void fl_xyline(int x, int y, int x1, int y2);
FL_EXPORT void fl_xyline(int x, int y, int x1, int y2, int x3);

// draw rectilinear lines, vertical segment first:
FL_EXPORT void fl_yxline(int x, int y, int y1);
FL_EXPORT void fl_yxline(int x, int y, int y1, int x2);
FL_EXPORT void fl_yxline(int x, int y, int y1, int x2, int y3);

// circular lines and pie slices (code in fl_arci.C):
FL_EXPORT void fl_arc(int x, int y, int w, int h, double a1, double a2);
FL_EXPORT void fl_pie(int x, int y, int w, int h, double a1, double a2);
FL_EXPORT void fl_chord(int x, int y, int w, int h, double a1, double a2); // nyi

// scalable drawing code (code in fl_vertex.C and fl_arc.C):
FL_EXPORT void fl_push_matrix();
FL_EXPORT void fl_pop_matrix();
FL_EXPORT void fl_scale(double x, double y);
FL_EXPORT void fl_scale(double x);
FL_EXPORT void fl_translate(double x, double y);
FL_EXPORT void fl_rotate(double d);
FL_EXPORT void fl_mult_matrix(double a, double b, double c, double d, double x,double y);
FL_EXPORT void fl_begin_points();
FL_EXPORT void fl_begin_line();
FL_EXPORT void fl_begin_loop();
FL_EXPORT void fl_begin_polygon();
FL_EXPORT void fl_vertex(double x, double y);
FL_EXPORT void fl_curve(double, double, double, double, double, double, double, double);
FL_EXPORT void fl_arc(double x, double y, double r, double start, double a);
FL_EXPORT void fl_circle(double x, double y, double r);
FL_EXPORT void fl_end_points();
FL_EXPORT void fl_end_line();
FL_EXPORT void fl_end_loop();
FL_EXPORT void fl_end_polygon();
FL_EXPORT void fl_begin_complex_polygon();
FL_EXPORT void fl_gap();
FL_EXPORT void fl_end_complex_polygon();
// get and use transformed positions:
FL_EXPORT double fl_transform_x(double x, double y);
FL_EXPORT double fl_transform_y(double x, double y);
FL_EXPORT double fl_transform_dx(double x, double y);
FL_EXPORT double fl_transform_dy(double x, double y);
FL_EXPORT void fl_transformed_vertex(double x, double y);

// current font:
FL_EXPORT void fl_font(int face, int size);
extern FL_EXPORT int fl_font_;
inline int fl_font() {return fl_font_;}
extern FL_EXPORT int fl_size_;
inline int fl_size() {return fl_size_;}

// information you can get about the current font:
FL_EXPORT int   fl_height();	// using "size" should work ok
inline int fl_height(int, int size) {return size;}
FL_EXPORT int   fl_descent();
FL_EXPORT double fl_width(const char*);
FL_EXPORT double fl_width(const char*, int n);
FL_EXPORT double fl_width(uchar);

// draw using current font:
FL_EXPORT void fl_draw(const char*, int x, int y);
FL_EXPORT void fl_draw(const char*, int n, int x, int y);
FL_EXPORT void fl_measure(const char*, int& x, int& y, int draw_symbols = 1);
FL_EXPORT void fl_draw(const char*, int,int,int,int, Fl_Align, Fl_Image* img=0,
                       int draw_symbols = 1);
FL_EXPORT void fl_draw(const char*, int,int,int,int, Fl_Align,
	               void (*callthis)(const char *, int n, int x, int y),
		       Fl_Image* img=0, int draw_symbols = 1);

// font encoding:
FL_EXPORT const char *fl_latin1_to_local(const char *, int n=-1);
FL_EXPORT const char *fl_local_to_latin1(const char *, int n=-1);
FL_EXPORT const char *fl_mac_roman_to_local(const char *, int n=-1);
FL_EXPORT const char *fl_local_to_mac_roman(const char *, int n=-1);

// boxtypes:
FL_EXPORT void fl_frame(const char* s, int x, int y, int w, int h);
FL_EXPORT void fl_frame2(const char* s, int x, int y, int w, int h);
FL_EXPORT void fl_draw_box(Fl_Boxtype, int x, int y, int w, int h, Fl_Color);

// images:
FL_EXPORT void fl_draw_image(const uchar*, int,int,int,int, int delta=3, int ldelta=0);
FL_EXPORT void fl_draw_image_mono(const uchar*, int,int,int,int, int delta=1, int ld=0);
typedef void (*Fl_Draw_Image_Cb)(void*,int,int,int,uchar*);
FL_EXPORT void fl_draw_image(Fl_Draw_Image_Cb, void*, int,int,int,int, int delta=3);
FL_EXPORT void fl_draw_image_mono(Fl_Draw_Image_Cb, void*, int,int,int,int, int delta=1);
FL_EXPORT void fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b);
FL_EXPORT char fl_can_do_alpha_blending();

FL_EXPORT uchar *fl_read_image(uchar *p, int x,int y, int w, int h, int alpha=0);

// pixmaps:
FL_EXPORT int fl_draw_pixmap(/*const*/ char* const* data, int x,int y,Fl_Color=FL_GRAY);
FL_EXPORT int fl_measure_pixmap(/*const*/ char* const* data, int &w, int &h);
FL_EXPORT int fl_draw_pixmap(const char* const* data, int x,int y,Fl_Color=FL_GRAY);
FL_EXPORT int fl_measure_pixmap(const char* const* data, int &w, int &h);

// other:
FL_EXPORT void fl_scroll(int X, int Y, int W, int H, int dx, int dy,
                         void (*draw_area)(void*, int,int,int,int), void* data);
FL_EXPORT const char* fl_shortcut_label(int);
FL_EXPORT void fl_overlay_rect(int,int,int,int);
FL_EXPORT void fl_overlay_clear();
FL_EXPORT void fl_cursor(Fl_Cursor, Fl_Color=FL_BLACK, Fl_Color=FL_WHITE);

// XForms symbols:
FL_EXPORT int fl_draw_symbol(const char* label,int x,int y,int w,int h, Fl_Color);
FL_EXPORT int fl_add_symbol(const char* name, void (*drawit)(Fl_Color), int scalable);

#endif

//
// End of "$Id: fl_draw.H 5430 2006-09-15 15:35:16Z matt $".
//
