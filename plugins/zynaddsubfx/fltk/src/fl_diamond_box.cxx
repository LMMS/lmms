//
// "$Id: fl_diamond_box.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Diamond box code for the Fast Light Tool Kit (FLTK).
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

// Box drawing code for an obscure box type.
// These box types are in seperate files so they are not linked
// in if not used.

// The diamond box draws best if the area is square!

#include <FL/Fl.H>
#include <FL/fl_draw.H>

extern uchar* fl_gray_ramp();

static void fl_diamond_up_box(int x,int y,int w,int h,Fl_Color bgcolor) {
  w &= -2;
  h &= -2;
  int x1 = x+w/2;
  int y1 = y+h/2;
  fl_color(bgcolor); fl_polygon(x+3, y1, x1,y+3, x+w-3,y1, x1,y+h-3);
  uchar *g = fl_gray_ramp();
  fl_color(g['W']); fl_line(x+1, y1, x1, y+1, x+w-1, y1);
  fl_color(g['U']); fl_line(x+2, y1, x1, y+2, x+w-2, y1);
  fl_color(g['S']); fl_line(x+3, y1, x1, y+3, x+w-3, y1);
  fl_color(g['P']); fl_line(x+3, y1, x1, y+h-3, x+w-3, y1);
  fl_color(g['N']); fl_line(x+2, y1, x1, y+h-2, x+w-2, y1);
  fl_color(g['H']); fl_line(x+1, y1, x1, y+h-1, x+w-1, y1);
  fl_color(g['A']); fl_loop(x, y1, x1, y, x+w, y1, x1, y+h);
}

static void fl_diamond_down_box(int x,int y,int w,int h,Fl_Color bgcolor) {
  w &= -2;
  h &= -2;
  int x1 = x+w/2;
  int y1 = y+h/2;
  uchar *g = fl_gray_ramp();
  fl_color(g['P']); fl_line(x+0, y1, x1, y+0, x+w-0, y1);
  fl_color(g['N']); fl_line(x+1, y1, x1, y+1, x+w-1, y1);
  fl_color(g['H']); fl_line(x+2, y1, x1, y+2, x+w-2, y1);
  fl_color(g['W']); fl_line(x+2, y1, x1, y+h-2, x+w-2, y1);
  fl_color(g['U']); fl_line(x+1, y1, x1, y+h-1, x+w-1, y1);
  fl_color(g['S']); fl_line(x+0, y1, x1, y+h-0, x+w-0, y1);
  fl_color(bgcolor); fl_polygon(x+3, y1, x1,y+3, x+w-3,y1, x1,y+h-3);
  fl_color(g['A']); fl_loop(x+3, y1, x1, y+3, x+w-3, y1, x1, y+h-3);
}

extern void fl_internal_boxtype(Fl_Boxtype, Fl_Box_Draw_F*);
Fl_Boxtype fl_define_FL_DIAMOND_BOX() {
  fl_internal_boxtype(_FL_DIAMOND_DOWN_BOX, fl_diamond_down_box);
  fl_internal_boxtype(_FL_DIAMOND_UP_BOX,fl_diamond_up_box);
  return _FL_DIAMOND_UP_BOX;
}

//
// End of "$Id: fl_diamond_box.cxx 5190 2006-06-09 16:16:34Z mike $".
//
