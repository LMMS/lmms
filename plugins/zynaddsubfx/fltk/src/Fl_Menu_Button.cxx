//
// "$Id: Fl_Menu_Button.cxx 7903 2010-11-28 21:06:39Z matt $"
//
// Menu button widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_draw.H>


static Fl_Menu_Button	*pressed_menu_button_ = 0;

void Fl_Menu_Button::draw() {
  if (!box() || type()) return;
  draw_box(pressed_menu_button_ == this ? fl_down(box()) : box(), color());
  draw_label();
  if (Fl::focus() == this) draw_focus();
  // ** if (box() == FL_FLAT_BOX) return; // for XForms compatibility
  int H = (labelsize()-3)&-2;
  int X = x()+w()-H*2;
  int Y = y()+(h()-H)/2;
  fl_color(active_r() ? FL_DARK3 : fl_inactive(FL_DARK3));
  fl_line(X+H/2, Y+H, X, Y, X+H, Y);
  fl_color(active_r() ? FL_LIGHT3 : fl_inactive(FL_LIGHT3));
  fl_line(X+H, Y, X+H/2, Y+H);
}

/**
  Act exactly as though the user clicked the button or typed the
  shortcut key.  The menu appears, it waits for the user to pick an item,
  and if they pick one it sets value() and does the callback or
  sets changed() as described above.  The menu item is returned
  or NULL if the user dismisses the menu.
*/
const Fl_Menu_Item* Fl_Menu_Button::popup() {
  const Fl_Menu_Item* m;
  pressed_menu_button_ = this;
  redraw();
  Fl_Widget_Tracker mb(this);
  if (!box() || type()) {
    m = menu()->popup(Fl::event_x(), Fl::event_y(), label(), mvalue(), this);
  } else {
    m = menu()->pulldown(x(), y(), w(), h(), 0, this);
  }
  picked(m);
  pressed_menu_button_ = 0;
  if (mb.exists()) redraw();
  return m;
}

int Fl_Menu_Button::handle(int e) {
  if (!menu() || !menu()->text) return 0;
  switch (e) {
  case FL_ENTER: /* FALLTHROUGH */
  case FL_LEAVE:
    return (box() && !type()) ? 1 : 0;
  case FL_PUSH:
    if (!box()) {
      if (Fl::event_button() != 3) return 0;
    } else if (type()) {
      if (!(type() & (1 << (Fl::event_button()-1)))) return 0;
    }
    if (Fl::visible_focus()) Fl::focus(this);
    popup();
    return 1;
  case FL_KEYBOARD:
    if (!box()) return 0;
    if (Fl::event_key() == ' ' &&
        !(Fl::event_state() & (FL_SHIFT | FL_CTRL | FL_ALT | FL_META))) {
      popup();
      return 1;
    } else return 0;
  case FL_SHORTCUT:
    if (Fl_Widget::test_shortcut()) {popup(); return 1;}
    return test_shortcut() != 0;
  case FL_FOCUS: /* FALLTHROUGH */
  case FL_UNFOCUS:
    if (box() && Fl::visible_focus()) {
      redraw();
      return 1;
    }
  default:
    return 0;
  }
}

/**
  Creates a new Fl_Menu_Button widget using the given position,
  size, and label string. The default boxtype is FL_UP_BOX.
  <P>The constructor sets menu() to NULL.  See 
  Fl_Menu_ for the methods to set or change the menu.
*/
Fl_Menu_Button::Fl_Menu_Button(int X,int Y,int W,int H,const char *l)
: Fl_Menu_(X,Y,W,H,l) {
  down_box(FL_NO_BOX);
}

//
// End of "$Id: Fl_Menu_Button.cxx 7903 2010-11-28 21:06:39Z matt $".
//
