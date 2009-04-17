//
// "$Id: Fl_Menu_Bar.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Menu bar widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_draw.H>

void Fl_Menu_Bar::draw() {
  draw_box();
  if (!menu() || !menu()->text) return;
  const Fl_Menu_Item* m;
  int X = x()+6;
  for (m=menu()->first(); m->text; m = m->next()) {
    int W = m->measure(0,this) + 16;
    m->draw(X, y(), W, h(), this);
    X += W;
    if (m->flags & FL_MENU_DIVIDER) {
      int y1 = y() + Fl::box_dy(box());
      int y2 = y1 + h() - Fl::box_dh(box()) - 1;

      // Draw a vertical divider between menus...
      fl_color(FL_DARK3);
      fl_yxline(X - 6, y1, y2);
      fl_color(FL_LIGHT3);
      fl_yxline(X - 5, y1, y2);
    }
  }
}

int Fl_Menu_Bar::handle(int event) {
  const Fl_Menu_Item* v;
  if (menu() && menu()->text) switch (event) {
  case FL_ENTER:
  case FL_LEAVE:
    return 1;
  case FL_PUSH:
    v = 0;
  J1:
    v = menu()->pulldown(x(), y(), w(), h(), v, this, 0, 1);
    picked(v);
    return 1;
  case FL_SHORTCUT:
    if (visible_r()) {
      v = menu()->find_shortcut();
      if (v && v->submenu()) goto J1;
    }
    return test_shortcut() != 0;
  }
  return 0;
}

//
// End of "$Id: Fl_Menu_Bar.cxx 5190 2006-06-09 16:16:34Z mike $".
//
