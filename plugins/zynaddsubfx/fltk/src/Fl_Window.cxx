//
// "$Id: Fl_Window.cxx 8472 2011-02-25 08:44:47Z AlbrechtS $"
//
// Window widget class for the Fast Light Tool Kit (FLTK).
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

// The Fl_Window is a window in the fltk library.
// This is the system-independent portions.  The huge amount of 
// crap you need to do to communicate with X is in Fl_x.cxx, the
// equivalent (but totally different) crap for MSWindows is in Fl_win32.cxx
#include <config.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <stdlib.h>
#include "flstring.h"

#ifdef __APPLE_QUARTZ__
#include <FL/fl_draw.H>
#endif

char *Fl_Window::default_xclass_ = 0L;

void Fl_Window::_Fl_Window() {
  type(FL_WINDOW);
  box(FL_FLAT_BOX);
  if (Fl::scheme_bg_) {
    labeltype(FL_NORMAL_LABEL);
    align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
    image(Fl::scheme_bg_);
  } else {
    labeltype(FL_NO_LABEL);
  }
  i = 0;
  xclass_ = 0;
  icon_ = 0;
  iconlabel_ = 0;
  resizable(0);
  size_range_set = 0;
  minw = maxw = minh = maxh = 0;
  callback((Fl_Callback*)default_callback);
}

Fl_Window::Fl_Window(int X,int Y,int W, int H, const char *l)
: Fl_Group(X, Y, W, H, l) {
  cursor_default = FL_CURSOR_DEFAULT;
  cursor_fg      = FL_BLACK;
  cursor_bg      = FL_WHITE;

  _Fl_Window();
  set_flag(FORCE_POSITION);
}

Fl_Window::Fl_Window(int W, int H, const char *l)
// fix common user error of a missing end() with current(0):
  : Fl_Group((Fl_Group::current(0),0), 0, W, H, l) {
  cursor_default = FL_CURSOR_DEFAULT;
  cursor_fg      = FL_BLACK;
  cursor_bg      = FL_WHITE;

  _Fl_Window();
  clear_visible();
}

Fl_Window *Fl_Widget::window() const {
  for (Fl_Widget *o = parent(); o; o = o->parent())
    if (o->type() >= FL_WINDOW) return (Fl_Window*)o;
  return 0;
}
/** Gets the x position of the window on the screen */
int Fl_Window::x_root() const {
  Fl_Window *p = window();
  if (p) return p->x_root() + x();
  return x();
}
/** Gets the y position of the window on the screen */
int Fl_Window::y_root() const {
  Fl_Window *p = window();
  if (p) return p->y_root() + y();
  return y();
}

void Fl_Window::draw() {

  // The following is similar to Fl_Group::draw(), but ...
  //  - we draw the box with x=0 and y=0 instead of x() and y()
  //  - we don't draw a label

  if (damage() & ~FL_DAMAGE_CHILD) {	 // draw the entire thing
    draw_box(box(),0,0,w(),h(),color()); // draw box with x/y = 0
  }
  draw_children();

#ifdef __APPLE_QUARTZ__
  // on OS X, windows have no frame. To resize a window, we drag the lower right
  // corner. This code draws a little ribbed triangle for dragging.
  extern CGContextRef fl_gc;
  if (fl_gc && !parent() && resizable() && (!size_range_set || minh!=maxh || minw!=maxw)) {
    int dx = Fl::box_dw(box())-Fl::box_dx(box());
    int dy = Fl::box_dh(box())-Fl::box_dy(box());
    if (dx<=0) dx = 1;
    if (dy<=0) dy = 1;
    int x1 = w()-dx-1, x2 = x1, y1 = h()-dx-1, y2 = y1;
    Fl_Color c[4] = {
      color(),
      fl_color_average(color(), FL_WHITE, 0.7f),
      fl_color_average(color(), FL_BLACK, 0.6f),
      fl_color_average(color(), FL_BLACK, 0.8f),
    };
    int i;
    for (i=dx; i<12; i++) {
      fl_color(c[i&3]);
      fl_line(x1--, y1, x2, y2--);
    }
  }
#endif

# if defined(FLTK_USE_CAIRO)
  Fl::cairo_make_current(this); // checkout if an update is necessary
# endif
}

void Fl_Window::label(const char *name) {
  label(name, iconlabel());
}

void Fl_Window::copy_label(const char *a) {
  if (flags() & COPIED_LABEL) {
    free((void *)label());
    clear_flag(COPIED_LABEL);
  }
  if (a) a = strdup(a);
  label(a, iconlabel());
  set_flag(COPIED_LABEL);
}


void Fl_Window::iconlabel(const char *iname) {
  label(label(), iname);
}

// the Fl::atclose pointer is provided for back compatibility.  You
// can now just change the callback for the window instead.

/** Default callback for window widgets. It hides the window and then calls the default widget callback. */
void Fl::default_atclose(Fl_Window* window, void* v) {
  window->hide();
  Fl_Widget::default_callback(window, v); // put on Fl::read_queue()
}
/** Back compatibility: default window callback handler \see Fl::set_atclose() */
void (*Fl::atclose)(Fl_Window*, void*) = default_atclose;
/** Back compatibility: Sets the default callback v for win to call on close event */
void Fl_Window::default_callback(Fl_Window* win, void* v) {
  Fl::atclose(win, v);
}

/**  Returns the last window that was made current. \see Fl_Window::make_current() */
Fl_Window *Fl_Window::current() {
  return current_;
}

/** Returns the default xclass.

  \see Fl_Window::default_xclass(const char *)

 */
const char *Fl_Window::default_xclass()
{
  if (default_xclass_) {
    return default_xclass_;
  } else {
    return "FLTK";
  }
}

/** Sets the default window xclass.

  The default xclass is used for all windows that don't have their
  own xclass set before show() is called. You can change the default
  xclass whenever you want, but this only affects windows that are
  created (and shown) after this call.

  The given string \p xc is copied. You can use a local variable or
  free the string immediately after this call.

  If you don't call this, the default xclass for all windows will be "FLTK".
  You can reset the default xclass by specifying NULL for \p xc.

  If you call Fl_Window::xclass(const char *) for any window, then
  this also sets the default xclass, unless it has been set before.

  \param[in] xc default xclass for all windows subsequently created

  \see Fl_Window::xclass(const char *)
*/
void Fl_Window::default_xclass(const char *xc)
{
  if (default_xclass_) {
    free(default_xclass_);
    default_xclass_ = 0L;
  }
  if (xc) {
    default_xclass_ = strdup(xc);
  }
}

/** Sets the xclass for this window.

  A string used to tell the system what type of window this is. Mostly
  this identifies the picture to draw in the icon. This only works if
  called \e before calling show().

  <I>Under X</I>, this is turned into a XA_WM_CLASS pair by truncating at
  the first non-alphanumeric character and capitalizing the first character,
  and the second one if the first is 'x'.  Thus "foo" turns into "foo, Foo",
  and "xprog.1" turns into "xprog, XProg".

  <I>Under Microsoft Windows</I>, this string is used as the name of the
  WNDCLASS structure, though it is not clear if this can have any
  visible effect.

  \since FLTK 1.3 the passed string is copied. You can use a local
  variable or free the string immediately after this call. Note that
  FLTK 1.1 stores the \e pointer without copying the string.

  If the default xclass has not yet been set, this also sets the
  default xclass for all windows created subsequently.

  \see Fl_Window::default_xclass(const char *)
*/
void Fl_Window::xclass(const char *xc) 
{
  if (xclass_) {
    free(xclass_);
    xclass_ = 0L;
  }
  if (xc) {
    xclass_ = strdup(xc);
    if (!default_xclass_) {
      default_xclass(xc);
    }
  }
}

/** Returns the xclass for this window, or a default.

  \see Fl_Window::default_xclass(const char *)
  \see Fl_Window::xclass(const char *)
*/
const char *Fl_Window::xclass() const
{
  if (xclass_) {
    return xclass_;
  } else {
    return default_xclass();
  }
}

/** Gets the current icon window target dependent data. */
const void *Fl_Window::icon() const {
  return icon_;
}

/** Sets the current icon window target dependent data. */
void Fl_Window::icon(const void * ic) {
  icon_ = ic;
}


//
// End of "$Id: Fl_Window.cxx 8472 2011-02-25 08:44:47Z AlbrechtS $".
//
