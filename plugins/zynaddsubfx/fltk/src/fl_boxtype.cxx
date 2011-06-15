//
// "$Id: fl_boxtype.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Box drawing code for the Fast Light Tool Kit (FLTK).
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
  \file fl_boxtype.cxx
  \brief drawing code for common box types.
*/

// Box drawing code for the common box types and the table of
// boxtypes.  Other box types are in separate files so they are not
// linked in if not used.

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <config.h>

////////////////////////////////////////////////////////////////

static uchar active_ramp[24] = {
  FL_GRAY_RAMP+0, FL_GRAY_RAMP+1, FL_GRAY_RAMP+2, FL_GRAY_RAMP+3,
  FL_GRAY_RAMP+4, FL_GRAY_RAMP+5, FL_GRAY_RAMP+6, FL_GRAY_RAMP+7,
  FL_GRAY_RAMP+8, FL_GRAY_RAMP+9, FL_GRAY_RAMP+10,FL_GRAY_RAMP+11,
  FL_GRAY_RAMP+12,FL_GRAY_RAMP+13,FL_GRAY_RAMP+14,FL_GRAY_RAMP+15,
  FL_GRAY_RAMP+16,FL_GRAY_RAMP+17,FL_GRAY_RAMP+18,FL_GRAY_RAMP+19,
  FL_GRAY_RAMP+20,FL_GRAY_RAMP+21,FL_GRAY_RAMP+22,FL_GRAY_RAMP+23};
static uchar inactive_ramp[24] = {
  43, 43, 44, 44,
  44, 45, 45, 46,
  46, 46, 47, 47,
  48, 48, 48, 49,
  49, 49, 50, 50,
  51, 51, 52, 52};
static int draw_it_active = 1;

/**
  Determines if the current draw box is active or inactive. 
  If inactive, the box color is changed by the inactive color.
*/
int Fl::draw_box_active() { return draw_it_active; }

uchar *fl_gray_ramp() {return (draw_it_active?active_ramp:inactive_ramp)-'A';}

/**
  Draws a series of line segments around the given box.
  The string \p s must contain groups of 4 letters which specify one of 24
  standard grayscale values, where 'A' is black and 'X' is white.
  The order of each set of 4 characters is: top, left, bottom, right.
  The result of calling fl_frame() with a string that is not a multiple
  of 4 characters in length is undefined.
  The only difference between this function and fl_frame2() is the order
  of the line segments.
  \param[in] s sets of 4 grayscale values in top, left, bottom, right order
  \param[in] x, y, w, h position and size
*/
void fl_frame(const char* s, int x, int y, int w, int h) {
  uchar *g = fl_gray_ramp();
  if (h > 0 && w > 0) for (;*s;) {
    // draw top line:
    fl_color(g[(int)*s++]);
    fl_xyline(x, y, x+w-1);
    y++; if (--h <= 0) break;
    // draw left line:
    fl_color(g[(int)*s++]);
    fl_yxline(x, y+h-1, y);
    x++; if (--w <= 0) break;
    // draw bottom line:
    fl_color(g[(int)*s++]);
    fl_xyline(x, y+h-1, x+w-1);
    if (--h <= 0) break;
    // draw right line:
    fl_color(g[(int)*s++]);
    fl_yxline(x+w-1, y+h-1, y);
    if (--w <= 0) break;
  }
}

/**
  Draws a series of line segments around the given box.
  The string \p s must contain groups of 4 letters which specify one of 24
  standard grayscale values, where 'A' is black and 'X' is white.
  The order of each set of 4 characters is: bottom, right, top, left.
  The result of calling fl_frame2() with a string that is not a multiple
  of 4 characters in length is undefined.
  The only difference between this function and fl_frame() is the order
  of the line segments.
  \param[in] s sets of 4 grayscale values in bottom, right, top, left order
  \param[in] x, y, w, h position and size
*/
void fl_frame2(const char* s, int x, int y, int w, int h) {
  uchar *g = fl_gray_ramp();
  if (h > 0 && w > 0) for (;*s;) {
    // draw bottom line:
    fl_color(g[(int)*s++]);
    fl_xyline(x, y+h-1, x+w-1);
    if (--h <= 0) break;
    // draw right line:
    fl_color(g[(int)*s++]);
    fl_yxline(x+w-1, y+h-1, y);
    if (--w <= 0) break;
    // draw top line:
    fl_color(g[(int)*s++]);
    fl_xyline(x, y, x+w-1);
    y++; if (--h <= 0) break;
    // draw left line:
    fl_color(g[(int)*s++]);
    fl_yxline(x, y+h-1, y);
    x++; if (--w <= 0) break;
  }
}

/** Draws a box of type FL_NO_BOX */
void fl_no_box(int, int, int, int, Fl_Color) {}

/** Draws a frame of type FL_THIN_DOWN_FRAME */
void fl_thin_down_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame2("WWHH",x,y,w,h);
}

/** Draws a box of type FL_THIN_DOWN_BOX */
void fl_thin_down_box(int x, int y, int w, int h, Fl_Color c) {
  fl_thin_down_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+1, y+1, w-2, h-2);
}

/** Draws a frame of type FL_THIN_UP_FRAME */
void fl_thin_up_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame2("HHWW",x,y,w,h);
}

/** Draws a box of type FL_THIN_UP_BOX */
void fl_thin_up_box(int x, int y, int w, int h, Fl_Color c) {
  fl_thin_up_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+1, y+1, w-2, h-2);
}

/** Draws a frame of type FL_UP_FRAME */
void fl_up_frame(int x, int y, int w, int h, Fl_Color) {
#if BORDER_WIDTH == 1
  fl_frame2("HHWW",x,y,w,h);
#else
#if BORDER_WIDTH == 2
  fl_frame2("AAWWMMTT",x,y,w,h);
#else
  fl_frame("AAAAWWJJUTNN",x,y,w,h);
#endif
#endif
}

#define D1 BORDER_WIDTH
#define D2 (BORDER_WIDTH+BORDER_WIDTH)

/** Draws a box of type FL_UP_BOX */
void fl_up_box(int x, int y, int w, int h, Fl_Color c) {
  fl_up_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+D1, y+D1, w-D2, h-D2);
}

/** Draws a frame of type FL_DOWN_FRAME */
void fl_down_frame(int x, int y, int w, int h, Fl_Color) {
#if BORDER_WIDTH == 1
  fl_frame2("WWHH",x,y,w,h);
#else
#if BORDER_WIDTH == 2
  fl_frame2("WWMMPPAA",x,y,w,h);
#else
  fl_frame("NNTUJJWWAAAA",x,y,w,h);
#endif
#endif
}

/** Draws a box of type FL_DOWN_BOX */
void fl_down_box(int x, int y, int w, int h, Fl_Color c) {
  fl_down_frame(x,y,w,h,c);
  fl_color(c); fl_rectf(x+D1, y+D1, w-D2, h-D2);
}

/** Draws a frame of type FL_ENGRAVED_FRAME */
void fl_engraved_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame("HHWWWWHH",x,y,w,h);
}

/** Draws a box of type FL_ENGRAVED_BOX */
void fl_engraved_box(int x, int y, int w, int h, Fl_Color c) {
  fl_engraved_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+2, y+2, w-4, h-4);
}

/** Draws a frame of type FL_EMBOSSED_FRAME */
void fl_embossed_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame("WWHHHHWW",x,y,w,h);
}

/** Draws a box of type FL_EMBOSSED_BOX */
void fl_embossed_box(int x, int y, int w, int h, Fl_Color c) {
  fl_embossed_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+2, y+2, w-4, h-4);
}

/**
  Draws a bounded rectangle with a given position, size and color.
  Equivalent to drawing a box of type FL_BORDER_BOX.
*/
void fl_rectbound(int x, int y, int w, int h, Fl_Color bgcolor) {
  fl_color(draw_it_active ? FL_BLACK : fl_inactive(FL_BLACK));
  fl_rect(x, y, w, h);
  fl_color(draw_it_active ? bgcolor : fl_inactive(bgcolor));
  fl_rectf(x+1, y+1, w-2, h-2);
}
#define fl_border_box fl_rectbound	/**< allow consistent naming */

/**
  Draws a frame of type FL_BORDER_FRAME.
*/
void fl_border_frame(int x, int y, int w, int h, Fl_Color c) {
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rect(x, y, w, h);
}

////////////////////////////////////////////////////////////////

static struct {
  Fl_Box_Draw_F *f;
  uchar dx, dy, dw, dh;
  int set;
} fl_box_table[256] = {
// must match list in Enumerations.H!!!
  {fl_no_box,		0,0,0,0,1},		
  {fl_rectf,		0,0,0,0,1}, // FL_FLAT_BOX
  {fl_up_box,		D1,D1,D2,D2,1},
  {fl_down_box,		D1,D1,D2,D2,1},
  {fl_up_frame,		D1,D1,D2,D2,1},
  {fl_down_frame,	D1,D1,D2,D2,1},
  {fl_thin_up_box,	1,1,2,2,1},
  {fl_thin_down_box,	1,1,2,2,1},
  {fl_thin_up_frame,	1,1,2,2,1},
  {fl_thin_down_frame,	1,1,2,2,1},
  {fl_engraved_box,	2,2,4,4,1},
  {fl_embossed_box,	2,2,4,4,1},
  {fl_engraved_frame,	2,2,4,4,1},
  {fl_embossed_frame,	2,2,4,4,1},
  {fl_border_box,	1,1,2,2,1},
  {fl_border_box,	1,1,5,5,0}, // _FL_SHADOW_BOX,
  {fl_border_frame,	1,1,2,2,1},
  {fl_border_frame,	1,1,5,5,0}, // _FL_SHADOW_FRAME,
  {fl_border_box,	1,1,2,2,0}, // _FL_ROUNDED_BOX,
  {fl_border_box,	1,1,2,2,0}, // _FL_RSHADOW_BOX,
  {fl_border_frame,	1,1,2,2,0}, // _FL_ROUNDED_FRAME
  {fl_rectf,		0,0,0,0,0}, // _FL_RFLAT_BOX,
  {fl_up_box,		3,3,6,6,0}, // _FL_ROUND_UP_BOX
  {fl_down_box,		3,3,6,6,0}, // _FL_ROUND_DOWN_BOX,
  {fl_up_box,		0,0,0,0,0}, // _FL_DIAMOND_UP_BOX
  {fl_down_box,		0,0,0,0,0}, // _FL_DIAMOND_DOWN_BOX
  {fl_border_box,	1,1,2,2,0}, // _FL_OVAL_BOX,
  {fl_border_box,	1,1,2,2,0}, // _FL_OVAL_SHADOW_BOX,
  {fl_border_frame,	1,1,2,2,0}, // _FL_OVAL_FRAME
  {fl_rectf,		0,0,0,0,0}, // _FL_OVAL_FLAT_BOX,
  {fl_up_box,		4,4,8,8,0}, // _FL_PLASTIC_UP_BOX,
  {fl_down_box,		2,2,4,4,0}, // _FL_PLASTIC_DOWN_BOX,
  {fl_up_frame,		2,2,4,4,0}, // _FL_PLASTIC_UP_FRAME,
  {fl_down_frame,	2,2,4,4,0}, // _FL_PLASTIC_DOWN_FRAME,
  {fl_up_box,		2,2,4,4,0}, // _FL_PLASTIC_THIN_UP_BOX,
  {fl_down_box,		2,2,4,4,0}, // _FL_PLASTIC_THIN_DOWN_BOX,
  {fl_up_box,		2,2,4,4,0}, // _FL_PLASTIC_ROUND_UP_BOX,
  {fl_down_box,		2,2,4,4,0}, // _FL_PLASTIC_ROUND_DOWN_BOX,
  {fl_up_box,		2,2,4,4,0}, // _FL_GTK_UP_BOX,
  {fl_down_box,		2,2,4,4,0}, // _FL_GTK_DOWN_BOX,
  {fl_up_frame,		2,2,4,4,0}, // _FL_GTK_UP_FRAME,
  {fl_down_frame,	2,2,4,4,0}, // _FL_GTK_DOWN_FRAME,
  {fl_up_frame,		1,1,2,2,0}, // _FL_GTK_THIN_UP_FRAME,
  {fl_down_frame,	1,1,2,2,0}, // _FL_GTK_THIN_DOWN_FRAME,
  {fl_up_box,		1,1,2,2,0}, // _FL_GTK_THIN_ROUND_UP_BOX,
  {fl_down_box,		1,1,2,2,0}, // _FL_GTK_THIN_ROUND_DOWN_BOX,
  {fl_up_box,		2,2,4,4,0}, // _FL_GTK_ROUND_UP_BOX,
  {fl_down_box,		2,2,4,4,0}, // _FL_GTK_ROUND_DOWN_BOX,
  {fl_up_box,		3,3,6,6,0}, // FL_FREE_BOX+0
  {fl_down_box,		3,3,6,6,0}, // FL_FREE_BOX+1
  {fl_up_box,		3,3,6,6,0}, // FL_FREE_BOX+2
  {fl_down_box,		3,3,6,6,0}, // FL_FREE_BOX+3
  {fl_up_box,		3,3,6,6,0}, // FL_FREE_BOX+4
  {fl_down_box,		3,3,6,6,0}, // FL_FREE_BOX+5
  {fl_up_box,		3,3,6,6,0}, // FL_FREE_BOX+6
  {fl_down_box,		3,3,6,6,0}, // FL_FREE_BOX+7
};

/**
  Returns the X offset for the given boxtype.
  \see box_dy()
*/
int Fl::box_dx(Fl_Boxtype t) {return fl_box_table[t].dx;}

/**
    Returns the Y offset for the given boxtype.

    These functions return the offset values necessary for a given
    boxtype, useful for computing the area inside a box's borders, to
    prevent overdrawing the borders.

    For instance, in the case of a boxtype like FL_DOWN_BOX
    where the border width might be 2 pixels all around, the above 
    functions would return 2, 2, 4, and 4 for box_dx, 
    box_dy, box_dw, and box_dh respectively.

    An example to compute the area inside a widget's box():
    \code
         int X = yourwidget->x() + Fl::box_dx(yourwidget->box());
         int Y = yourwidget->y() + Fl::box_dy(yourwidget->box());
         int W = yourwidget->w() - Fl::box_dw(yourwidget->box());
         int H = yourwidget->h() - Fl::box_dh(yourwidget->box());
    \endcode
    These functions are mainly useful in the draw() code 
    for deriving custom widgets, where one wants to avoid drawing 
    over the widget's own border box().
*/
int Fl::box_dy(Fl_Boxtype t) {return fl_box_table[t].dy;}

/**
  Returns the width offset for the given boxtype.
  \see box_dy().
*/
int Fl::box_dw(Fl_Boxtype t) {return fl_box_table[t].dw;}

/**
  Returns the height offset for the given boxtype.
  \see box_dy().
*/
int Fl::box_dh(Fl_Boxtype t) {return fl_box_table[t].dh;}

/**
  Sets the drawing function for a given box type.
  \param[in] t box type
  \param[in] f box drawing function
*/
void fl_internal_boxtype(Fl_Boxtype t, Fl_Box_Draw_F* f) {
  if (!fl_box_table[t].set) {
    fl_box_table[t].f   = f;
    fl_box_table[t].set = 1;
  }
}

/** Gets the current box drawing function for the specified box type. */
Fl_Box_Draw_F *Fl::get_boxtype(Fl_Boxtype t) {
  return fl_box_table[t].f;
}
/** Sets the function to call to draw a specific boxtype. */
void Fl::set_boxtype(Fl_Boxtype t, Fl_Box_Draw_F* f,
		      uchar a, uchar b, uchar c, uchar d) {
  fl_box_table[t].f   = f;
  fl_box_table[t].set = 1;
  fl_box_table[t].dx  = a;
  fl_box_table[t].dy  = b;
  fl_box_table[t].dw  = c;
  fl_box_table[t].dh  = d;
}
/** Copies the from boxtype. */
void Fl::set_boxtype(Fl_Boxtype to, Fl_Boxtype from) {
  fl_box_table[to] = fl_box_table[from];
}

/**
  Draws a box using given type, position, size and color.
  \param[in] t box type
  \param[in] x, y, w, h position and size
  \param[in] c color
*/
void fl_draw_box(Fl_Boxtype t, int x, int y, int w, int h, Fl_Color c) {
  if (t && fl_box_table[t].f) fl_box_table[t].f(x,y,w,h,c);
}

//extern Fl_Widget *fl_boxcheat; // hack set by Fl_Window.cxx
/** Draws the widget box according its box style */
void Fl_Widget::draw_box() const {
  if (box_) draw_box((Fl_Boxtype)box_, x_, y_, w_, h_, color_);
  draw_backdrop();
}
/** If FL_ALIGN_IMAGE_BACKDROP is set, the image or deimage will be drawn */
void Fl_Widget::draw_backdrop() const {
  if (align() & FL_ALIGN_IMAGE_BACKDROP) {
    const Fl_Image *img = image();
    // if there is no image, we will not draw the deimage either
    if (img && deimage() && !active_r())
      img = deimage();
    if (img) 
      ((Fl_Image*)img)->draw(x_+(w_-img->w())/2, y_+(h_-img->h())/2);
  }
}
/** Draws a box of type t, of color c at the widget's position and size. */
void Fl_Widget::draw_box(Fl_Boxtype t, Fl_Color c) const {
  draw_box(t, x_, y_, w_, h_, c);
}
/** Draws a box of type t, of color c at the position X,Y and size W,H. */
void Fl_Widget::draw_box(Fl_Boxtype t, int X, int Y, int W, int H, Fl_Color c) const {
  draw_it_active = active_r();
  fl_box_table[t].f(X, Y, W, H, c);
  draw_it_active = 1;
}

//
// End of "$Id: fl_boxtype.cxx 7903 2010-11-28 21:06:39Z matt $".
//
