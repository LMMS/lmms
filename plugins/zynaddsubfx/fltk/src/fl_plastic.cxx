//
// "$Id: fl_plastic.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// "Plastic" drawing routines for the Fast Light Tool Kit (FLTK).
//
// These box types provide a cross between Aqua and KDE buttons; kindof
// like translucent plastic buttons...
//
// Copyright 2001-2005 by Michael Sweet.
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

// Box drawing code for an obscure box type.
// These box types are in seperate files so they are not linked
// in if not used.

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "flstring.h"

//
// Uncomment the following line to restore the old plastic box type
// appearance.
//

//#define USE_OLD_PLASTIC_BOX
#define USE_OLD_PLASTIC_COLOR

extern uchar *fl_gray_ramp();

inline Fl_Color shade_color(uchar gc, Fl_Color bc) {
#ifdef USE_OLD_PLASTIC_COLOR
  return fl_color_average((Fl_Color)gc, bc, 0.75f);
#else
  unsigned	grgb = Fl::get_color((Fl_Color)gc),
		brgb = Fl::get_color(bc);
  int		red, green, blue, gray;


  gray  = ((grgb >> 24) & 255);
  red   = gray * ((brgb >> 24) & 255) / 255 + gray * gray / 510;
  gray  = ((grgb >> 16) & 255);
  green = gray * ((brgb >> 16) & 255) / 255 + gray * gray / 510;
  gray  = ((grgb >> 8) & 255);
  blue  = gray * ((brgb >> 8) & 255) / 255 + gray * gray / 510;

  if (red > 255)
    red = 255;

  if (green > 255)
    green = 255;

  if (blue > 255)
    blue = 255;

  if (Fl::draw_box_active())
    return fl_rgb_color(red, green, blue);
  else
    return fl_color_average(FL_GRAY, fl_rgb_color(red, green, blue), 0.75f);
#endif // USE_OLD_PLASTIC_COLOR
}


static void frame_rect(int x, int y, int w, int h, const char *c, Fl_Color bc) {
  uchar *g = fl_gray_ramp();
  int b = strlen(c) / 4 + 1;

  for (x += b, y += b, w -= 2 * b, h -= 2 * b; b > 1; b --)
  {
    // Draw lines around the perimeter of the button, 4 colors per
    // circuit.
    fl_color(shade_color(g[*c++], bc));
    fl_line(x, y + h + b, x + w - 1, y + h + b, x + w + b - 1, y + h);
    fl_color(shade_color(g[*c++], bc));
    fl_line(x + w + b - 1, y + h, x + w + b - 1, y, x + w - 1, y - b);
    fl_color(shade_color(g[*c++], bc));
    fl_line(x + w - 1, y - b, x, y - b, x - b, y);
    fl_color(shade_color(g[*c++], bc));
    fl_line(x - b, y, x - b, y + h, x, y + h + b);
  }
}


static void frame_round(int x, int y, int w, int h, const char *c, Fl_Color bc) {
  uchar *g = fl_gray_ramp();
  int b = strlen(c) / 4 + 1;

  if (w==h) {
    for (; b > 1; b --, x ++, y ++, w -= 2, h -= 2)
    {
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, h, 45.0, 135.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, h, 315.0, 405.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, h, 225.0, 315.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, h, 135.0, 225.0);
    }
  } else if (w>h) {
    int d = h/2;
    for (; b > 1; d--, b --, x ++, y ++, w -= 2, h -= 2)
    {
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, h, h, 90.0, 135.0);
      fl_xyline(x+d, y, x+w-d);
      fl_arc(x+w-h, y, h, h, 45.0, 90.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x+w-h, y, h, h, 315.0, 405.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x+w-h, y, h, h, 270.0, 315.0);
      fl_xyline(x+d, y+h-1, x+w-d);
      fl_arc(x, y, h, h, 225.0, 270.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, h, h, 135.0, 225.0);
    }
  } else if (w<h) {
    int d = w/2;
    for (; b > 1; d--, b --, x ++, y ++, w -= 2, h -= 2)
    {
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, w, 45.0, 135.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y, w, w, 0.0, 45.0);
      fl_yxline(x+w-1, y+d, y+h-d);
      fl_arc(x, y+h-w, w, w, 315.0, 360.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y+h-w, w, w, 225.0, 315.0);
      fl_color(shade_color(g[*c++], bc));
      fl_arc(x, y+h-w, w, w, 180.0, 225.0);
      fl_yxline(x, y+d, y+h-d);
      fl_arc(x, y, w, w, 135.0, 180.0);
    }
  }
}


static void shade_rect(int x, int y, int w, int h, const char *c, Fl_Color bc) {
  uchar		*g = fl_gray_ramp();
  int		i, j;
  int		clen = strlen(c) - 1;
  int		chalf = clen / 2;
  int		cstep = 1;

  if (h < (w * 2)) {
    // Horizontal shading...
    if (clen >= h) cstep = 2;

    for (i = 0, j = 0; j < chalf; i ++, j += cstep) {
      // Draw the top line and points...
      fl_color(shade_color(g[c[i]], bc));
      fl_xyline(x + 1, y + i, x + w - 2);

      fl_color(shade_color(g[c[i] - 2], bc));
      fl_point(x, y + i + 1);
      fl_point(x + w - 1, y + i + 1);

      // Draw the bottom line and points...
      fl_color(shade_color(g[c[clen - i]], bc));
      fl_xyline(x + 1, y + h - i, x + w - 2);

      fl_color(shade_color(g[c[clen - i] - 2], bc));
      fl_point(x, y + h - i);
      fl_point(x + w - 1, y + h - i);
    }

    // Draw the interior and sides...
    i = chalf / cstep;

    fl_color(shade_color(g[c[chalf]], bc));
    fl_rectf(x + 1, y + i, w - 2, h - 2 * i + 1);

    fl_color(shade_color(g[c[chalf] - 2], bc));
    fl_yxline(x, y + i, y + h - i);
    fl_yxline(x + w - 1, y + i, y + h - i);
  } else {
    // Vertical shading...
    if (clen >= w) cstep = 2;

    for (i = 0, j = 0; j < chalf; i ++, j += cstep) {
      // Draw the left line and points...
      fl_color(shade_color(g[c[i]], bc));
      fl_yxline(x + i, y + 1, y + h - 1);

      fl_color(shade_color(g[c[i] - 2], bc));
      fl_point(x + i + 1, y);
      fl_point(x + i + 1, y + h);

      // Draw the right line and points...
      fl_color(shade_color(g[c[clen - i]], bc));
      fl_yxline(x + w - 1 - i, y + 1, y + h - 1);

      fl_color(shade_color(g[c[clen - i] - 2], bc));
      fl_point(x + w - 2 - i, y);
      fl_point(x + w - 2 - i, y + h);
    }

    // Draw the interior, top, and bottom...
    i = chalf / cstep;

    fl_color(shade_color(g[c[chalf]], bc));
    fl_rectf(x + i, y + 1, w - 2 * i, h - 1);

    fl_color(shade_color(g[c[chalf] - 2], bc));
    fl_xyline(x + i, y, x + w - i);
    fl_xyline(x + i, y + h, x + w - i);
  }
}

static void shade_round(int x, int y, int w, int h, const char *c, Fl_Color bc) {
  uchar		*g = fl_gray_ramp();
  int		i;
  int		clen = strlen(c) - 1;
  int		chalf = clen / 2;

  if (w>h) {
    int d = h/2;
    const int na = 8;
    for (i=0; i<chalf; i++, d--, x++, y++, w-=2, h-=2)
    {
      fl_color(shade_color(g[c[i]], bc));
      fl_pie(x, y, h, h, 90.0, 135.0+i*na);
      fl_xyline(x+d, y, x+w-d);
      fl_pie(x+w-h, y, h, h, 45.0+i*na, 90.0);
      fl_color(shade_color(g[c[i] - 2], bc));
      fl_pie(x+w-h, y, h, h, 315.0+i*na, 405.0+i*na);
      fl_color(shade_color(g[c[clen - i]], bc));
      fl_pie(x+w-h, y, h, h, 270.0, 315.0+i*na);
      fl_xyline(x+d, y+h-1, x+w-d);
      fl_pie(x, y, h, h, 225.0+i*na, 270.0);
      fl_color(shade_color(g[c[clen - i] - 2], bc));
      fl_pie(x, y, h, h, 135.0+i*na, 225.0+i*na);
    }
    fl_color(shade_color(g[c[chalf]], bc));
    fl_rectf(x+d, y, w-h+1, h+1);
    fl_pie(x, y, h, h, 90.0, 270.0);
    fl_pie(x+w-h, y, h, h, 270.0, 90.0);
  } else {
    int d = w/2;
    const int na = 8;
    for (i=0; i<chalf; i++, d--, x++, y++, w-=2, h-=2)
    {
      fl_color(shade_color(g[c[i]], bc));
      fl_pie(x, y, w, w, 45.0+i*na, 135.0+i*na);
      fl_color(shade_color(g[c[i] - 2], bc));
      fl_pie(x, y, w, w, 0.0, 45.0+i*na);
      fl_yxline(x+w-1, y+d, y+h-d);
      fl_pie(x, y+h-w, w, w, 315.0+i*na, 360.0);
      fl_color(shade_color(g[c[clen - i]], bc));
      fl_pie(x, y+h-w, w, w, 225.0+i*na, 315.0+i*na);
      fl_color(shade_color(g[c[clen - i] - 2], bc));
      fl_pie(x, y+h-w, w, w, 180.0, 225.0+i*na);
      fl_yxline(x, y+d, y+h-d);
      fl_pie(x, y, w, w, 135.0+i*na, 180.0);
    }
    fl_color(shade_color(g[c[chalf]], bc));
    fl_rectf(x, y+d, w+1, h-w+1);
    fl_pie(x, y, w, w, 0.0, 180.0);
    fl_pie(x, y+h-w, w, w, 180.0, 360.0);
  }
}


static void up_frame(int x, int y, int w, int h, Fl_Color c) {
  frame_rect(x, y, w, h - 1, "KLDIIJLM", c);
}


static void narrow_thin_box(int x, int y, int w, int h, Fl_Color c) {
  if (h<=0 || w<=0) return;
  uchar *g = fl_gray_ramp();
  fl_color(shade_color(g['R'], c));
  fl_rectf(x+1, y+1, w-2, h-2);
  fl_color(shade_color(g['I'], c));
  if (w > 1) {
    fl_xyline(x+1, y, x+w-2);
    fl_xyline(x+1, y+h-1, x+w-2);
  }
  if (h > 1) {
    fl_yxline(x, y+1, y+h-2);
    fl_yxline(x+w-1, y+1, y+h-2);
  }
}


static void thin_up_box(int x, int y, int w, int h, Fl_Color c) {
#ifdef USE_OLD_PLASTIC_BOX
  shade_rect(x + 2, y + 2, w - 4, h - 5, "RVQNOPQRSTUVWVQ", c);
  up_frame(x, y, w, h, c);
#else
  if (w > 4 && h > 4) {
    shade_rect(x + 1, y + 1, w - 2, h - 3, "RQOQSUWQ", c);
    frame_rect(x, y, w, h - 1, "IJLM", c);
  } else {
    narrow_thin_box(x, y, w, h, c);
  }
#endif // USE_OLD_PLASTIC_BOX
}


static void up_box(int x, int y, int w, int h, Fl_Color c) {
#ifdef USE_OLD_PLASTIC_BOX
  shade_rect(x + 2, y + 2, w - 4, h - 5, "RVQNOPQRSTUVWVQ", c);
  up_frame(x, y, w, h, c);
#else
  if (w > 8 && h > 8) {
    shade_rect(x + 1, y + 1, w - 2, h - 3, "RVQNOPQRSTUVWVQ", c);
    frame_rect(x, y, w, h - 1, "IJLM", c);
  } else {
    thin_up_box(x, y, w, h, c);
  }
#endif // USE_OLD_PLASTIC_BOX
}


static void up_round(int x, int y, int w, int h, Fl_Color c) {
  shade_round(x, y, w, h, "RVQNOPQRSTUVWVQ", c);
  frame_round(x, y, w, h, "IJLM", c);
}


static void down_frame(int x, int y, int w, int h, Fl_Color c) {
  frame_rect(x, y, w, h - 1, "LLLLTTRR", c);
}


static void down_box(int x, int y, int w, int h, Fl_Color c) {
  if (w > 6 && h > 6) {
    shade_rect(x + 2, y + 2, w - 4, h - 5, "STUVWWWVT", c);
    down_frame(x, y, w, h, c);
  }
  else {
    narrow_thin_box(x, y, w, h, c);
  }
}


static void down_round(int x, int y, int w, int h, Fl_Color c) {
  shade_round(x, y, w, h, "STUVWWWVT", c);
  frame_round(x, y, w, h, "IJLM", c);
}


extern void fl_internal_boxtype(Fl_Boxtype, Fl_Box_Draw_F*);


Fl_Boxtype fl_define_FL_PLASTIC_UP_BOX() {
  fl_internal_boxtype(_FL_PLASTIC_UP_BOX, up_box);
  fl_internal_boxtype(_FL_PLASTIC_DOWN_BOX, down_box);
  fl_internal_boxtype(_FL_PLASTIC_UP_FRAME, up_frame);
  fl_internal_boxtype(_FL_PLASTIC_DOWN_FRAME, down_frame);
  fl_internal_boxtype(_FL_PLASTIC_THIN_UP_BOX, thin_up_box);
  fl_internal_boxtype(_FL_PLASTIC_THIN_DOWN_BOX, down_box);
  fl_internal_boxtype(_FL_PLASTIC_ROUND_UP_BOX, up_round);
  fl_internal_boxtype(_FL_PLASTIC_ROUND_DOWN_BOX, down_round);

  return _FL_PLASTIC_UP_BOX;
}


//
// End of "$Id: fl_plastic.cxx 5190 2006-06-09 16:16:34Z mike $".
//
