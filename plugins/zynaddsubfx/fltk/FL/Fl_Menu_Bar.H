//
// "$Id: Fl_Menu_Bar.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Menu bar header file for the Fast Light Tool Kit (FLTK).
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

/* \file
   Fl_Menu_Bar widget . */

#ifndef Fl_Menu_Bar_H
#define Fl_Menu_Bar_H

#include "Fl_Menu_.H"

/**
  This widget provides a standard menubar interface.  Usually you will
  put this widget along the top edge of your window.  The height of the
  widget should be 30 for the menu titles to draw correctly with the
  default font.
  <P>The items on the bar and the menus they bring up are defined by a
  single Fl_Menu_Item
  array.  Because a Fl_Menu_Item array defines a hierarchy, the
  top level menu defines the items in the menubar, while the submenus
  define the pull-down menus. Sub-sub menus and lower pop up to the right
  of the submenus. </P>
  <P ALIGN=CENTER>\image html  menubar.png</P>
  \image latex  menubar.png " menubar" width=12cm
  <P>If there is an item in the top menu that is not a title of a
  submenu, then it acts like a "button" in the menubar.  Clicking on it
  will pick it. </P>
  <P>When the user picks an item off the menu, the item's callback is
  done with the menubar as the Fl_Widget* argument.  If the item
  does not have a callback the menubar's callback is done instead. </P>
  <P>Submenus will also pop up in response to shortcuts indicated by
  putting a '&' character in the name field of the menu item. If you put a
  '&' character in a top-level "button" then the shortcut picks it.  The
  '&' character in submenus is ignored until the menu is popped up. </P>
  <P>Typing the shortcut() of any of the menu items will cause
  callbacks exactly the same as when you pick the item with the mouse.
*/
class FL_EXPORT Fl_Menu_Bar : public Fl_Menu_ {
protected:
    void draw();
public:
    int handle(int);
  /**
      Creates a new Fl_Menu_Bar widget using the given position,
    size, and label string. The default boxtype is FL_UP_BOX.
    <P>The constructor sets menu() to NULL.  See 
    Fl_Menu_ for the methods to set or change the menu. </P>
    <P>labelsize(), labelfont(), and labelcolor()
    are used to control how the menubar items are drawn.  They are
    initialized from the Fl_Menu static variables, but you can
    change them if desired. </P>
    <P>label() is ignored unless you change align() to
    put it outside the menubar.
    <P>The destructor removes the Fl_Menu_Bar widget and all of its
    menu items.
  */
    Fl_Menu_Bar(int X, int Y, int W, int H,const char *l=0)
      : Fl_Menu_(X,Y,W,H,l) {}
};

#endif

//
// End of "$Id: Fl_Menu_Bar.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
