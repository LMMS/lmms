//
// "$Id: Fl_Browser_.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Base Browser widget class for the Fast Light Tool Kit (FLTK).
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

#define DISPLAY_SEARCH_BOTH_WAYS_AT_ONCE

#include <stdio.h>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Browser_.H>
#include <FL/fl_draw.H>


// This is the base class for browsers.  To be useful it must be
// subclassed and several virtual functions defined.  The
// Forms-compatible browser and the file chooser's browser are
// subclassed off of this.

// Yes, I know this should be a template...

// This has been designed so that the subclass has complete control
// over the storage of the data, although because next() and prev()
// functions are used to index, it works best as a linked list or as a
// large block of characters in which the line breaks must be searched
// for.

// A great deal of work has been done so that the "height" of a data
// object does not need to be determined until it is drawn.  This was
// done for the file chooser, because the height requires doing stat()
// to see if the file is a directory, which can be annoyingly slow
// over the network.

/* redraw bits:
   1 = redraw children (the scrollbar)
   2 = redraw one or two items
   4 = redraw all items
*/

static void scrollbar_callback(Fl_Widget* s, void*) {
  ((Fl_Browser_*)(s->parent()))->position(int(((Fl_Scrollbar*)s)->value()));
}

static void hscrollbar_callback(Fl_Widget* s, void*) {
  ((Fl_Browser_*)(s->parent()))->hposition(int(((Fl_Scrollbar*)s)->value()));
}

// return where to draw the actual box:
/**
  Returns the bounding box for the interior of the list's display window, inside
  the scrollbars.
  \param[out] X,Y,W,H The returned bounding box.\n
                      (The original contents of these parameters are overwritten)
*/
void Fl_Browser_::bbox(int& X, int& Y, int& W, int& H) const {
  int scrollsize = scrollbar_size_ ? scrollbar_size_ : Fl::scrollbar_size();
  Fl_Boxtype b = box() ? box() : FL_DOWN_BOX;
  X = x()+Fl::box_dx(b);
  Y = y()+Fl::box_dy(b);
  W = w()-Fl::box_dw(b);
  H = h()-Fl::box_dh(b);
  if (scrollbar.visible()) {
    W -= scrollsize;
    if (scrollbar.align() & FL_ALIGN_LEFT) X += scrollsize;
  }
  if (W < 0) W = 0;
  if (hscrollbar.visible()) {
    H -= scrollsize;
    if (scrollbar.align() & FL_ALIGN_TOP) Y += scrollsize;
  }
  if (H < 0) H = 0;
}

/**
  This method returns the X position of the left edge of the list area
  after adjusting for the scrollbar and border, if any.
  \returns The X position of the left edge of the list, in pixels.
  \see Fl_Browser_::bbox()
*/
int Fl_Browser_::leftedge() const {
  int X, Y, W, H; bbox(X, Y, W, H);
  return X;
}

// The scrollbars may be moved again by draw(), since each one's size
// depends on whether the other is visible or not.  This skips over
// Fl_Group::resize since it moves the scrollbars uselessly.
/**
  Repositions and/or resizes the browser.
  \param[in] X,Y,W,H The new position and size for the browser, in pixels.
*/
void Fl_Browser_::resize(int X, int Y, int W, int H) {
  int scrollsize = scrollbar_size_ ? scrollbar_size_ : Fl::scrollbar_size();
  Fl_Widget::resize(X, Y, W, H);
  // move the scrollbars so they can respond to events:
  bbox(X,Y,W,H);
  scrollbar.resize(
	scrollbar.align()&FL_ALIGN_LEFT ? X-scrollsize : X+W,
	Y, scrollsize, H);
  hscrollbar.resize(
	X, scrollbar.align()&FL_ALIGN_TOP ? Y-scrollsize : Y+H,
	W, scrollsize);
}

// Cause minimal update to redraw the given item:
/**
  This method should be called when the contents of \p item has changed,
  but not its height.
  \param[in] item The item that needs to be redrawn.
  \see redraw_lines(), redraw_line()
*/
void Fl_Browser_::redraw_line(void* item) {
  if (!redraw1 || redraw1 == item) {redraw1 = item; damage(FL_DAMAGE_EXPOSE);}
  else if (!redraw2 || redraw2 == item) {redraw2 = item; damage(FL_DAMAGE_EXPOSE);}
  else damage(FL_DAMAGE_SCROLL);
}

// Figure out top() based on position():
void Fl_Browser_::update_top() {
  if (!top_) top_ = item_first();
  if (position_ != real_position_) {
    void* l;
    int ly;
    int yy = position_;
    // start from either head or current position, whichever is closer:
    if (!top_ || yy <= (real_position_/2)) {
      l = item_first();
      ly = 0;
    } else {
      l = top_;
      ly = real_position_-offset_;
    }
    if (!l) {
      top_ = 0;
      offset_ = 0;
      real_position_ = 0;
    } else {
      int hh = item_quick_height(l);
      // step through list until we find line containing this point:
      while (ly > yy) {
	void* l1 = item_prev(l);
	if (!l1) {ly = 0; break;} // hit the top
	l  = l1;
	hh = item_quick_height(l);
	ly -= hh;
      }
      while ((ly+hh) <= yy) {
	void* l1 = item_next(l);
	if (!l1) {yy = ly+hh-1; break;}
	l = l1;
	ly += hh;
	hh = item_quick_height(l);
      }
      // top item must *really* be visible, use slow height:
      for (;;) {
	hh = item_height(l);
	if ((ly+hh) > yy) break; // it is big enough to see
	// go up to top of previous item:
	void* l1 = item_prev(l);
	if (!l1) {ly = yy = 0; break;} // hit the top
	l = l1; yy = position_ = ly = ly-item_quick_height(l);
      }
      // use it:
      top_ = l;
      offset_ = yy-ly;
      real_position_ = yy;
    }
    damage(FL_DAMAGE_SCROLL);
  }
}

// Change position(), top() will update when update_top() is called
// (probably by draw() or handle()):
/**
  Sets the vertical scroll position of the list to pixel position \p pos.
  The position is how many pixels of the list are scrolled off the top edge
  of the screen. Example: A position of '3' scrolls the top three pixels of
  the list off the top edge of the screen.
  \param[in] pos The vertical position (in pixels) to scroll the browser to.
  \see position(), hposition()
*/
void Fl_Browser_::position(int pos) {
  if (pos < 0) pos = 0;
  if (pos == position_) return;
  position_ = pos;
  if (pos != real_position_) redraw_lines();
}

/**
  Sets the horizontal scroll position of the list to pixel position \p pos.
  The position is how many pixels of the list are scrolled off the left edge
  of the screen. Example: A position of '18' scrolls the left 18 pixels of the list
  off the left edge of the screen.
  \param[in] pos The horizontal position (in pixels) to scroll the browser to.
  \see position(), hposition()
*/
void Fl_Browser_::hposition(int pos) {
  if (pos < 0) pos = 0;
  if (pos == hposition_) return;
  hposition_ = pos;
  if (pos != real_hposition_) redraw_lines();
}

// Tell whether item is currently displayed:
/**
  Returns non-zero if \p item has been scrolled to a position where it is being displayed.
  Checks to see if the item's vertical position is within the top and bottom
  edges of the display window. This does NOT take into account the hide()/show()
  status of the widget or item.
  \param[in] item The item to check
  \returns 1 if visible, 0 if not visible.
  \see display(), displayed()
*/
int Fl_Browser_::displayed(void* item) const {
  int X, Y, W, H; bbox(X, Y, W, H);
  int yy = H+offset_;
  for (void* l = top_; l && yy > 0; l = item_next(l)) {
    if (l == item) return 1;
    yy -= item_height(l);
  }
  return 0;
}

// Ensure this item is displayed:
// Messy because we have no idea if it is before top or after bottom:
/**
  Displays the \p item, scrolling the list as necessary.
  \param[in] item The item to be displayed.
  \see display(), displayed()
*/
void Fl_Browser_::display(void* item) {

  // First special case - want to display first item in the list?
  update_top();
  if (item == item_first()) {position(0); return;}

  int X, Y, W, H, Yp; bbox(X, Y, W, H);
  void* l = top_;
  Y = Yp = -offset_;
  int h1;

  // 2nd special case - want to display item already displayed at top of browser?
  if (l == item) {position(real_position_+Y); return;} // scroll up a bit

  // 3rd special case - want to display item just above top of browser?
  void* lp = item_prev(l);
  if (lp == item) {position(real_position_+Y-item_quick_height(lp)); return;}

#ifdef DISPLAY_SEARCH_BOTH_WAYS_AT_ONCE
  // search for item.  We search both up and down the list at the same time,
  // this evens up the execution time for the two cases - the old way was
  // much slower for going up than for going down.
  while (l || lp) {
    if (l) {
      h1 = item_quick_height(l);
      if (l == item) {
	if (Y <= H) { // it is visible or right at bottom
	  Y = Y+h1-H; // find where bottom edge is
	  if (Y > 0) position(real_position_+Y); // scroll down a bit
	} else {
	  position(real_position_+Y-(H-h1)/2); // center it
	}
	return;
      }
      Y += h1;
      l = item_next(l);
    }
    if (lp) {
      h1 = item_quick_height(lp);
      Yp -= h1;
      if (lp == item) {
	if ((Yp + h1) >= 0) position(real_position_+Yp);
	else position(real_position_+Yp-(H-h1)/2);
	return;
      }
      lp = item_prev(lp);
    }
  }
#else
  // Old version went forwards and then backwards:
  // search forward for it:
  l = top_;
  for (; l; l = item_next(l)) {
    h1 = item_quick_height(l);
    if (l == item) {
      if (Y <= H) { // it is visible or right at bottom
	Y = Y+h1-H; // find where bottom edge is
	if (Y > 0) position(real_position_+Y); // scroll down a bit
      } else {
	position(real_position_+Y-(H-h1)/2); // center it
      }
      return;
    }
    Y += h1;
  }
  // search backward for it, if found center it:
  l = lp;
  Y = -offset_;
  for (; l; l = item_prev(l)) {
    h1 = item_quick_height(l);
    Y -= h1;
    if (l == item) {
      if ((Y + h1) >= 0) position(real_position_+Y);
      else position(real_position_+Y-(H-h1)/2);
      return;
    }
  }
#endif
}

// redraw, has side effect of updating top and setting scrollbar:
/**
  Draws the list within the normal widget bounding box.
*/
void Fl_Browser_::draw() {
  int drawsquare = 0;
  update_top();
  int full_width_ = full_width();
  int full_height_ = full_height();
  int X, Y, W, H; bbox(X, Y, W, H);
  int dont_repeat = 0;
J1:
  if (damage() & FL_DAMAGE_ALL) { // redraw the box if full redraw
    Fl_Boxtype b = box() ? box() : FL_DOWN_BOX;
    draw_box(b, x(), y(), w(), h(), color());
    drawsquare = 1;
  }
  // see if scrollbar needs to be switched on/off:
  if ((has_scrollbar_ & VERTICAL) && (
	(has_scrollbar_ & ALWAYS_ON) || position_ || full_height_ > H)) {
    if (!scrollbar.visible()) {
      scrollbar.set_visible();
      drawsquare = 1;
      bbox(X, Y, W, H);
    }
  } else {
    top_ = item_first(); real_position_ = offset_ = 0;
    if (scrollbar.visible()) {
      scrollbar.clear_visible();
      clear_damage((uchar)(damage()|FL_DAMAGE_SCROLL));
    }
  }

  if ((has_scrollbar_ & HORIZONTAL) && (
	(has_scrollbar_ & ALWAYS_ON) || hposition_ || full_width_ > W)) {
    if (!hscrollbar.visible()) {
      hscrollbar.set_visible();
      drawsquare = 1;
      bbox(X, Y, W, H);
    }
  } else {
    real_hposition_ = 0;
    if (hscrollbar.visible()) {
      hscrollbar.clear_visible();
      clear_damage((uchar)(damage()|FL_DAMAGE_SCROLL));
    }
  }

  // Check the vertical scrollbar again, just in case it needs to be drawn
  // because the horizontal one is drawn.  There should be a cleaner way
  // to do this besides copying the same code...
  if ((has_scrollbar_ & VERTICAL) && (
	(has_scrollbar_ & ALWAYS_ON) || position_ || full_height_ > H)) {
    if (!scrollbar.visible()) {
      scrollbar.set_visible();
      drawsquare = 1;
      bbox(X, Y, W, H);
    }
  } else {
    top_ = item_first(); real_position_ = offset_ = 0;
    if (scrollbar.visible()) {
      scrollbar.clear_visible();
      clear_damage((uchar)(damage()|FL_DAMAGE_SCROLL));
    }
  }

  bbox(X, Y, W, H);

  fl_push_clip(X, Y, W, H);
  // for each line, draw it if full redraw or scrolled.  Erase background
  // if not a full redraw or if it is selected:
  void* l = top();
  int yy = -offset_;
  for (; l && yy < H; l = item_next(l)) {
    int hh = item_height(l);
    if (hh <= 0) continue;
    if ((damage()&(FL_DAMAGE_SCROLL|FL_DAMAGE_ALL)) || l == redraw1 || l == redraw2) {
      if (item_selected(l)) {
	fl_color(active_r() ? selection_color() : fl_inactive(selection_color()));
	fl_rectf(X, yy+Y, W, hh);
      } else if (!(damage()&FL_DAMAGE_ALL)) {
	fl_push_clip(X, yy+Y, W, hh);
	draw_box(box() ? box() : FL_DOWN_BOX, x(), y(), w(), h(), color());
	fl_pop_clip();
      }
      item_draw(l, X-hposition_, yy+Y, W+hposition_, hh);
      if (l == selection_ && Fl::focus() == this) {
	draw_box(FL_BORDER_FRAME, X, yy+Y, W, hh, color());
	draw_focus(FL_NO_BOX, X, yy+Y, W+1, hh+1);
      }
      int ww = item_width(l);
      if (ww > max_width) {max_width = ww; max_width_item = l;}
    }
    yy += hh;
  }
  // erase the area below last line:
  if (!(damage()&FL_DAMAGE_ALL) && yy < H) {
    fl_push_clip(X, yy+Y, W, H-yy);
    draw_box(box() ? box() : FL_DOWN_BOX, x(), y(), w(), h(), color());
    fl_pop_clip();
  }
  fl_pop_clip();
  redraw1 = redraw2 = 0;

  if (!dont_repeat) {
    dont_repeat = 1;
    // see if changes to full_height caused by calls to slow_height
    // caused scrollbar state to change, in which case we have to redraw:
    full_height_ = full_height();
    full_width_ = full_width();
    if ((has_scrollbar_ & VERTICAL) &&
	((has_scrollbar_ & ALWAYS_ON) || position_ || full_height_>H)) {
      if (!scrollbar.visible()) { damage(FL_DAMAGE_ALL); goto J1; }
    } else {
      if (scrollbar.visible()) { damage(FL_DAMAGE_ALL); goto J1; }
    }
    if ((has_scrollbar_ & HORIZONTAL) &&
	((has_scrollbar_ & ALWAYS_ON) || hposition_ || full_width_>W)) {
      if (!hscrollbar.visible()) { damage(FL_DAMAGE_ALL); goto J1; }
    } else {
      if (hscrollbar.visible()) { damage(FL_DAMAGE_ALL); goto J1; }
    }
  }

  // update the scrollbars and redraw them:
  int scrollsize = scrollbar_size_ ? scrollbar_size_ : Fl::scrollbar_size();
  int dy = top_ ? item_quick_height(top_) : 0; if (dy < 10) dy = 10;
  if (scrollbar.visible()) {
    scrollbar.damage_resize(
	scrollbar.align()&FL_ALIGN_LEFT ? X-scrollsize : X+W,
	Y, scrollsize, H);
    scrollbar.value(position_, H, 0, full_height_);
    scrollbar.linesize(dy);
    if (drawsquare) draw_child(scrollbar);
    else update_child(scrollbar);
  }
  if (hscrollbar.visible()) {
    hscrollbar.damage_resize(
	X, scrollbar.align()&FL_ALIGN_TOP ? Y-scrollsize : Y+H,
	W, scrollsize);
    hscrollbar.value(hposition_, W, 0, full_width_);
    hscrollbar.linesize(dy);
    if (drawsquare) draw_child(hscrollbar);
    else update_child(hscrollbar);
  }

  // draw that little square between the scrollbars:
  if (drawsquare && scrollbar.visible() && hscrollbar.visible()) {
    fl_color(parent()->color());
    fl_rectf(scrollbar.x(), hscrollbar.y(), scrollsize, scrollsize);
  }

  real_hposition_ = hposition_;
}

// Quick way to delete and reset everything:
/**
  This method should be called when the list data is completely replaced
  or cleared. It informs the Fl_Browser_ widget that any cached
  information it has concerning the items is invalid.
  This method does not clear the list, it just handles the follow up
  bookkeeping after the list has been cleared.
*/
void Fl_Browser_::new_list() {
  top_ = 0;
  position_ = real_position_ = 0;
  hposition_ = real_hposition_ = 0;
  selection_ = 0;
  offset_ = 0;
  max_width = 0;
  max_width_item = 0;
  redraw_lines();
}

// Tell it that this item is going away, and that this must remove
// all pointers to it:
/**
  This method should be used when \p item is being deleted from the list.
  It allows the Fl_Browser_ to discard any cached data it has on the item.
  This method does not actually delete the item, but handles the follow up
  bookkeeping after the item has just been deleted.
  \param[in] item The item being deleted.
*/
void Fl_Browser_::deleting(void* item) {
  if (displayed(item)) {
    redraw_lines();
    if (item == top_) {
      real_position_ -= offset_;
      offset_ = 0;
      top_ = item_next(item);
      if (!top_) top_ = item_prev(item);
    }
  } else {
    // we don't know where this item is, recalculate top...
    real_position_ = 0;
    offset_ = 0;
    top_ = 0;
  }
  if (item == selection_) selection_ = 0;
  if (item == max_width_item) {max_width_item = 0; max_width = 0;}
}

/**
  This method should be used when item \p a is being replaced by item \p b.
  It allows the Fl_Browser_ to update its cache data as needed,
  schedules a redraw for the item being changed, and tries to maintain the selection.
  This method does not actually replace the item, but handles the follow up
  bookkeeping after the item has just been replaced.
  \param[in] a Item being replaced
  \param[in] b Item to replace 'a'
*/
void Fl_Browser_::replacing(void* a, void* b) {
  redraw_line(a);
  if (a == selection_) selection_ = b;
  if (a == top_) top_ = b;
  if (a == max_width_item) {max_width_item = 0; max_width = 0;}
}

/**
  This method should be used when two items \p a and \p b are being swapped.
  It allows the Fl_Browser_ to update its cache data as needed,
  schedules a redraw for the two items, and tries to maintain the current selection.
  This method does not actually swap items, but handles the follow up
  bookkeeping after items have been swapped.
  \param[in] a,b Items being swapped.
*/
void Fl_Browser_::swapping(void* a, void* b) {
  redraw_line(a);
  redraw_line(b);
  if (a == selection_) selection_ = b;
  else if (b == selection_) selection_ = a;
  if (a == top_) top_ = b;
  else if (b == top_) top_ = a;
}

/**
  This method should be used when an item is in the process of
  being inserted into the list.
  It allows the Fl_Browser_ to update its cache data as needed,
  scheduling a redraw for the affected lines.
  This method does not actually insert items, but handles the 
  follow up bookkeeping after items have been inserted.
  \param[in] a The starting item position
  \param[in] b The new item being inserted
*/
void Fl_Browser_::inserting(void* a, void* b) {
  if (displayed(a)) redraw_lines();
  if (a == top_) top_ = b;
}

/**
  This method returns the item under mouse y position \p ypos.
  NULL is returned if no item is displayed at that position.
  \param[in] ypos The y position (eg. Fl::event_y()) to find an item under.
  \returns The item, or NULL if not found
*/
void* Fl_Browser_::find_item(int ypos) {
  update_top();
  int X, Y, W, H; bbox(X, Y, W, H);
  int yy = Y-offset_;
  for (void *l = top_; l; l = item_next(l)) {
    int hh = item_height(l); if (hh <= 0) continue;
    yy += hh;
    if (ypos <= yy || yy>=(Y+H)) return l;
  }
  return 0;
}

/**
  Sets the selection state of \p item to \p val,
  and returns 1 if the state changed or 0 if it did not.
  
  If \p docallbacks is non-zero, select tries to call
  the callback function for the widget.

  \param[in] item The item whose selection state is to be changed
  \param[in] val The new selection state (1=select, 0=de-select)
  \param[in] docallbacks If 1, invokes widget callback if item changed.\n
                         If 0, doesn't do callback (default).
  \returns 1 if state was changed, 0 if not.
*/
int Fl_Browser_::select(void* item, int val, int docallbacks) {
  if (type() == FL_MULTI_BROWSER) {
    if (selection_ != item) {
      if (selection_) redraw_line(selection_);
      selection_ = item;
      redraw_line(item);
    }
    if ((!val)==(!item_selected(item))) return 0;
    item_select(item, val);
    redraw_line(item);
  } else {
    if (val && selection_ == item) return 0;
    if (!val && selection_ != item) return 0;
    if (selection_) {
      item_select(selection_, 0);
      redraw_line(selection_);
      selection_ = 0;
    }
    if (val) {
      item_select(item, 1);
      selection_ = item;
      redraw_line(item);
      display(item);
    }
  }	    
  if (docallbacks) {
    set_changed();
    do_callback();
  }
  return 1;
}

/**
  Deselects all items in the list and returns 1 if the state changed
  or 0 if it did not.
  
  If the optional \p docallbacks parameter is non-zero, deselect tries
  to call the callback function for the widget.

  \param[in] docallbacks If 1, invokes widget callback if item changed.\n
                         If 0, doesn't do callback (default).
*/
int Fl_Browser_::deselect(int docallbacks) {
  if (type() == FL_MULTI_BROWSER) {
    int change = 0;
    for (void* p = item_first(); p; p = item_next(p))
      change |= select(p, 0, docallbacks);
    return change;
  } else {
    if (!selection_) return 0;
    item_select(selection_, 0);
    redraw_line(selection_);
    selection_ = 0;
    return 1;
  }
}

/**
  Selects \p item and returns 1 if the state changed or 0 if it did not.
  Any other items in the list are deselected.
  \param[in] item The \p item to select.
  \param[in] docallbacks If 1, invokes widget callback if item changed.\n
                         If 0, doesn't do callback (default).
*/
int Fl_Browser_::select_only(void* item, int docallbacks) {
  if (!item) return deselect(docallbacks);
  int change = 0;
  Fl_Widget_Tracker wp(this);
  if (type() == FL_MULTI_BROWSER) {
    for (void* p = item_first(); p; p = item_next(p)) {
      if (p != item) change |= select(p, 0, docallbacks);
      if (wp.deleted()) return change;
    }
  }
  change |= select(item, 1, docallbacks);
  if (wp.deleted()) return change;
  display(item);
  return change;
}

/**
  Handles the \p event within the normal widget bounding box.
  \param[in] event The event to process.
  \returns 1 if event was processed, 0 if not.
*/
int Fl_Browser_::handle(int event) {

  // NOTE:
  // We use Fl_Widget_Tracker to test if the user has deleted
  // this widget in a callback. Callbacks can be called by:
  //  - do_callback()
  //  - select()
  //  - select_only()
  //  - deselect()
  // Thus we must test wp.deleted() after each of these calls,
  // unless we return directly after one of these.
  // If wp.deleted() is true, we return 1 because we used the event.

  Fl_Widget_Tracker wp(this);

  // must do shortcuts first or the scrollbar will get them...
  if (event == FL_ENTER || event == FL_LEAVE) return 1;
  if (event == FL_KEYBOARD && type() >= FL_HOLD_BROWSER) {
    void* l1 = selection_;
    void* l = l1; if (!l) l = top_; if (!l) l = item_first();
    if (l) {
      if (type()==FL_HOLD_BROWSER) {
        switch (Fl::event_key()) {
        case FL_Down:
          while ((l = item_next(l)))
            if (item_height(l)>0) {select_only(l, when()); break;}
            return 1;
        case FL_Up:
          while ((l = item_prev(l))) {
	    if (item_height(l)>0) {
	      select_only(l, when());
	      break; // no need to test wp (return 1)
	    }
	  }
          return 1;
        } 
      } else  {
        switch (Fl::event_key()) {
        case FL_Enter:
        case FL_KP_Enter:
          select_only(l, when() & ~FL_WHEN_ENTER_KEY);
	  if (wp.deleted()) return 1;
	  if (when() & FL_WHEN_ENTER_KEY) {
	    set_changed();
	    do_callback();
	  }
          return 1;
        case ' ':
          selection_ = l;
          select(l, !item_selected(l), when() & ~FL_WHEN_ENTER_KEY);
          return 1;
        case FL_Down:
          while ((l = item_next(l))) {
            if (Fl::event_state(FL_SHIFT|FL_CTRL))
              select(l, l1 ? item_selected(l1) : 1, when());
	    if (wp.deleted()) return 1;
            if (item_height(l)>0) goto J1;
          }
          return 1;
        case FL_Up:
          while ((l = item_prev(l))) {
            if (Fl::event_state(FL_SHIFT|FL_CTRL))
              select(l, l1 ? item_selected(l1) : 1, when());
	    if (wp.deleted()) return 1;
            if (item_height(l)>0) goto J1;
          }
          return 1;
J1:
          if (selection_) redraw_line(selection_);
          selection_ = l; redraw_line(l);
          display(l);
          return 1;
        }
      }
    }
  }
  
  if (Fl_Group::handle(event)) return 1;
  if (wp.deleted()) return 1;

  int X, Y, W, H; bbox(X, Y, W, H);
  int my;
// NOTE:
// instead of:
//     change = select_only(find_item(my), when() & FL_WHEN_CHANGED)
// we use the construct:
//     change = select_only(find_item(my), 0);
//     if (change && (when() & FL_WHEN_CHANGED)) {
//	 set_changed();
//       do_callback();
//     }
// See str #834
// The first form calls the callback *before* setting change.
// The callback may execute an Fl::wait(), resulting in another
// call of Fl_Browser_::handle() for the same widget. The sequence
// of events can be an FL_PUSH followed by an FL_RELEASE.
// This second call of Fl_Browser_::handle() may result in a -
// somewhat unexpected - second concurrent invocation of the callback.

  static char change;
  static char whichway;
  static int py;
  switch (event) {
  case FL_PUSH:
    if (!Fl::event_inside(X, Y, W, H)) return 0;
    if (Fl::visible_focus()) {
      Fl::focus(this);
      redraw();
    }
    my = py = Fl::event_y();
    change = 0;
    if (type() == FL_NORMAL_BROWSER || !top_)
      ;
    else if (type() != FL_MULTI_BROWSER) {
      change = select_only(find_item(my), 0);
      if (wp.deleted()) return 1;
      if (change && (when() & FL_WHEN_CHANGED)) {
	set_changed();
	do_callback();
	if (wp.deleted()) return 1;
      }
    } else {
      void* l = find_item(my);
      whichway = 1;
      if (Fl::event_state(FL_CTRL)) { // toggle selection:
      TOGGLE:
	if (l) {
	  whichway = !item_selected(l);
	  change = select(l, whichway, 0);
	  if (wp.deleted()) return 1;
	  if (change && (when() & FL_WHEN_CHANGED)) {
	    set_changed();
	    do_callback();
	    if (wp.deleted()) return 1;
	  }
	}
      } else if (Fl::event_state(FL_SHIFT)) { // extend selection:
	if (l == selection_) goto TOGGLE;
	// state of previous selection determines new value:
	whichway = l ? !item_selected(l) : 1;
	// see which of the new item or previous selection is earlier,
	// by searching from the previous forward for this one:
	int down;
	if (!l) down = 1;
	else {for (void* m = selection_; ; m = item_next(m)) {
	  if (m == l) {down = 1; break;}
	  if (!m) {down = 0; break;}
	}}
	if (down) {
	  for (void* m = selection_; m != l; m = item_next(m)) {
	    select(m, whichway, when() & FL_WHEN_CHANGED);
	    if (wp.deleted()) return 1;
	  }
	} else {
	  void* e = selection_;
	  for (void* m = item_next(l); m; m = item_next(m)) {
	    select(m, whichway, when() & FL_WHEN_CHANGED);
	    if (wp.deleted()) return 1;
	    if (m == e) break;
	  }
	}
	// do the clicked item last so the select box is around it:
	change = 1;
	if (l) select(l, whichway, when() & FL_WHEN_CHANGED);
	if (wp.deleted()) return 1;
      } else { // select only this item
	change = select_only(l, 0);
	if (wp.deleted()) return 1;
	if (change && (when() & FL_WHEN_CHANGED)) {
	  set_changed();
	  do_callback();
	  if (wp.deleted()) return 1;
	}
      }
    }
    return 1;
  case FL_DRAG:
    // do the scrolling first:
    my = Fl::event_y();
    if (my < Y && my < py) {
      int p = real_position_+my-Y;
      if (p<0) p = 0;
      position(p);
    } else if (my > (Y+H) && my > py) {
      int p = real_position_+my-(Y+H);
      int hh = full_height()-H; if (p > hh) p = hh;
      if (p<0) p = 0;
      position(p);
    }
    if (type() == FL_NORMAL_BROWSER || !top_)
      ;
    else if (type() == FL_MULTI_BROWSER) {
      void* l = find_item(my);
      void* t; void* b; // this will be the range to change
      if (my > py) { // go down
	t = selection_ ? item_next(selection_) : 0;
	b = l ? item_next(l) : 0;
      } else {	// go up
	t = l;
	b = selection_;
      }
      for (; t && t != b; t = item_next(t)) {
	char change_t;
	change_t = select(t, whichway, 0);
	if (wp.deleted()) return 1;
	change |= change_t;
	if (change_t && (when() & FL_WHEN_CHANGED)) {
	  set_changed();
	  do_callback();
	  if (wp.deleted()) return 1;
	}
      }
      if (l) selection_ = l;
    } else {
      void* l1 = selection_;
      void* l =
	(Fl::event_x()<x() || Fl::event_x()>x()+w()) ? selection_ :
	find_item(my);
      change = (l != l1);
      select_only(l, when() & FL_WHEN_CHANGED);
      if (wp.deleted()) return 1;
    }
    py = my;
    return 1;
  case FL_RELEASE:
    if (type() == FL_SELECT_BROWSER) {
      void* t = selection_;
      deselect();
      if (wp.deleted()) return 1;
      selection_ = t;
    }
    if (change) {
      set_changed();
      if (when() & FL_WHEN_RELEASE) do_callback();
    } else {
      if (when() & FL_WHEN_NOT_CHANGED) do_callback();
    }
    if (wp.deleted()) return 1;

    // double click calls the callback: (like Enter Key)
    if (Fl::event_clicks() && (when() & FL_WHEN_ENTER_KEY)) {
      set_changed();
      do_callback();
    }
    return 1;
  case FL_FOCUS:
  case FL_UNFOCUS:
    if (type() >= FL_HOLD_BROWSER && Fl::visible_focus()) {
      redraw();
      return 1;
    } else return 0;
  }

  return 0;
}

/**
  The constructor makes an empty browser.
  \param[in] X,Y,W,H position and size.
  \param[in] L The label string, may be NULL.
*/
Fl_Browser_::Fl_Browser_(int X, int Y, int W, int H, const char* L)
  : Fl_Group(X, Y, W, H, L),
    scrollbar(0, 0, 0, 0, 0), // they will be resized by draw()
    hscrollbar(0, 0, 0, 0, 0)
{
  box(FL_NO_BOX);
  align(FL_ALIGN_BOTTOM);
  position_ = real_position_ = 0;
  hposition_ = real_hposition_ = 0;
  offset_ = 0;
  top_ = 0;
  when(FL_WHEN_RELEASE_ALWAYS);
  selection_ = 0;
  color(FL_BACKGROUND2_COLOR, FL_SELECTION_COLOR);
  scrollbar.callback(scrollbar_callback);
//scrollbar.align(FL_ALIGN_LEFT|FL_ALIGN_BOTTOM); // back compatibility?
  hscrollbar.callback(hscrollbar_callback);
  hscrollbar.type(FL_HORIZONTAL);
  textfont_ = FL_HELVETICA;
  textsize_ = FL_NORMAL_SIZE;
  textcolor_ = FL_FOREGROUND_COLOR;
  has_scrollbar_ = BOTH;
  max_width = 0;
  max_width_item = 0;
  scrollbar_size_ = 0;
  redraw1 = redraw2 = 0;
  end();
}

/**
  Sort the items in the browser based on \p flags.
  item_swap(void*, void*) and item_text(void*) must be implemented for this call.
  \param[in] flags FL_SORT_ASCENDING -- sort in ascending order\n
                   FL_SORT_DESCENDING -- sort in descending order\n
		   Values other than the above will cause undefined behavior\n
		   Other flags may appear in the future.
  \todo Add a flag to ignore case
*/
void Fl_Browser_::sort(int flags) {
  //
  // Simple bubble sort - pure lazyness on my side.
  //
  int i, j, n = -1, desc = ((flags&FL_SORT_DESCENDING)==FL_SORT_DESCENDING);
  void *a =item_first(), *b, *c;
  if (!a) return;
  while (a) {
    a = item_next(a);
    n++;
  }
  for (i=n; i>0; i--) {
    char swapped = 0;
    a = item_first();
    b = item_next(a);
    for (j=0; j<i; j++) {
      const char *ta = item_text(a);
      const char *tb = item_text(b);
      c = item_next(b);
      if (desc) {
        if (strcmp(ta, tb)<0) {
          item_swap(a, b);
          swapped = 1;
        }
      } else {
        if (strcmp(ta, tb)>0) {
          item_swap(a, b);
          swapped = 1;
        }
      }
      if (!c) break;
      b = c; a = item_prev(b);
    }
    if (!swapped)
      break;
  }
}

// Default versions of some of the virtual functions:

/**
  This method may be provided by the subclass to return the height of the
  \p item, in pixels.  
  Allow for two additional pixels for the list selection box.
  This method differs from item_height in that it is only called for 
  selection and scrolling operations. 
  The default implementation calls item_height.
  \param[in] item The item whose height to return.
  \returns The height, in pixels.
*/
int Fl_Browser_::item_quick_height(void* item) const {
  return item_height(item);
}

/**
  This method may be provided to return the average height of all items
  to be used for scrolling. 
  The default implementation uses the height of the first item.
  \returns The average height of items, in pixels.
*/
int Fl_Browser_::incr_height() const {
  return item_quick_height(item_first());
}

/**
  This method may be provided by the subclass to indicate the full height
  of the item list, in pixels. 
  The default implementation computes the full height from the item heights. 
  Includes the items that are scrolled off screen.
  \returns The height of the entire list, in pixels.
*/
int Fl_Browser_::full_height() const {
  int t = 0;
  for (void* p = item_first(); p; p = item_next(p))
    t += item_quick_height(p);
  return t;
}

/**
  This method may be provided by the subclass to indicate the full width
  of the item list, in pixels. 
  The default implementation computes the full width from the item widths.
  \returns The maximum width of all the items, in pixels.
*/
int Fl_Browser_::full_width() const {
  return max_width;
}

/**
  This method must be implemented by the subclass if it supports 
  multiple selections; sets the selection state to \p val for the \p item.
  Sets the selection state for \p item, where optional \p val is 1 (select, the default)
  or 0 (de-select).
  \param[in] item The item to be selected
  \param[in] val The optional selection state; 1=select, 0=de-select.\n
                 The default is to select the item (1).
*/
void Fl_Browser_::item_select(void *item, int val) {}

/**
  This method must be implemented by the subclass if it supports
  multiple selections; returns the selection state for \p item.
  The method should return 1 if \p item is selected, or 0 otherwise.
  \param[in] item The item to test.
*/
int Fl_Browser_::item_selected(void* item) const { return item==selection_ ? 1 : 0; }

//
// End of "$Id: Fl_Browser_.cxx 7903 2010-11-28 21:06:39Z matt $".
//
