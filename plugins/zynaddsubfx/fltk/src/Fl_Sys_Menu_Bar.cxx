//
// "$Id: Fl_Sys_Menu_Bar.cxx 6016 2008-01-09 21:32:40Z matt $"
//
// MacOS system menu bar widget for the Fast Light Tool Kit (FLTK).
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
 * - no sybolic labels
 * - embossed labels will be underlined instead
 * - no font sizes
 * - Shortcut Characters should be English alphanumeric only, no modifiers yet
 * - no disable main menus
 * - changes to menubar in run-time don't update! 
 *     (disable, etc. - toggle and readio button do!)
 *
 * No care was taken to clean up the menu bar after destruction!
 * ::menu(bar) should only be called once!
 * Many other calls of the parent class don't work.
 * Changing the menu items has no effect on the menu bar.
 * Starting with OS X 10.5, FLTK applications must be created as
 * a bundle for the System Menu Bar (and maybe other features) to work!
 */

#if defined(__APPLE__)

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/Fl_Sys_Menu_Bar.H>

#include "flstring.h"
#include <stdio.h>
#include <ctype.h>

typedef const Fl_Menu_Item *pFl_Menu_Item;

/**
 * copy the text of a menuitem into a buffer.
 * Skip all '&' which would mark the shortcut in FLTK
 * Skip all Mac control characters ('(', '<', ';', '^', '!' )
 */
static void catMenuText( const char *src, char *dst )
{
  char c;
  while ( *dst ) 
    dst++;
  if ( *src == '-' ) 
    src++;
  while ( ( c = *src++ ) ) 
  {
    if ( !strchr( "&(<;^!", c )  )
      *dst++ = c;
  }
  *dst = 0;
}

/**
 * append a marker to identify the menu font style
 * <B, I, U, O, and S
 */
static void catMenuFont( const Fl_Menu_Item *m, char *dst )
{
  if ( !m->labeltype_ && !m->labelfont_ ) 
    return;
  while ( *dst ) 
    dst++;
    
  if ( m->labelfont_ & FL_BOLD )
    strcat( dst, "<B" );
  if ( m->labelfont_ & FL_ITALIC )
    strcat( dst, "<I" );
  //if ( m->labelfont_ & FL_UNDERLINE )
  //  strcat( dst, "<U" );
  
  if ( m->labeltype_ == FL_EMBOSSED_LABEL )
      strcat( dst, "<U" );
  else if ( m->labeltype_ == FL_ENGRAVED_LABEL )
      strcat( dst, "<O" );
  else if ( m->labeltype_ == FL_SHADOW_LABEL )
      strcat( dst, "<S" );
  //else if ( m->labeltype_ == FL_SYMBOL_LABEL )
      ; // not supported
}

/**
 * append a marker to identify the menu shortcut
 * <B, I, U, O, and S
enum {
  kMenuNoModifiers = 0,
  kMenuShiftModifier = (1 << 0),
  kMenuOptionModifier = (1 << 1),
  kMenuControlModifier = (1 << 2),
  kMenuNoCommandModifier = (1 << 3)
}; 
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
  
  long macMod = kMenuNoCommandModifier;
  if ( m->shortcut_ & FL_META ) macMod = kMenuNoModifiers;
  if ( m->shortcut_ & FL_SHIFT || isupper(key) ) macMod |= kMenuShiftModifier;
  if ( m->shortcut_ & FL_ALT ) macMod |= kMenuOptionModifier;
  if ( m->shortcut_ & FL_CTRL ) macMod |= kMenuControlModifier;
  
  //SetMenuItemKeyGlyph( mh, miCnt, key );
  SetItemCmd( mh, miCnt, toupper(key) );
  SetMenuItemModifiers( mh, miCnt, macMod );
}

#if 0
// this function needs to be verified before we compile it back in.
static void catMenuShortcut( const Fl_Menu_Item *m, char *dst )
{
  if ( !m->shortcut_ ) 
    return;
  char c = m->shortcut_ & 0xff;
  if ( !isalnum( c & 0xff ) )
    return;
  while ( *dst ) 
    dst++;
  if ( m->shortcut_ & FL_CTRL )
  {
    sprintf( dst, "/%c", toupper( c ) );
  }
  //if ( isalnum( mm->shortcut_ ) && !( mm->flags & FL_SUBMENU ) )
  //sprintf( buf+strlen(buf), "/%c", mm->shortcut_ );
}
#endif

static void setMenuFlags( MenuHandle mh, int miCnt, const Fl_Menu_Item *m )
{
  if ( m->flags & FL_MENU_TOGGLE )
  {
    SetItemMark( mh, miCnt, ( m->flags & FL_MENU_VALUE ) ? 0x12 : 0 );
  }
  else if ( m->flags & FL_MENU_RADIO )
    SetItemMark( mh, miCnt, ( m->flags & FL_MENU_VALUE ) ? 0x13 : 0 );
}

static void catMenuFlags( const Fl_Menu_Item *m, char *dst )
{
  if ( !m->flags ) 
    return;
  if ( m->flags & FL_MENU_INACTIVE )
    strcat( dst, "(" );
}

/**
 * create a sub menu for a specific menu handle
 */
static void createSubMenu( MenuHandle mh, int &cnt, pFl_Menu_Item &mm )
{
  char buf[255];
  int miCnt = 1;
  while ( mm->text )
  {
    MenuHandle smh = 0;
    buf[1] = 0;
    catMenuFont( mm, buf+1 );
    //catMenuShortcut( mm, buf+1 );
    catMenuText( mm->text, buf+1 );
    catMenuFlags( mm, buf+1 );
    if ( mm->flags & (FL_SUBMENU | FL_SUBMENU_POINTER) )
    {
      cnt++;
      smh = NewMenu( cnt, (unsigned char*)"\001 " );
      sprintf( buf+1+strlen(buf+1), "/\033!%c", cnt );
    }
    if ( mm->flags & FL_MENU_DIVIDER )
      strcat( buf+1, ";-" );
    buf[0] = strlen( buf+1 );
    AppendMenu( mh, (unsigned char*)buf );
    // insert Appearanc manager functions here!
    setMenuFlags( mh, miCnt, mm );
    setMenuShortcut( mh, miCnt, mm );
    SetMenuItemRefCon( mh, miCnt, (UInt32)mm );
    miCnt++;
    if ( mm->flags & FL_MENU_DIVIDER )
      miCnt++;
    if ( mm->flags & FL_SUBMENU )
    {
      createSubMenu( smh, cnt, ++mm );
    }
    else if ( mm->flags & FL_SUBMENU_POINTER )
    {
      const Fl_Menu_Item *smm = (Fl_Menu_Item*)mm->user_data_;
      createSubMenu( mh, cnt, smm );
    }
    mm++;
  }
  InsertMenu( mh, -1 );
}
 

/**
 * create a system menu bar using the given list of menu structs
 *
 * \author Matthias Melcher
 *
 * @param m list of Fl_Menu_Item
 */
void Fl_Sys_Menu_Bar::menu(const Fl_Menu_Item *m) 
{
  fl_open_display();
  Fl_Menu_Bar::menu( m );
  fl_sys_menu_bar = this;

  char buf[255];

  int cnt = 1; // first menu is no 2. no 1 is the Apple Menu
  const Fl_Menu_Item *mm = m;
  for (;;)
  {
    if ( !mm || !mm->text )
      break;
    char visible = mm->visible() ? 1 : 0;
    buf[1] = 0;
    catMenuText( mm->text, buf+1 );
    buf[0] = strlen( buf+1 );
    MenuHandle mh = NewMenu( ++cnt, (unsigned char*)buf );
    if ( mm->flags & FL_MENU_INACTIVE ) {
      ChangeMenuAttributes(mh, kMenuAttrAutoDisable, 0);
      DisableAllMenuItems(mh);
      DisableMenuItem(mh, 0);
    }
    if ( mm->flags & FL_SUBMENU )
      createSubMenu( mh, cnt, ++mm );
    else if ( mm->flags & FL_SUBMENU_POINTER ) {
      const Fl_Menu_Item *smm = (Fl_Menu_Item*)mm->user_data_;
      createSubMenu( mh, cnt, smm );
    }
    if ( visible ) {
      InsertMenu( mh, 0 );
    }
    mm++;
  }
  DrawMenuBar();
}

/*
const Fl_Menu_Item* Fl_Sys_Menu_Bar::picked(const Fl_Menu_Item* v) {
  Fl_menu_Item *ret = Fl_Menu_Bar::picked( v );
  
  if ( m->flags & FL_MENU_TOGGLE )
  {
    SetItemMark( mh, miCnt, ( m->flags & FL_MENU_VALUE ) ? 0x12 : 0 );
  }
  
  return ret;
}
*/

void Fl_Sys_Menu_Bar::draw() {
/* -- nothing to do, system should take care of this
  draw_box();
  if (!menu() || !menu()->text) return;
  const Fl_Menu_Item* m;
  int X = x()+6;
  for (m=menu(); m->text; m = m->next()) {
    int W = m->measure(0,this) + 16;
    m->draw(X, y(), W, h(), this);
    X += W;
  }
  */
}

/*
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
*/

#endif /* __APPLE__ */

//
// End of "$Id: Fl_Sys_Menu_Bar.cxx 6016 2008-01-09 21:32:40Z matt $".
//
