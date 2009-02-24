//
// "$Id: Fl_Multi_Label.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Multi-label widget for the Fast Light Tool Kit (FLTK).
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

// Allows two labels to be used on a widget (by having one of them
// be one of these it allows an infinte number!)

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Multi_Label.H>

static void multi_labeltype(
    const Fl_Label* o, int x, int y, int w, int h, Fl_Align a)
{
  Fl_Multi_Label* b = (Fl_Multi_Label*)(o->value);
  Fl_Label local = *o;
  local.value = b->labela;
  local.type = b->typea;
  int W = w; int H = h; local.measure(W, H);
  local.draw(x,y,w,h,a);
  if (a & FL_ALIGN_BOTTOM) h -= H;
  else if (a & FL_ALIGN_TOP) {y += H; h -= H;}
  else if (a & FL_ALIGN_RIGHT) w -= W;
  else if (a & FL_ALIGN_LEFT) {x += W; w -= W;}
  else {int d = (h+H)/2; y += d; h -= d;}
  local.value = b->labelb;
  local.type = b->typeb;
  local.draw(x,y,w,h,a);
}

// measurement is only correct for left-to-right appending...
static void multi_measure(const Fl_Label* o, int& w, int& h) {
  Fl_Multi_Label* b = (Fl_Multi_Label*)(o->value);
  Fl_Label local = *o;
  local.value = b->labela;
  local.type = b->typea;
  local.measure(w,h);
  local.value = b->labelb;
  local.type = b->typeb;
  int W = 0; int H = 0; local.measure(W,H);
  w += W; if (H>h) h = H;
}

void Fl_Multi_Label::label(Fl_Widget* o) {
  Fl::set_labeltype(_FL_MULTI_LABEL, multi_labeltype, multi_measure);
  o->label(_FL_MULTI_LABEL, (const char*)this);
}

void Fl_Multi_Label::label(Fl_Menu_Item* o) {
  Fl::set_labeltype(_FL_MULTI_LABEL, multi_labeltype, multi_measure);
  o->label(_FL_MULTI_LABEL, (const char*)this);
}

//
// End of "$Id: Fl_Multi_Label.cxx 5190 2006-06-09 16:16:34Z mike $".
//
