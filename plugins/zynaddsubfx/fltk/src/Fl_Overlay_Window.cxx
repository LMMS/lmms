//
// "$Id: Fl_Overlay_Window.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Overlay window code for the Fast Light Tool Kit (FLTK).
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

// A window using double-buffering and able to draw an overlay
// on top of that.  Uses the hardware to draw the overlay if
// possible, otherwise it just draws in the front buffer.

#include <config.h>
#include <FL/Fl.H>
#include <FL/Fl_Overlay_Window.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

void Fl_Overlay_Window::show() {
  Fl_Double_Window::show();
  if (overlay_ && overlay_ != this) overlay_->show();
}

void Fl_Overlay_Window::hide() {
  Fl_Double_Window::hide();
}

void Fl_Overlay_Window::flush() {
#ifdef BOXX_BUGS
  if (overlay_ && overlay_ != this && overlay_->shown()) {
    // all drawing to windows hidden by overlay windows is ignored, fix this
    XUnmapWindow(fl_display, fl_xid(overlay_));
    Fl_Double_Window::flush(0);
    XMapWindow(fl_display, fl_xid(overlay_));
    return;
  }
#endif
  int erase_overlay = (damage()&FL_DAMAGE_OVERLAY);
  clear_damage((uchar)(damage()&~FL_DAMAGE_OVERLAY));
  Fl_Double_Window::flush(erase_overlay);
  if (overlay_ == this) draw_overlay();
}

void Fl_Overlay_Window::resize(int X, int Y, int W, int H) {
  Fl_Double_Window::resize(X,Y,W,H);
  if (overlay_ && overlay_!=this) overlay_->resize(0,0,w(),h());
}

Fl_Overlay_Window::~Fl_Overlay_Window() {
  hide();
//  delete overlay; this is done by ~Fl_Group
}

#if !HAVE_OVERLAY

int Fl_Overlay_Window::can_do_overlay() {return 0;}

void Fl_Overlay_Window::redraw_overlay() {
  overlay_ = this;
  clear_damage((uchar)(damage()|FL_DAMAGE_OVERLAY));
  Fl::damage(FL_DAMAGE_CHILD);
}

#else

extern XVisualInfo *fl_find_overlay_visual();
extern XVisualInfo *fl_overlay_visual;
extern Colormap fl_overlay_colormap;
extern unsigned long fl_transparent_pixel;
static GC gc;	// the GC used by all X windows
extern uchar fl_overlay; // changes how fl_color(x) works

class _Fl_Overlay : public Fl_Window {
  friend class Fl_Overlay_Window;
  void flush();
  void show();
public:
  _Fl_Overlay(int x, int y, int w, int h) :
    Fl_Window(x,y,w,h) {set_flag(INACTIVE);}
};

int Fl_Overlay_Window::can_do_overlay() {
  return fl_find_overlay_visual() != 0;
}

void _Fl_Overlay::show() {
  if (shown()) {Fl_Window::show(); return;}
  fl_background_pixel = int(fl_transparent_pixel);
  Fl_X::make_xid(this, fl_overlay_visual, fl_overlay_colormap);
  fl_background_pixel = -1;
  // find the outermost window to tell wm about the colormap:
  Fl_Window *w = window();
  for (;;) {Fl_Window *w1 = w->window(); if (!w1) break; w = w1;}
  XSetWMColormapWindows(fl_display, fl_xid(w), &(Fl_X::i(this)->xid), 1);
}

void _Fl_Overlay::flush() {
  fl_window = fl_xid(this);
  if (!gc) gc = XCreateGC(fl_display, fl_xid(this), 0, 0);
  fl_gc = gc;
  fl_overlay = 1;
  Fl_Overlay_Window *w = (Fl_Overlay_Window *)parent();
  Fl_X *myi = Fl_X::i(this);
  if (damage() != FL_DAMAGE_EXPOSE) XClearWindow(fl_display, fl_xid(this));
  fl_clip_region(myi->region); myi->region = 0;
  w->draw_overlay();
  fl_overlay = 0;
}

void Fl_Overlay_Window::redraw_overlay() {
  if (!fl_display) return; // this prevents fluid -c from opening display
  if (!overlay_) {
    if (can_do_overlay()) {
      Fl_Group::current(this);
      overlay_ = new _Fl_Overlay(0,0,w(),h());
      Fl_Group::current(0);
    } else {
      overlay_ = this;	// fake the overlay
    }
  }
  if (shown()) {
    if (overlay_ == this) {
      clear_damage(damage()|FL_DAMAGE_OVERLAY);
      Fl::damage(FL_DAMAGE_CHILD);
    } else if (!overlay_->shown())
      overlay_->show();
    else
      overlay_->redraw();
  }
}

#endif

//
// End of "$Id: Fl_Overlay_Window.cxx 5190 2006-06-09 16:16:34Z mike $".
//
