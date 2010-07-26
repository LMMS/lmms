//
// "$Id: Fl_Sys_Menu_Bar.cxx 7518 2010-04-16 19:27:28Z manolo $"
//
// MacOS system menu bar widget for the Fast Light Tool Kit (FLTK).
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

/**
 * This code is a quick hack! It was written as a proof of concept.
 * It has been tested on the "menubar" sample program and provides
 * basic functionality. 
 * 
 * To use the System Menu Bar, simply replace the main Fl_Menu_Bar
 * in an application with Fl_Sys_Menu_Bar.
 *
 * FLTK features not supported by the Mac System menu
 *
 * - no invisible menu items
 * - no symbolic labels
 * - embossed labels will be underlined instead
 * - no font sizes
 * - Shortcut Characters should be English alphanumeric only, no modifiers yet
 * - no disable main menus
 * - changes to menubar in run-time don't update! 
 *     (disable, etc. - toggle and radio button do!)
 *
 * No care was taken to clean up the menu bar after destruction!
 * ::menu(bar) should only be called once!
 * Many other calls of the parent class don't work.
 * Changing the menu items has no effect on the menu bar.
 * Starting with OS X 10.5, FLTK applications must be created as
 * a bundle for the System Menu Bar (and maybe other features) to work!
 */

#if defined(__APPLE__) || defined(FL_DOXYGEN)

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Sys_Menu_Bar.H>

#include "flstring.h"
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

#define MenuHandle void *

typedef const Fl_Menu_Item *pFl_Menu_Item;
 

/*
 * Set a shortcut for an Apple menu item using the FLTK shortcut descriptor.
 */
static void setMenuShortcut( MenuHandle mh, int miCnt, const Fl_Menu_Item *m )
{
  if ( !m->shortcut_ ) 
    return;
  if ( m->flags & FL_SUBMENU )
    return;
  if ( m->flags & FL_SUBMENU_POINTER )
    return;
  char key = m->shortcut_ & 0xff;
  if ( !isalnum( key ) )
    return;
  
  void *menuItem = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::itemAtIndex, mh, miCnt);
  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setKeyEquivalent, menuItem, m->shortcut_ & 0xff );
  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setKeyEquivalentModifierMask, menuItem, m->shortcut_ );
}


/*
 * Set the Toggle and Radio flag based on FLTK flags
 */
static void setMenuFlags( MenuHandle mh, int miCnt, const Fl_Menu_Item *m )
{
  if ( m->flags & FL_MENU_TOGGLE )
  {
	void *menuItem = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::itemAtIndex, mh, miCnt);
	Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setState, menuItem, m->flags & FL_MENU_VALUE );
  }
  else if ( m->flags & FL_MENU_RADIO ) {
    void *menuItem = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::itemAtIndex, mh, miCnt);
    Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setState, menuItem, m->flags & FL_MENU_VALUE );
  }
}


/*
 * create a sub menu for a specific menu handle
 */
static void createSubMenu( void * mh, pFl_Menu_Item &mm )
{
  void *submenu;
  int miCnt, flags;
  
  void *menuItem;
  submenu = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::initWithTitle, mm->text);
  int cnt;
  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::numberOfItems, mh, &cnt);
  cnt--;
  menuItem = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::itemAtIndex, mh, cnt);
  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setSubmenu, menuItem, submenu);
  if ( mm->flags & FL_MENU_INACTIVE ) {
    Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setEnabled, menuItem, 0);
  }
  mm++;
  
  while ( mm->text )
  {
    int flRank = mm - fl_sys_menu_bar->Fl_Menu_::menu();
    Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::addNewItem, submenu, flRank, &miCnt);
    setMenuFlags( submenu, miCnt, mm );
    setMenuShortcut( submenu, miCnt, mm );
    if ( mm->flags & FL_MENU_INACTIVE ) {
      void *item = Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::itemAtIndex, submenu, miCnt);
      Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::setEnabled, item, 0);
    }
    flags = mm->flags;
    if ( mm->flags & FL_SUBMENU )
    {
      createSubMenu( submenu, mm );
    }
    else if ( mm->flags & FL_SUBMENU_POINTER )
    {
      const Fl_Menu_Item *smm = (Fl_Menu_Item*)mm->user_data_;
      createSubMenu( submenu, smm );
    }
    if ( flags & FL_MENU_DIVIDER ) {
      Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::addSeparatorItem, submenu);
      }
    mm++;
  }
}
 

/*
 * convert a complete Fl_Menu_Item array into a series of menus in the top menu bar
 * ALL PREVIOUS SYSTEM MENUS, EXCEPT APPLICATION MENU, ARE REPLACED BY THE NEW DATA
 */
static void convertToMenuBar(const Fl_Menu_Item *mm)
{
  int count;//first, delete all existing system menus
  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::numberOfItems, fl_system_menu, &count);
  for(int i = count - 1; i > 0; i--) {
	  Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::removeItem, fl_system_menu, i);
  }
  //now convert FLTK stuff into MacOS menus
  for (;;)
  {
    if ( !mm || !mm->text )
      break;
    char visible = mm->visible() ? 1 : 0;
    int flRank = mm - fl_sys_menu_bar->Fl_Menu_::menu();
    Fl_Sys_Menu_Bar::doMenuOrItemOperation(Fl_Sys_Menu_Bar::addNewItem, fl_system_menu, flRank, NULL);
		
    if ( mm->flags & FL_SUBMENU )
      createSubMenu( fl_system_menu, mm );
    else if ( mm->flags & FL_SUBMENU_POINTER ) {
      const Fl_Menu_Item *smm = (Fl_Menu_Item*)mm->user_data_;
      createSubMenu( fl_system_menu, smm );
    }
    if ( visible ) {
      //      InsertMenu( mh, 0 );
    }
    mm++;
  }
}


/**
 * @brief create a system menu bar using the given list of menu structs
 *
 * \author Matthias Melcher
 *
 * @param m list of Fl_Menu_Item
 */
void Fl_Sys_Menu_Bar::menu(const Fl_Menu_Item *m) 
{
  fl_open_display();
  Fl_Menu_Bar::menu( m );
  convertToMenuBar(m);
}


/**
 * @brief add to the system menu bar a new menu item
 *
 * add to the system menu bar a new menu item, with a title string, shortcut int,
 * callback, argument to the callback, and flags.
 *
 * @see Fl_Menu_::add(const char* label, int shortcut, Fl_Callback *cb, void *user_data, int flags) 
 */
int Fl_Sys_Menu_Bar::add(const char* label, int shortcut, Fl_Callback *cb, void *user_data, int flags)
{
  fl_open_display();
  int rank = Fl_Menu_::add(label, shortcut, cb, user_data, flags);
  convertToMenuBar(Fl_Menu_::menu());
  return rank;
}

/**
 * @brief insert in the system menu bar a new menu item
 *
 * insert in the system menu bar a new menu item, with a title string, shortcut int,
 * callback, argument to the callback, and flags.
 *
 * @see Fl_Menu_::insert(int index, const char* label, int shortcut, Fl_Callback *cb, void *user_data, int flags) 
 */
int Fl_Sys_Menu_Bar::insert(int index, const char* label, int shortcut, Fl_Callback *cb, void *user_data, int flags)
{
  fl_open_display();
  int rank = Fl_Menu_::insert(index, label, shortcut, cb, user_data, flags);
  convertToMenuBar(Fl_Menu_::menu());
  return rank;
}

void Fl_Sys_Menu_Bar::clear()
{
  Fl_Menu_::clear();
  convertToMenuBar(NULL);
}

int Fl_Sys_Menu_Bar::clear_submenu(int index)
{
  int retval = Fl_Menu_::clear_submenu(index);
  if (retval != -1) convertToMenuBar(Fl_Menu_::menu());
  return retval;
}

/**
 * @brief remove an item from the system menu bar
 *
 * @param rank		the rank of the item to remove
 */
void Fl_Sys_Menu_Bar::remove(int rank)
{
  Fl_Menu_::remove(rank);
  convertToMenuBar(Fl_Menu_::menu());
}


/**
 * @brief rename an item from the system menu bar
 *
 * @param rank		the rank of the item to rename
 * @param name		the new item name as a UTF8 string
 */
void Fl_Sys_Menu_Bar::replace(int rank, const char *name)
{
  doMenuOrItemOperation(renameItem, rank, name);
  fl_sys_menu_bar->Fl_Menu_::replace(rank, name);
}


/*
 * Draw the menu bar. 
 * Nothing here because the OS does this for us.
 */
void Fl_Sys_Menu_Bar::draw() {
}


#endif /* __APPLE__ */

//
// End of "$Id: Fl_Sys_Menu_Bar.cxx 7518 2010-04-16 19:27:28Z manolo $".
//
