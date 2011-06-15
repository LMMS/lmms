//
// "$Id: Fl_Group.cxx 8184 2011-01-04 18:28:01Z matt $"
//
// Group widget for the Fast Light Tool Kit (FLTK).
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

// The Fl_Group is the only defined container type in FLTK.

// Fl_Window itself is a subclass of this, and most of the event
// handling is designed so windows themselves work correctly.

#include <stdio.h>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <stdlib.h>

Fl_Group* Fl_Group::current_;

// Hack: A single child is stored in the pointer to the array, while
// multiple children are stored in an allocated array:

/**
  Returns a pointer to the array of children. <I>This pointer is only
  valid until the next time a child is added or removed.</I>
*/
Fl_Widget*const* Fl_Group::array() const {
  return children_ <= 1 ? (Fl_Widget**)(&array_) : array_;
}

/**
  Searches the child array for the widget and returns the index. Returns children()
  if the widget is NULL or not found.
*/
int Fl_Group::find(const Fl_Widget* o) const {
  Fl_Widget*const* a = array();
  int i; for (i=0; i < children_; i++) if (*a++ == o) break;
  return i;
}

// Metrowerks CodeWarrior and others can't export the static
// class member: current_, so these methods can't be inlined...

/**
  Sets the current group so you can build the widget
  tree by just constructing the widgets.

  begin() is automatically called by the constructor for Fl_Group (and thus for
  Fl_Window as well). begin() <I>is exactly the same as</I> current(this).
  <I>Don't forget to end() the group or window!</I>
*/
void Fl_Group::begin() {current_ = this;}

/**
  <I>Exactly the same as</I> current(this->parent()). Any new widgets
  added to the widget tree will be added to the parent of the group.
*/
void Fl_Group::end() {current_ = parent();}

/**
  Returns the currently active group.
  
  The Fl_Widget constructor automatically does current()->add(widget) if this
  is not null. To prevent new widgets from being added to a group, call
  Fl_Group::current(0).
*/
Fl_Group *Fl_Group::current() {return current_;}

/**
  Sets the current group.
  \see Fl_Group::current() 
*/
void Fl_Group::current(Fl_Group *g) {current_ = g;}

extern Fl_Widget* fl_oldfocus; // set by Fl::focus

// For back-compatibility, we must adjust all events sent to child
// windows so they are relative to that window.

static int send(Fl_Widget* o, int event) {
  if (o->type() < FL_WINDOW) return o->handle(event);
  switch ( event )
  {
  case FL_DND_ENTER: /* FALLTHROUGH */
  case FL_DND_DRAG:
    // figure out correct type of event:
    event = (o->contains(Fl::belowmouse())) ? FL_DND_DRAG : FL_DND_ENTER;
  }
  int save_x = Fl::e_x; Fl::e_x -= o->x();
  int save_y = Fl::e_y; Fl::e_y -= o->y();
  int ret = o->handle(event);
  Fl::e_y = save_y;
  Fl::e_x = save_x;
  switch ( event )
  {
  case FL_ENTER: /* FALLTHROUGH */
  case FL_DND_ENTER:
    // Successful completion of FL_ENTER means the widget is now the
    // belowmouse widget, but only call Fl::belowmouse if the child
    // widget did not do so:
    if (!o->contains(Fl::belowmouse())) Fl::belowmouse(o);
    break;
  }
  return ret;
}

// translate the current keystroke into up/down/left/right for navigation:
#define ctrl(x) (x^0x40)
static int navkey() {
  switch (Fl::event_key()) {
  case 0: // not an FL_KEYBOARD/FL_SHORTCUT event
    break;
  case FL_Tab:
    if (!Fl::event_state(FL_SHIFT)) return FL_Right;
    return FL_Left;
  case FL_Right:
    return FL_Right;
  case FL_Left:
    return FL_Left;
  case FL_Up:
    return FL_Up;
  case FL_Down:
    return FL_Down;
  }
  return 0;
}

int Fl_Group::handle(int event) {

  Fl_Widget*const* a = array();
  int i;
  Fl_Widget* o;

  switch (event) {

  case FL_FOCUS:
    switch (navkey()) {
    default:
      if (savedfocus_ && savedfocus_->take_focus()) return 1;
    case FL_Right:
    case FL_Down:
      for (i = children(); i--;) if ((*a++)->take_focus()) return 1;
      break;
    case FL_Left:
    case FL_Up:
      for (i = children(); i--;) if (a[i]->take_focus()) return 1;
      break;
    }
    return 0;

  case FL_UNFOCUS:
    savedfocus_ = fl_oldfocus;
    return 0;

  case FL_KEYBOARD:
    return navigation(navkey());

  case FL_SHORTCUT:
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && Fl::event_inside(o) && send(o,FL_SHORTCUT))
	return 1;
    }
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && !Fl::event_inside(o) && send(o,FL_SHORTCUT))
	return 1;
    }
    if ((Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter)) return navigation(FL_Down);
    return 0;

  case FL_ENTER:
  case FL_MOVE:
    for (i = children(); i--;) {
      o = a[i];
      if (o->visible() && Fl::event_inside(o)) {
	if (o->contains(Fl::belowmouse())) {
	  return send(o,FL_MOVE);
	} else {
	  Fl::belowmouse(o);
	  if (send(o,FL_ENTER)) return 1;
	}
      }
    }
    Fl::belowmouse(this);
    return 1;

  case FL_DND_ENTER:
  case FL_DND_DRAG:
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && Fl::event_inside(o)) {
	if (o->contains(Fl::belowmouse())) {
	  return send(o,FL_DND_DRAG);
	} else if (send(o,FL_DND_ENTER)) {
	  if (!o->contains(Fl::belowmouse())) Fl::belowmouse(o);
	  return 1;
	}
      }
    }
    Fl::belowmouse(this);
    return 0;

  case FL_PUSH:
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && Fl::event_inside(o)) {
	Fl_Widget_Tracker wp(o);
	if (send(o,FL_PUSH)) {
	  if (Fl::pushed() && wp.exists() && !o->contains(Fl::pushed())) Fl::pushed(o);
	  return 1;
	}
      }
    }
    return 0;

  case FL_RELEASE:
  case FL_DRAG:
    o = Fl::pushed();
    if (o == this) return 0;
    else if (o) send(o,event);
    else {
      for (i = children(); i--;) {
	o = a[i];
	if (o->takesevents() && Fl::event_inside(o)) {
	  if (send(o,event)) return 1;
	}
      }
    }
    return 0;

  case FL_MOUSEWHEEL:
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && Fl::event_inside(o) && send(o,FL_MOUSEWHEEL))
	return 1;
    }
    for (i = children(); i--;) {
      o = a[i];
      if (o->takesevents() && !Fl::event_inside(o) && send(o,FL_MOUSEWHEEL))
	return 1;
    }
    return 0;

  case FL_DEACTIVATE:
  case FL_ACTIVATE:
    for (i = children(); i--;) {
      o = *a++;
      if (o->active()) o->handle(event);
    }
    return 1;

  case FL_SHOW:
  case FL_HIDE:
    for (i = children(); i--;) {
      o = *a++;
      if (event == FL_HIDE && o == Fl::focus()) {
        // Give up input focus...
	int old_event = Fl::e_number;
        o->handle(Fl::e_number = FL_UNFOCUS);
	Fl::e_number = old_event;
	Fl::focus(0);
      }
      if (o->visible()) o->handle(event);
    }
    return 1;

  default:
    // For all other events, try to give to each child, starting at focus:
    for (i = 0; i < children(); i ++)
      if (Fl::focus_ == a[i]) break;

    if (i >= children()) i = 0;

    if (children()) {
      for (int j = i;;) {
        if (a[j]->takesevents()) if (send(a[j], event)) return 1;
        j++;
        if (j >= children()) j = 0;
        if (j == i) break;
      }
    }

    return 0;
  }
}

//void Fl_Group::focus(Fl_Widget *o) {Fl::focus(o); o->handle(FL_FOCUS);}

#if 0
const char *nameof(Fl_Widget *o) {
  if (!o) return "NULL";
  if (!o->label()) return "<no label>";
  return o->label();
}
#endif

// try to move the focus in response to a keystroke:
int Fl_Group::navigation(int key) {
  if (children() <= 1) return 0;
  int i;
  for (i = 0; ; i++) {
    if (i >= children_) return 0;
    if (array_[i]->contains(Fl::focus())) break;
  }
  Fl_Widget *previous = array_[i];

  for (;;) {
    switch (key) {
    case FL_Right:
    case FL_Down:
      i++;
      if (i >= children_) {
	if (parent()) return 0;
	i = 0;
      }
      break;
    case FL_Left:
    case FL_Up:
      if (i) i--;
      else {
	if (parent()) return 0;
	i = children_-1;
      }
      break;
    default:
      return 0;
    }
    Fl_Widget* o = array_[i];
    if (o == previous) return 0;
    switch (key) {
    case FL_Down:
    case FL_Up:
      // for up/down, the widgets have to overlap horizontally:
      if (o->x() >= previous->x()+previous->w() ||
	  o->x()+o->w() <= previous->x()) continue;
    }
    if (o->take_focus()) return 1;
  }
}

////////////////////////////////////////////////////////////////

Fl_Group::Fl_Group(int X,int Y,int W,int H,const char *l)
: Fl_Widget(X,Y,W,H,l) {
  align(FL_ALIGN_TOP);
  children_ = 0;
  array_ = 0;
  savedfocus_ = 0;
  resizable_ = this;
  sizes_ = 0; // this is allocated when first resize() is done
  // Subclasses may want to construct child objects as part of their
  // constructor, so make sure they are add()'d to this object.
  // But you must end() the object!
  begin();
}

/**
  Deletes all child widgets from memory recursively.

  This method differs from the remove() method in that it
  affects all child widgets and deletes them from memory.
*/
void Fl_Group::clear() {
  savedfocus_ = 0;
  resizable_ = this;
  init_sizes();

  // we must change the Fl::pushed() widget, if it is one of
  // the group's children. Otherwise fl_fix_focus() would send
  // lots of events to children that are about to be deleted
  // anyway.

  Fl_Widget *pushed = Fl::pushed();	// save pushed() widget
  if (contains(pushed)) pushed = this;	// set it to be the group, if it's a child
  Fl::pushed(this);			// for fl_fix_focus etc.

  // okay, now it is safe to destroy the children:

#define REVERSE_CHILDREN
#ifdef  REVERSE_CHILDREN
  // Reverse the order of the children. Doing this and deleting
  // always the last child is much faster than the other way around.
  if (children_ > 1) {
    Fl_Widget *temp;
    Fl_Widget **a = (Fl_Widget**)array();
    for (int i=0,j=children_-1; i<children_/2; i++,j--) {
      temp = a[i];
      a[i] = a[j];
      a[j] = temp;
    }
  }
#endif // REVERSE_CHILDREN

  while (children_) {			// delete all children
    int idx = children_-1;		// last child's index
    Fl_Widget* w = child(idx);		// last child widget
    if (w->parent()==this) {		// should always be true
      if (children_>2) {		// optimized removal
        w->parent_ = 0;			// reset child's parent
        children_--;			// update counter
      } else {				// slow removal
        remove(idx);
      }
      delete w;				// delete the child
    } else {				// should never happen
      remove(idx);			// remove it anyway
    }
  }

  if (pushed != this) Fl::pushed(pushed); // reset pushed() widget

}

/**
  The destructor <I>also deletes all the children</I>. This allows a
  whole tree to be deleted at once, without having to keep a pointer to
  all the children in the user code.
  
  It is allowed that the Fl_Group and all of its children are automatic
  (local) variables, but you must declare the Fl_Group \e first, so that
  it is destroyed last.
  
  If you add static or automatic (local) variables to an Fl_Group, then it
  is your responsibility to remove (or delete) all such static or automatic
  child widgets \e \b before destroying the group - otherwise the child
  widgets' destructors would be called twice!
*/
Fl_Group::~Fl_Group() {
  clear();
}

/**
  The widget is removed from its current group (if any) and then
  inserted into this group. It is put at index n - or at the end,
  if n >= children(). This can also be used to rearrange
  the widgets inside a group.
*/
void Fl_Group::insert(Fl_Widget &o, int index) {
  if (o.parent()) {
    Fl_Group* g = o.parent();
    int n = g->find(o);
    if (g == this) {
      if (index > n) index--;
      if (index == n) return;
    }
    g->remove(n);
  }
  o.parent_ = this;
  if (children_ == 0) { // use array pointer to point at single child
    array_ = (Fl_Widget**)&o;
  } else if (children_ == 1) { // go from 1 to 2 children
    Fl_Widget* t = (Fl_Widget*)array_;
    array_ = (Fl_Widget**)malloc(2*sizeof(Fl_Widget*));
    if (index) {array_[0] = t; array_[1] = &o;}
    else {array_[0] = &o; array_[1] = t;}
  } else {
    if (!(children_ & (children_-1))) // double number of children
      array_ = (Fl_Widget**)realloc((void*)array_,
				    2*children_*sizeof(Fl_Widget*));
    int j; for (j = children_; j > index; j--) array_[j] = array_[j-1];
    array_[j] = &o;
  }
  children_++;
  init_sizes();
}

/**
  The widget is removed from its current group (if any) and then added
  to the end of this group.
*/
void Fl_Group::add(Fl_Widget &o) {insert(o, children_);}

/**
  Removes the widget at \p index from the group but does not delete it.

  This method does nothing if \p index is out of bounds.

  This method differs from the clear() method in that it only affects
  a single widget and does not delete it from memory.
  
  \since FLTK 1.3.0
*/
void Fl_Group::remove(int index) {
  if (index < 0 || index >= children_) return;
  Fl_Widget &o = *child(index);
  if (&o == savedfocus_) savedfocus_ = 0;
  if (o.parent_ == this) {	// this should always be true
    o.parent_ = 0;
  } 

  // remove the widget from the group

  children_--;
  if (children_ == 1) { // go from 2 to 1 child
    Fl_Widget *t = array_[!index];
    free((void*)array_);
    array_ = (Fl_Widget**)t;
  } else if (children_ > 1) { // delete from array
    for (; index < children_; index++) array_[index] = array_[index+1];
  }
  init_sizes();
}

/**
  Removes a widget from the group but does not delete it.

  This method does nothing if the widget is not a child of the group.

  This method differs from the clear() method in that it only affects
  a single widget and does not delete it from memory.
  
  \note If you have the child's index anyway, use remove(int index)
  instead, because this doesn't need a child lookup in the group's
  table of children. This can be much faster, if there are lots of
  children.
*/
void Fl_Group::remove(Fl_Widget &o) {
  if (!children_) return;
  int i = find(o);
  if (i < children_) remove(i);
}

////////////////////////////////////////////////////////////////

// Rather lame kludge here, I need to detect windows and ignore the
// changes to X,Y, since all children are relative to X,Y.  That
// is why I check type():

// sizes array stores the initial positions of widgets as
// left,right,top,bottom quads.  The first quad is the group, the
// second is the resizable (clipped to the group), and the
// rest are the children.  This is a convenient order for the
// algorithm.  If you change this be sure to fix Fl_Tile which
// also uses this array!

/**
  Resets the internal array of widget sizes and positions.

  The Fl_Group widget keeps track of the original widget sizes and
  positions when resizing occurs so that if you resize a window back to its
  original size the widgets will be in the correct places. If you rearrange
  the widgets in your group, call this method to register the new arrangement
  with the Fl_Group that contains them.
  
  If you add or remove widgets, this will be done automatically.
  
  \note The internal array of widget sizes and positions will be allocated and
  filled when the next resize() occurs.
  
  \sa sizes()
*/
void Fl_Group::init_sizes() {
  delete[] sizes_; sizes_ = 0;
}

/**
  Returns the internal array of widget sizes and positions.

  If the sizes() array does not exist, it will be allocated and filled
  with the current widget sizes and positions.

  \note You should never need to use this method directly, unless you have
  special needs to rearrange the children of a Fl_Group. Fl_Tile uses
  this to rearrange its widget positions.
  
  \sa init_sizes()

  \todo Should the internal representation of the sizes() array be documented?
*/
int* Fl_Group::sizes() {
  if (!sizes_) {
    int* p = sizes_ = new int[4*(children_+2)];
    // first thing in sizes array is the group's size:
    if (type() < FL_WINDOW) {p[0] = x(); p[2] = y();} else {p[0] = p[2] = 0;}
    p[1] = p[0]+w(); p[3] = p[2]+h();
    // next is the resizable's size:
    p[4] = p[0]; // init to the group's size
    p[5] = p[1];
    p[6] = p[2];
    p[7] = p[3];
    Fl_Widget* r = resizable();
    if (r && r != this) { // then clip the resizable to it
      int t;
      t = r->x(); if (t > p[0]) p[4] = t;
      t +=r->w(); if (t < p[1]) p[5] = t;
      t = r->y(); if (t > p[2]) p[6] = t;
      t +=r->h(); if (t < p[3]) p[7] = t;
    }
    // next is all the children's sizes:
    p += 8;
    Fl_Widget*const* a = array();
    for (int i=children_; i--;) {
      Fl_Widget* o = *a++;
      *p++ = o->x();
      *p++ = o->x()+o->w();
      *p++ = o->y();
      *p++ = o->y()+o->h();
    }
  }
  return sizes_;
}

/**
  Resizes the Fl_Group widget and all of its children.

  The Fl_Group widget first resizes itself, and then it moves and resizes
  all its children according to the rules documented for
  Fl_Group::resizable(Fl_Widget*)

  \sa Fl_Group::resizable(Fl_Widget*)
  \sa Fl_Group::resizable()
  \sa Fl_Widget::resize(int,int,int,int)
*/
void Fl_Group::resize(int X, int Y, int W, int H) {

  int dx = X-x();
  int dy = Y-y();
  int dw = W-w();
  int dh = H-h();
  
  int *p = sizes(); // save initial sizes and positions

  Fl_Widget::resize(X,Y,W,H); // make new xywh values visible for children

  if (!resizable() || (dw==0 && dh==0) ) {

    if (type() < FL_WINDOW) {
      Fl_Widget*const* a = array();
      for (int i=children_; i--;) {
	Fl_Widget* o = *a++;
	o->resize(o->x()+dx, o->y()+dy, o->w(), o->h());
      }
    }

  } else if (children_) {

    // get changes in size/position from the initial size:
    dx = X - p[0];
    dw = W - (p[1]-p[0]);
    dy = Y - p[2];
    dh = H - (p[3]-p[2]);
    if (type() >= FL_WINDOW) dx = dy = 0;
    p += 4;

    // get initial size of resizable():
    int IX = *p++;
    int IR = *p++;
    int IY = *p++;
    int IB = *p++;

    Fl_Widget*const* a = array();
    for (int i=children_; i--;) {
      Fl_Widget* o = *a++;
#if 1
      int XX = *p++;
      if (XX >= IR) XX += dw;
      else if (XX > IX) XX = IX+((XX-IX)*(IR+dw-IX)+(IR-IX)/2)/(IR-IX);
      int R = *p++;
      if (R >= IR) R += dw;
      else if (R > IX) R = IX+((R-IX)*(IR+dw-IX)+(IR-IX)/2)/(IR-IX);

      int YY = *p++;
      if (YY >= IB) YY += dh;
      else if (YY > IY) YY = IY+((YY-IY)*(IB+dh-IY)+(IB-IY)/2)/(IB-IY);
      int B = *p++;
      if (B >= IB) B += dh;
      else if (B > IY) B = IY+((B-IY)*(IB+dh-IY)+(IB-IY)/2)/(IB-IY);
#else // much simpler code from Francois Ostiguy:
      int XX = *p++;
      if (XX >= IR) XX += dw;
      else if (XX > IX) XX += dw * (XX-IX)/(IR-IX);
      int R = *p++;
      if (R >= IR) R += dw;
      else if (R > IX) R = R + dw * (R-IX)/(IR-IX);

      int YY = *p++;
      if (YY >= IB) YY += dh;
      else if (YY > IY) YY = YY + dh*(YY-IY)/(IB-IY);
      int B = *p++;
      if (B >= IB) B += dh;
      else if (B > IY) B = B + dh*(B-IY)/(IB-IY);
#endif
      o->resize(XX+dx, YY+dy, R-XX, B-YY);
    }
  }
}

/**
  Draws all children of the group.

  This is useful, if you derived a widget from Fl_Group and want to draw a special
  border or background. You can call draw_children() from the derived draw() method
  after drawing the box, border, or background.
*/
void Fl_Group::draw_children() {
  Fl_Widget*const* a = array();

  if (clip_children()) {
    fl_push_clip(x() + Fl::box_dx(box()),
                 y() + Fl::box_dy(box()),
		 w() - Fl::box_dw(box()),
		 h() - Fl::box_dh(box()));
  }

  if (damage() & ~FL_DAMAGE_CHILD) { // redraw the entire thing:
    for (int i=children_; i--;) {
      Fl_Widget& o = **a++;
      draw_child(o);
      draw_outside_label(o);
    }
  } else {	// only redraw the children that need it:
    for (int i=children_; i--;) update_child(**a++);
  }

  if (clip_children()) fl_pop_clip();
}

void Fl_Group::draw() {
  if (damage() & ~FL_DAMAGE_CHILD) { // redraw the entire thing:
    draw_box();
    draw_label();
  }
  draw_children();
}

/**
  Draws a child only if it needs it.

  This draws a child widget, if it is not clipped \em and if any damage() bits
  are set. The damage bits are cleared after drawing.

  \sa Fl_Group::draw_child(Fl_Widget& widget) const
*/
void Fl_Group::update_child(Fl_Widget& widget) const {
  if (widget.damage() && widget.visible() && widget.type() < FL_WINDOW &&
      fl_not_clipped(widget.x(), widget.y(), widget.w(), widget.h())) {
    widget.draw();	
    widget.clear_damage();
  }
}

/**
  Forces a child to redraw.

  This draws a child widget, if it is not clipped.
  The damage bits are cleared after drawing.
*/
void Fl_Group::draw_child(Fl_Widget& widget) const {
  if (widget.visible() && widget.type() < FL_WINDOW &&
      fl_not_clipped(widget.x(), widget.y(), widget.w(), widget.h())) {
    widget.clear_damage(FL_DAMAGE_ALL);
    widget.draw();
    widget.clear_damage();
  }
}

extern char fl_draw_shortcut;

/** Parents normally call this to draw outside labels of child widgets. */
void Fl_Group::draw_outside_label(const Fl_Widget& widget) const {
  if (!widget.visible()) return;
  // skip any labels that are inside the widget:
  if (!(widget.align()&15) || (widget.align() & FL_ALIGN_INSIDE)) return;
  // invent a box that is outside the widget:
  Fl_Align a = widget.align();
  int X = widget.x();
  int Y = widget.y();
  int W = widget.w();
  int H = widget.h();
  int wx, wy;
  if (const_cast<Fl_Group*>(this)->as_window()) {
    wx = wy = 0;
  } else {
    wx = x(); wy = y();
  }
  if ( (a & 0x0f) == FL_ALIGN_LEFT_TOP ) {
    a = (a &~0x0f ) | FL_ALIGN_TOP_RIGHT;
    X = wx;
    W = widget.x()-X-3;
  } else if ( (a & 0x0f) == FL_ALIGN_LEFT_BOTTOM ) {
    a = (a &~0x0f ) | FL_ALIGN_BOTTOM_RIGHT; 
    X = wx;
    W = widget.x()-X-3;
  } else if ( (a & 0x0f) == FL_ALIGN_RIGHT_TOP ) {
    a = (a &~0x0f ) | FL_ALIGN_TOP_LEFT; 
    X = X+W+3;
    W = wx+this->w()-X;
  } else if ( (a & 0x0f) == FL_ALIGN_RIGHT_BOTTOM ) {
    a = (a &~0x0f ) | FL_ALIGN_BOTTOM_LEFT; 
    X = X+W+3;
    W = wx+this->w()-X;
  } else if (a & FL_ALIGN_TOP) {
    a ^= (FL_ALIGN_BOTTOM|FL_ALIGN_TOP);
    Y = wy;
    H = widget.y()-Y;
  } else if (a & FL_ALIGN_BOTTOM) {
    a ^= (FL_ALIGN_BOTTOM|FL_ALIGN_TOP);
    Y = Y+H;
    H = wy+h()-Y;
  } else if (a & FL_ALIGN_LEFT) {
    a ^= (FL_ALIGN_LEFT|FL_ALIGN_RIGHT);
    X = wx;
    W = widget.x()-X-3;
  } else if (a & FL_ALIGN_RIGHT) {
    a ^= (FL_ALIGN_LEFT|FL_ALIGN_RIGHT);
    X = X+W+3;
    W = wx+this->w()-X;
  }
  widget.draw_label(X,Y,W,H,(Fl_Align)a);
}

//
// End of "$Id: Fl_Group.cxx 8184 2011-01-04 18:28:01Z matt $".
//
