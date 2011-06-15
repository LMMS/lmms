//
// "$Id: Fl_Sys_Menu_Bar.H 7903 2010-11-28 21:06:39Z matt $"
//
// MacOS system menu bar header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Sys_Menu_Bar_H
#define Fl_Sys_Menu_Bar_H

#include "Fl_Menu_Bar.H"
#include "x.H"

#if defined(__APPLE__) || defined(FL_DOXYGEN)

/**
 @brief A class to create, modify and delete menus that appear on Mac OS X in the menu bar at the top of the screen.
 *
 * On other than Mac OS X platforms, Fl_Sys_Menu_Bar is a synonym of class Fl_Menu_Bar.
 */
class FL_EXPORT Fl_Sys_Menu_Bar : public Fl_Menu_Bar {
protected:
  void draw();
public:
  /**
   @brief The constructor.
   *
   * On Mac OS X, all arguments are unused. On other platforms they are used as by Fl_Menu_Bar::Fl_Menu_Bar().
   */
  Fl_Sys_Menu_Bar(int x,int y,int w,int h,const char *l=0)
      : Fl_Menu_Bar(x,y,w,h,l) {
    deactivate();			// don't let the old area take events
    fl_sys_menu_bar = this;
  }
  void menu(const Fl_Menu_Item *m);
  int add(const char* label, int shortcut, Fl_Callback*, void *user_data=0, int flags=0);
  int insert(int index, const char* label, int shortcut, Fl_Callback *cb, void *user_data=0, int flags=0);
  void remove(int n);
  void replace(int rank, const char *name);
  void clear();
  int clear_submenu(int index);
#if ! defined(FL_DOXYGEN)
  enum menuOrItemOperation { itemAtIndex, setKeyEquivalent, setKeyEquivalentModifierMask, setState, initWithTitle,
    numberOfItems, setSubmenu, setEnabled, addSeparatorItem, setTitle, removeItem, addNewItem, renameItem };
  // function doMenuOrItemOperation is in file Fl_cocoa.mm because it contains objective-c code
  static void *doMenuOrItemOperation( menuOrItemOperation operation, ...);
#endif
};

#else

typedef Fl_Menu_Bar Fl_Sys_Menu_Bar;

#endif // defined(__APPLE__) || defined(FL_DOXYGEN)

#endif // Fl_Sys_Menu_Bar_H

//
// End of "$Id: Fl_Sys_Menu_Bar.H 7903 2010-11-28 21:06:39Z matt $".
//
