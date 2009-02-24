//
// "$Id: fl_boxtype.cxx 5505 2006-10-03 02:35:12Z mike $"
//
// Box drawing code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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

// Box drawing code for the common box types and the table of
// boxtypes.  Other box types are in seperate files so they are not
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

int Fl::draw_box_active() { return draw_it_active; }

uchar *fl_gray_ramp() {return (draw_it_active?active_ramp:inactive_ramp)-'A';}

void fl_frame(const char* s, int x, int y, int w, int h) {
  uchar *g = fl_gray_ramp();
  if (h > 0 && w > 0) for (;*s;) {
    // draw top line:
    fl_color(g[*s++]);
    fl_xyline(x, y, x+w-1);
    y++; if (--h <= 0) break;
    // draw left line:
    fl_color(g[*s++]);
    fl_yxline(x, y+h-1, y);
    x++; if (--w <= 0) break;
    // draw bottom line:
    fl_color(g[*s++]);
    fl_xyline(x, y+h-1, x+w-1);
    if (--h <= 0) break;
    // draw right line:
    fl_color(g[*s++]);
    fl_yxline(x+w-1, y+h-1, y);
    if (--w <= 0) break;
  }
}

void fl_frame2(const char* s, int x, int y, int w, int h) {
  uchar *g = fl_gray_ramp();
  if (h > 0 && w > 0) for (;*s;) {
    // draw bottom line:
    fl_color(g[*s++]);
    fl_xyline(x, y+h-1, x+w-1);
    if (--h <= 0) break;
    // draw right line:
    fl_color(g[*s++]);
    fl_yxline(x+w-1, y+h-1, y);
    if (--w <= 0) break;
    // draw top line:
    fl_color(g[*s++]);
    fl_xyline(x, y, x+w-1);
    y++; if (--h <= 0) break;
    // draw left line:
    fl_color(g[*s++]);
    fl_yxline(x, y+h-1, y);
    x++; if (--w <= 0) break;
  }
}

void fl_no_box(int, int, int, int, Fl_Color) {}

void fl_thin_down_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame2("WWHH",x,y,w,h);
}

void fl_thin_down_box(int x, int y, int w, int h, Fl_Color c) {
  fl_thin_down_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+1, y+1, w-2, h-2);
}

void fl_thin_up_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame2("HHWW",x,y,w,h);
}

void fl_thin_up_box(int x, int y, int w, int h, Fl_Color c) {
  fl_thin_up_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+1, y+1, w-2, h-2);
}

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

void fl_up_box(int x, int y, int w, int h, Fl_Color c) {
  fl_up_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+D1, y+D1, w-D2, h-D2);
}

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

void fl_down_box(int x, int y, int w, int h, Fl_Color c) {
  fl_down_frame(x,y,w,h,c);
  fl_color(c); fl_rectf(x+D1, y+D1, w-D2, h-D2);
}

void fl_engraved_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame("HHWWWWHH",x,y,w,h);
}

void fl_engraved_box(int x, int y, int w, int h, Fl_Color c) {
  fl_engraved_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+2, y+2, w-4, h-4);
}

void fl_embossed_frame(int x, int y, int w, int h, Fl_Color) {
  fl_frame("WWHHHHWW",x,y,w,h);
}

void fl_embossed_box(int x, int y, int w, int h, Fl_Color c) {
  fl_embossed_frame(x,y,w,h,c);
  fl_color(draw_it_active ? c : fl_inactive(c));
  fl_rectf(x+2, y+2, w-4, h-4);
}

void fl_rectbound(int x, int y, int w, int h, Fl_Color bgcolor) {
  fl_color(draw_it_active ? FL_BLACK : fl_inactive(FL_BLACK));
  fl_rect(x, y, w, h);
  fl_color(draw_it_active ? bgcolor : fl_inactive(bgcolor));
  fl_rectf(x+1, y+1, w-2, h-2);
}
#define fl_border_box fl_rectbound

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

int Fl::box_dx(Fl_Boxtype t) {return fl_box_table[t].dx;}
int Fl::box_dy(Fl_Boxtype t) {return fl_box_table[t].dy;}
int Fl::box_dw(Fl_Boxtype t) {return fl_box_table[t].dw;}
int Fl::box_dh(Fl_Boxtype t) {return fl_box_table[t].dh;}

void fl_internal_boxtype(Fl_Boxtype t, Fl_Box_Draw_F* f) {
  if (!fl_box_table[t].set) {
    fl_box_table[t].f   = f;
    fl_box_table[t].set = 1;
  }
}

Fl_Box_Draw_F *Fl::get_boxtype(Fl_Boxtype t) {
  return fl_box_table[t].f;
}

void Fl::set_boxtype(Fl_Boxtype t, Fl_Box_Draw_F* f,
		      uchar a, uchar b, uchar c, uchar d) {
  fl_box_table[t].f   = f;
  fl_box_table[t].set = 1;
  fl_box_table[t].dx  = a;
  fl_box_table[t].dy  = b;
  fl_box_table[t].dw  = c;
  fl_box_table[t].dh  = d;
}

void Fl::set_boxtype(Fl_Boxtype t, Fl_Boxtype f) {
  fl_box_table[t] = fl_box_table[f];
}

void fl_draw_box(Fl_Boxtype t, int x, int y, int w, int h, Fl_Color c) {
  if (t && fl_box_table[t].f) fl_box_table[t].f(x,y,w,h,c);
}

//extern Fl_Widget *fl_boxcheat; // hack set by Fl_Window.cxx

void Fl_Widget::draw_box() const {
  int t = box_;
  if (!t) return;
//   if (this == fl_boxcheat) {
//     fl_boxcheat = 0;
//     if (t == FL_FLAT_BOX) return;
//     t += 2; // convert box to frame
//   }
  draw_box((Fl_Boxtype)t, x_, y_, w_, h_, (Fl_Color)color_);
}

void Fl_Widget::draw_box(Fl_Boxtype b, Fl_Color c) const {
  draw_box(b, x_, y_, w_, h_, c);
}

void Fl_Widget::draw_box(Fl_Boxtype b, int X, int Y, int W, int H, Fl_Color c)
const {
  draw_it_active = active_r();
  fl_box_table[b].f(X, Y, W, H, c);
  draw_it_active = 1;
}

//
// End of "$Id: fl_boxtype.cxx 5505 2006-10-03 02:35:12Z mike $".
//
