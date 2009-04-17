//
// "$Id: Fl_Scroll.cxx 5547 2006-11-16 23:17:13Z mike $"
//
// Scroll widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Tiled_Image.H>
#include <FL/Fl_Scroll.H>
#include <FL/fl_draw.H>

// Clear all but the scrollbars...
void Fl_Scroll::clear() {
  for (int i=children() - 1; i >= 0; i --) {
    Fl_Widget* o = child(i);
    if (o != &hscrollbar && o != &scrollbar) {
      remove(o);
      delete o;
    }
  }
}

// Insure the scrollbars are the last children:
void Fl_Scroll::fix_scrollbar_order() {
  Fl_Widget** a = (Fl_Widget**)array();
  if (a[children()-1] != &scrollbar) {
    int i,j; for (i = j = 0; j < children(); j++)
      if (a[j] != &hscrollbar && a[j] != &scrollbar) a[i++] = a[j];
    a[i++] = &hscrollbar;
    a[i++] = &scrollbar;
  }
}

void Fl_Scroll::draw_clip(void* v,int X, int Y, int W, int H) {
  fl_clip(X,Y,W,H);
  Fl_Scroll* s = (Fl_Scroll*)v;
  // erase background as needed...
  switch (s->box()) {
    case FL_NO_BOX :
    case FL_UP_FRAME :
    case FL_DOWN_FRAME :
    case FL_THIN_UP_FRAME :
    case FL_THIN_DOWN_FRAME :
    case FL_ENGRAVED_FRAME :
    case FL_EMBOSSED_FRAME :
    case FL_BORDER_FRAME :
    case _FL_SHADOW_FRAME :
    case _FL_ROUNDED_FRAME :
    case _FL_OVAL_FRAME :
    case _FL_PLASTIC_UP_FRAME :
    case _FL_PLASTIC_DOWN_FRAME :
        if (s->parent() == (Fl_Group *)s->window() && Fl::scheme_bg_) {
	  Fl::scheme_bg_->draw(X-(X%((Fl_Tiled_Image *)Fl::scheme_bg_)->image()->w()),
	                       Y-(Y%((Fl_Tiled_Image *)Fl::scheme_bg_)->image()->h()),
	                       W+((Fl_Tiled_Image *)Fl::scheme_bg_)->image()->w(),
			       H+((Fl_Tiled_Image *)Fl::scheme_bg_)->image()->h());
	  break;
        }

    default :
	fl_color(s->color());
	fl_rectf(X,Y,W,H);
	break;
  }
  Fl_Widget*const* a = s->array();
  for (int i=s->children()-2; i--;) {
    Fl_Widget& o = **a++;
    s->draw_child(o);
    s->draw_outside_label(o);
  }
  fl_pop_clip();
}

void Fl_Scroll::bbox(int& X, int& Y, int& W, int& H) {
  X = x()+Fl::box_dx(box());
  Y = y()+Fl::box_dy(box());
  W = w()-Fl::box_dw(box());
  H = h()-Fl::box_dh(box());
  if (scrollbar.visible()) {
    W -= scrollbar.w();
    if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
  }
  if (hscrollbar.visible()) {
    H -= hscrollbar.h();
    if (scrollbar.align() & FL_ALIGN_TOP) Y += hscrollbar.h();
  }
}

void Fl_Scroll::draw() {
  fix_scrollbar_order();
  int X,Y,W,H; bbox(X,Y,W,H);

  uchar d = damage();

  if (d & FL_DAMAGE_ALL) { // full redraw
    draw_box(box(),x(),y(),w(),h(),color());
    draw_clip(this, X, Y, W, H);
  } else {
    if (d & FL_DAMAGE_SCROLL) {
      // scroll the contents:
      fl_scroll(X, Y, W, H, oldx-xposition_, oldy-yposition_, draw_clip, this);

      // Erase the background as needed...
      Fl_Widget*const* a = array();
      int L, R, T, B;
      L = 999999;
      R = 0;
      T = 999999;
      B = 0;
      for (int i=children()-2; i--; a++) {
        if ((*a)->x() < L) L = (*a)->x();
	if (((*a)->x() + (*a)->w()) > R) R = (*a)->x() + (*a)->w();
        if ((*a)->y() < T) T = (*a)->y();
	if (((*a)->y() + (*a)->h()) > B) B = (*a)->y() + (*a)->h();
      }
      if (L > X) draw_clip(this, X, Y, L - X, H);
      if (R < (X + W)) draw_clip(this, R, Y, X + W - R, H);
      if (T > Y) draw_clip(this, X, Y, W, T - Y);
      if (B < (Y + H)) draw_clip(this, X, B, W, Y + H - B);
    }
    if (d & FL_DAMAGE_CHILD) { // draw damaged children
      fl_clip(X, Y, W, H);
      Fl_Widget*const* a = array();
      for (int i=children()-2; i--;) update_child(**a++);
      fl_pop_clip();
    }
  }

  // accumulate bounding box of children:
  int l = X; int r = X; int t = Y; int b = Y;
  Fl_Widget*const* a = array();
  for (int i=children()-2; i--;) {
    Fl_Object* o = *a++;
    if (o->x() < l) l = o->x();
    if (o->y() < t) t = o->y();
    if (o->x()+o->w() > r) r = o->x()+o->w();
    if (o->y()+o->h() > b) b = o->y()+o->h();
  }

  // turn the scrollbars on and off as necessary:
  // See if children would fit if we had no scrollbars...
  X = x()+Fl::box_dx(box());
  Y = y()+Fl::box_dy(box());
  W = w()-Fl::box_dw(box());
  H = h()-Fl::box_dh(box());
  int vneeded = 0;
  int hneeded = 0;
  if (type() & VERTICAL) {
    if ((type() & ALWAYS_ON) || t < Y || b > Y+H) {
      vneeded = 1;
      W -= scrollbar.w();
      if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
    }
  }
  if (type() & HORIZONTAL) {
    if ((type() & ALWAYS_ON) || l < X || r > X+W) {
      hneeded = 1;
      H -= hscrollbar.h();
      if (scrollbar.align() & FL_ALIGN_TOP) Y += hscrollbar.h();
      // recheck vertical since we added a horizontal scrollbar
      if (!vneeded && (type() & VERTICAL)) {
	if ((type() & ALWAYS_ON) || t < Y || b > Y+H) {
	  vneeded = 1;
	  W -= scrollbar.w();
	  if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollbar.w();
	}
      }
    }
  }
  // Now that we know what's needed, make it so.
  if (vneeded && !scrollbar.visible()) {
    scrollbar.set_visible();
    d = FL_DAMAGE_ALL;
  }
  else if (!vneeded && scrollbar.visible()) {
    scrollbar.clear_visible();
    draw_clip(this,
	      scrollbar.align()&FL_ALIGN_LEFT ? X : X+W-scrollbar.w(),
	      Y, scrollbar.w(), H);
    d = FL_DAMAGE_ALL;
  }
  if (hneeded && !hscrollbar.visible()) {
    hscrollbar.set_visible();
    d = FL_DAMAGE_ALL;
  }
  else if (!hneeded && hscrollbar.visible()) {
    hscrollbar.clear_visible();
    draw_clip(this,
	      X, scrollbar.align()&FL_ALIGN_TOP ? Y : Y+H-hscrollbar.h(),
	      W, hscrollbar.h());
    d = FL_DAMAGE_ALL;
  }

  scrollbar.resize(scrollbar.align()&FL_ALIGN_LEFT ? X-scrollbar.w() : X+W,
		   Y, scrollbar.w(), H);
  scrollbar.value(oldy = yposition_ = (Y-t), H, 0, b-t);

  hscrollbar.resize(X,
		    scrollbar.align()&FL_ALIGN_TOP ? Y-hscrollbar.h() : Y+H,
		    W, hscrollbar.h());
  hscrollbar.value(oldx = xposition_ = (X-l), W, 0, r-l);

  // draw the scrollbars:
  if (d & FL_DAMAGE_ALL) {
    draw_child(scrollbar);
    draw_child(hscrollbar);
    if (scrollbar.visible() && hscrollbar.visible()) {
      // fill in the little box in the corner
      fl_color(color());
      fl_rectf(scrollbar.x(), hscrollbar.y(), scrollbar.w(), hscrollbar.h());
    }
  } else {
    update_child(scrollbar);
    update_child(hscrollbar);
  }
}

void Fl_Scroll::resize(int X, int Y, int W, int H) {
  fix_scrollbar_order();
  // move all the children:
  Fl_Widget*const* a = array();
  for (int i=children()-2; i--;) {
    Fl_Object* o = *a++;
    o->position(o->x()+X-x(), o->y()+Y-y());
  }
  Fl_Widget::resize(X,Y,W,H);
}

void Fl_Scroll::position(int X, int Y) {
  int dx = xposition_-X;
  int dy = yposition_-Y;
  if (!dx && !dy) return;
  xposition_ = X;
  yposition_ = Y;
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o == &hscrollbar || o == &scrollbar) continue;
    o->position(o->x()+dx, o->y()+dy);
  }
  if (parent() == (Fl_Group *)window() && Fl::scheme_bg_) damage(FL_DAMAGE_ALL);
  else damage(FL_DAMAGE_SCROLL);
}

void Fl_Scroll::hscrollbar_cb(Fl_Widget* o, void*) {
  Fl_Scroll* s = (Fl_Scroll*)(o->parent());
  s->position(int(((Fl_Scrollbar*)o)->value()), s->yposition());
}

void Fl_Scroll::scrollbar_cb(Fl_Widget* o, void*) {
  Fl_Scroll* s = (Fl_Scroll*)(o->parent());
  s->position(s->xposition(), int(((Fl_Scrollbar*)o)->value()));
}

Fl_Scroll::Fl_Scroll(int X,int Y,int W,int H,const char* L)
  : Fl_Group(X,Y,W,H,L), 
    scrollbar(X+W-Fl::scrollbar_size(),Y,
              Fl::scrollbar_size(),H-Fl::scrollbar_size()),
    hscrollbar(X,Y+H-Fl::scrollbar_size(),
               W-Fl::scrollbar_size(),Fl::scrollbar_size()) {
  type(BOTH);
  xposition_ = 0;
  yposition_ = 0;
  hscrollbar.type(FL_HORIZONTAL);
  hscrollbar.callback(hscrollbar_cb);
  scrollbar.callback(scrollbar_cb);
}

int Fl_Scroll::handle(int event) {
  fix_scrollbar_order();
  return Fl_Group::handle(event);
}

//
// End of "$Id: Fl_Scroll.cxx 5547 2006-11-16 23:17:13Z mike $".
//
