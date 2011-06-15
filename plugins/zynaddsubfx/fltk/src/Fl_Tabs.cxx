//
// "$Id: Fl_Tabs.cxx 8658 2011-05-12 15:53:59Z manolo $"
//
// Tab widget for the Fast Light Tool Kit (FLTK).
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


// This is the "file card tabs" interface to allow you to put lots and lots
// of buttons and switches in a panel, as popularized by many toolkits.

// Each child widget is a card, and its label() is printed on the card tab.
// Clicking the tab makes that card visible.

#include <stdio.h>
#include <FL/Fl.H>
#include <FL/Fl_Tabs.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Tooltip.H>

#define BORDER 2
#define EXTRASPACE 10
#define SELECTION_BORDER 5

// Return the left edges of each tab (plus a fake left edge for a tab
// past the right-hand one).  These positions are actually of the left
// edge of the slope.  They are either separated by the correct distance
// or by EXTRASPACE or by zero.
// These positions are updated in the private arrays tab_pos[] and
// tab_width[], resp.. If needed, these arrays are (re)allocated.
// Return value is the index of the selected item.

int Fl_Tabs::tab_positions() {
  int nc = children();
  if (nc != tab_count) {
    clear_tab_positions();
    if (nc) {
      tab_pos   = (int*)malloc((nc+1)*sizeof(int));
      tab_width = (int*)malloc((nc+1)*sizeof(int));
    }
    tab_count = nc;
  }
  if (nc == 0) return 0;
  int selected = 0;
  Fl_Widget*const* a = array();
  int i;
  char prev_draw_shortcut = fl_draw_shortcut;
  fl_draw_shortcut = 1;

  tab_pos[0] = Fl::box_dx(box());
  for (i=0; i<nc; i++) {
    Fl_Widget* o = *a++;
    if (o->visible()) selected = i;

    int wt = 0; int ht = 0;
    o->measure_label(wt,ht);

    tab_width[i] = wt + EXTRASPACE;
    tab_pos[i+1] = tab_pos[i] + tab_width[i] + BORDER;
  }
  fl_draw_shortcut = prev_draw_shortcut;

  int r = w();
  if (tab_pos[i] <= r) return selected;
  // uh oh, they are too big:
  // pack them against right edge:
  tab_pos[i] = r;
  for (i = nc; i--;) {
    int l = r-tab_width[i];
    if (tab_pos[i+1] < l) l = tab_pos[i+1];
    if (tab_pos[i] <= l) break;
    tab_pos[i] = l;
    r -= EXTRASPACE;
  }
  // pack them against left edge and truncate width if they still don't fit:
  for (i = 0; i<nc; i++) {
    if (tab_pos[i] >= i*EXTRASPACE) break;
    tab_pos[i] = i*EXTRASPACE;
    int W = w()-1-EXTRASPACE*(children()-i) - tab_pos[i];
    if (tab_width[i] > W) tab_width[i] = W;
  }
  // adjust edges according to visiblity:
  for (i = nc; i > selected; i--) {
    tab_pos[i] = tab_pos[i-1] + tab_width[i-1];
  }
  return selected;
}

// Returns space (height) in pixels needed for tabs. Negative to put them on the bottom.
// Returns full height, if children() = 0.
int Fl_Tabs::tab_height() {
  if (children() == 0) return h();
  int H = h();
  int H2 = y();
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o->y() < y()+H) H = o->y()-y();
    if (o->y()+o->h() > H2) H2 = o->y()+o->h();
  }
  H2 = y()+h()-H2;
  if (H2 > H) return (H2 <= 0) ? 0 : -H2;
  else return (H <= 0) ? 0 : H;
}

// This is used for event handling (clicks) and by fluid to pick tabs.
// Returns 0, if children() = 0, or if the event is outside of the tabs area.
Fl_Widget *Fl_Tabs::which(int event_x, int event_y) {
  if (children() == 0) return 0;
  int H = tab_height();
  if (H < 0) {
    if (event_y > y()+h() || event_y < y()+h()+H) return 0;
  } else {
    if (event_y > y()+H || event_y < y()) return 0;
  }
  if (event_x < x()) return 0;
  Fl_Widget *ret = 0L;
  int nc = children();
  tab_positions();
  for (int i=0; i<nc; i++) {
    if (event_x < x()+tab_pos[i+1]) {
      ret = child(i);
      break;
    }
  }
  return ret;
}

void Fl_Tabs::redraw_tabs()
{
  int H = tab_height();
  if (H >= 0) {
    H += Fl::box_dy(box());
    damage(FL_DAMAGE_SCROLL, x(), y(), w(), H);
  } else {
    H = Fl::box_dy(box()) - H;
    damage(FL_DAMAGE_SCROLL, x(), y() + h() - H, w(), H);
  }
}

int Fl_Tabs::handle(int event) {

  Fl_Widget *o;
  int i;

  switch (event) {

  case FL_PUSH: {
    int H = tab_height();
    if (H >= 0) {
      if (Fl::event_y() > y()+H) return Fl_Group::handle(event);
    } else {
      if (Fl::event_y() < y()+h()+H) return Fl_Group::handle(event);
    }}
    /* FALLTHROUGH */
  case FL_DRAG:
  case FL_RELEASE:
    o = which(Fl::event_x(), Fl::event_y());
    if (event == FL_RELEASE) {
      push(0);
      if (o && Fl::visible_focus() && Fl::focus()!=this) { 
        Fl::focus(this);
        redraw_tabs();
      }
      if (o && value(o)) {
        Fl_Widget_Tracker wp(o);
        set_changed();
	do_callback();
	if (wp.deleted()) return 1;
      }
      Fl_Tooltip::current(o);
    } else {
      push(o);
    }
    return 1;
  case FL_MOVE: {
    int ret = Fl_Group::handle(event);
    Fl_Widget *o = Fl_Tooltip::current(), *n = o;
    int H = tab_height();
    if ( (H>=0) && (Fl::event_y()>y()+H) )
      return ret;
    else if ( (H<0) && (Fl::event_y() < y()+h()+H) )
      return ret;
    else { 
      n = which(Fl::event_x(), Fl::event_y());
      if (!n) n = this;
    }
    if (n!=o)
      Fl_Tooltip::enter(n);
    return ret; }
  case FL_FOCUS:
  case FL_UNFOCUS:
    if (!Fl::visible_focus()) return Fl_Group::handle(event);
    if (Fl::event() == FL_RELEASE ||
	Fl::event() == FL_SHORTCUT ||
	Fl::event() == FL_KEYBOARD ||
	Fl::event() == FL_FOCUS ||
	Fl::event() == FL_UNFOCUS) {
      redraw_tabs();
      if (Fl::event() == FL_FOCUS) return Fl_Group::handle(event);
      if (Fl::event() == FL_UNFOCUS) return 0;
      else return 1;
    } else return Fl_Group::handle(event);
  case FL_KEYBOARD:
    switch (Fl::event_key()) {
      case FL_Left:
        if (child(0)->visible()) return 0;
	for (i = 1; i < children(); i ++)
	  if (child(i)->visible()) break;
	value(child(i - 1));
	set_changed();
	do_callback();
        return 1;
      case FL_Right:
        if (child(children() - 1)->visible()) return 0;
	for (i = 0; i < children(); i ++)
	  if (child(i)->visible()) break;
	value(child(i + 1));
	set_changed();
	do_callback();
        return 1;
      case FL_Down:
        redraw();
        return Fl_Group::handle(FL_FOCUS);
      default:
        break;
    }
    return Fl_Group::handle(event);
  case FL_SHORTCUT:
    for (i = 0; i < children(); ++i) {
      Fl_Widget *c = child(i);
      if (c->test_shortcut(c->label())) {
        char sc = !c->visible();
        value(c);
        if (sc) set_changed();
        do_callback();
        return 1;
      }
    }
    return Fl_Group::handle(event);
  case FL_SHOW:
    value(); // update visibilities and fall through
  default:
    return Fl_Group::handle(event);

  }
}

int Fl_Tabs::push(Fl_Widget *o) {
  if (push_ == o) return 0;
  if ( (push_ && !push_->visible()) || (o && !o->visible()) )
    redraw_tabs();
  push_ = o;
  return 1;
}

/**  
   Gets the currently visible widget/tab.
   The value() is the first visible child (or the last child if none
   are visible) and this also hides any other children.
   This allows the tabs to be deleted, moved to other groups, and
   show()/hide() called without it screwing up.
*/
Fl_Widget* Fl_Tabs::value() {
  Fl_Widget* v = 0;
  Fl_Widget*const* a = array();
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (v) o->hide();
    else if (o->visible()) v = o;
    else if (!i) {o->show(); v = o;}
  }
  return v;
}

/**
  Sets the widget to become the current visible widget/tab.
  Setting the value hides all other children, and makes this one
  visible, if it is really a child.
*/
int Fl_Tabs::value(Fl_Widget *newvalue) {
  Fl_Widget*const* a = array();
  int ret = 0;
  for (int i=children(); i--;) {
    Fl_Widget* o = *a++;
    if (o == newvalue) {
      if (!o->visible()) ret = 1;
      o->show();
    } else {
      o->hide();
    }
  }
  return ret;
}

enum {LEFT, RIGHT, SELECTED};

void Fl_Tabs::draw() {
  Fl_Widget *v = value();
  int H = tab_height();

  if (damage() & FL_DAMAGE_ALL) { // redraw the entire thing:
    Fl_Color c = v ? v->color() : color();

    draw_box(box(), x(), y()+(H>=0?H:0), w(), h()-(H>=0?H:-H), c);

    if (selection_color() != c) {
      // Draw the top or bottom SELECTION_BORDER lines of the tab pane in the
      // selection color so that the user knows which tab is selected...
      int clip_y = (H >= 0) ? y() + H : y() + h() + H - SELECTION_BORDER;
      fl_push_clip(x(), clip_y, w(), SELECTION_BORDER);
      draw_box(box(), x(), clip_y, w(), SELECTION_BORDER, selection_color());
      fl_pop_clip();
    }
    if (v) draw_child(*v);
  } else { // redraw the child
    if (v) update_child(*v);
  }
  if (damage() & (FL_DAMAGE_SCROLL|FL_DAMAGE_ALL)) {
    int nc = children();
    int selected = tab_positions();
    int i;
    Fl_Widget*const* a = array();
    for (i=0; i<selected; i++)
      draw_tab(x()+tab_pos[i], x()+tab_pos[i+1],
               tab_width[i], H, a[i], LEFT);
    for (i=nc-1; i > selected; i--)
      draw_tab(x()+tab_pos[i], x()+tab_pos[i+1],
               tab_width[i], H, a[i], RIGHT);
    if (v) {
      i = selected;
      draw_tab(x()+tab_pos[i], x()+tab_pos[i+1],
               tab_width[i], H, a[i], SELECTED);
    }
  }
}

void Fl_Tabs::draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int what) {
  int sel = (what == SELECTED);
  int dh = Fl::box_dh(box());
  int dy = Fl::box_dy(box());
  char prev_draw_shortcut = fl_draw_shortcut;
  fl_draw_shortcut = 1;

  Fl_Boxtype bt = (o==push_ &&!sel) ? fl_down(box()) : box();

  // compute offsets to make selected tab look bigger
  int yofs = sel ? 0 : BORDER;

  if ((x2 < x1+W) && what == RIGHT) x1 = x2 - W;

  if (H >= 0) {
    if (sel) fl_push_clip(x1, y(), x2 - x1, H + dh - dy);
    else fl_push_clip(x1, y(), x2 - x1, H);

    H += dh;

    Fl_Color c = sel ? selection_color() : o->selection_color();

    draw_box(bt, x1, y() + yofs, W, H + 10 - yofs, c);

    // Save the previous label color
    Fl_Color oc = o->labelcolor();

    // Draw the label using the current color...
    o->labelcolor(sel ? labelcolor() : o->labelcolor());    
    o->draw_label(x1, y() + yofs, W, H - yofs, FL_ALIGN_CENTER);

    // Restore the original label color...
    o->labelcolor(oc);

    if (Fl::focus() == this && o->visible())
      draw_focus(box(), x1, y(), W, H);

    fl_pop_clip();
  } else {
    H = -H;

    if (sel) fl_push_clip(x1, y() + h() - H - dy, x2 - x1, H + dy);
    else fl_push_clip(x1, y() + h() - H, x2 - x1, H);

    H += dh;

    Fl_Color c = sel ? selection_color() : o->selection_color();

    draw_box(bt, x1, y() + h() - H - 10, W, H + 10 - yofs, c);

    // Save the previous label color
    Fl_Color oc = o->labelcolor();

    // Draw the label using the current color...
    o->labelcolor(sel ? labelcolor() : o->labelcolor());
    o->draw_label(x1, y() + h() - H, W, H - yofs, FL_ALIGN_CENTER);

    // Restore the original label color...
    o->labelcolor(oc);

    if (Fl::focus() == this && o->visible())
      draw_focus(box(), x1, y() + h() - H, W, H);

    fl_pop_clip();
  }
  fl_draw_shortcut = prev_draw_shortcut;
}

/**
    Creates a new Fl_Tabs widget using the given position, size,
    and label string. The default boxtype is FL_THIN_UP_BOX.

    Use add(Fl_Widget*) to add each child, which are usually
    Fl_Group widgets. The children should be sized to stay
    away from the top or bottom edge of the Fl_Tabs widget,
    which is where the tabs will be drawn.

    All children of Fl_Tabs should have the same size and exactly fit on top of 
    each other. They should only leave space above or below where that tabs will 
    go, but not on the sides. If the first child of Fl_Tabs is set to 
    "resizable()", the riders will not resize when the tabs are resized.

    The destructor <I>also deletes all the children</I>. This
    allows a whole tree to be deleted at once, without having to
    keep a pointer to all the children in the user code. A kludge
    has been done so the Fl_Tabs and all of its children
    can be automatic (local) variables, but you must declare the
    Fl_Tabs widget <I>first</I> so that it is destroyed last.
*/
Fl_Tabs::Fl_Tabs(int X,int Y,int W, int H, const char *l) :
  Fl_Group(X,Y,W,H,l)
{
  box(FL_THIN_UP_BOX);
  push_ = 0;
  tab_pos = 0;
  tab_width = 0;
  tab_count = 0;
}

Fl_Tabs::~Fl_Tabs() {
  clear_tab_positions();
}

/**
    Returns the position and size available to be used by its children.

    If there isn't any child yet the \p tabh parameter will be used to
    calculate the return values. This assumes that the children's labelsize
    is the same as the Fl_Tabs' labelsize and adds a small border.

    If there are already children, the values of child(0) are returned, and
    \p tabh is ignored.

    \note Children should always use the same positions and sizes.

    \p tabh can be one of
    \li    0: calculate label size, tabs on top
    \li   -1: calculate label size, tabs on bottom
    \li >  0: use given \p tabh value, tabs on top (height = tabh)
    \li < -1: use given \p tabh value, tabs on bottom (height = -tabh)

    \param[in]	tabh		position and optional height of tabs (see above)
    \param[out]	rx,ry,rw,rh	(x,y,w,h) of client area for children

    \since	FLTK 1.3.0
*/
void Fl_Tabs::client_area(int &rx, int &ry, int &rw, int &rh, int tabh) {

  if (children()) {			// use existing values

    rx = child(0)->x();
    ry = child(0)->y();
    rw = child(0)->w();
    rh = child(0)->h();

  } else {				// calculate values

    int y_offset;
    int label_height = fl_height(labelfont(), labelsize()) + BORDER*2;

    if (tabh == 0)			// use default (at top)
      y_offset = label_height;
    else if (tabh == -1)	 	// use default (at bottom)
      y_offset = -label_height;
    else
      y_offset = tabh;			// user given value

    rx = x();
    rw = w();

    if (y_offset >= 0) {		// labels at top
      ry = y() + y_offset;
      rh = h() - y_offset;
    } else {				// labels at bottom
      ry = y();
      rh = h() + y_offset;
    }
  }
}

void Fl_Tabs::clear_tab_positions() {
  if (tab_pos) {
    free(tab_pos);
    tab_pos = 0;
  }
  if (tab_width){
    free(tab_width);
    tab_width = 0;
  }
}

//
// End of "$Id: Fl_Tabs.cxx 8658 2011-05-12 15:53:59Z manolo $".
//
