//
// "$Id: Fl_Menu_add.cxx 8110 2010-12-23 08:02:52Z manolo $"
//
// Menu utilities for the Fast Light Tool Kit (FLTK).
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
// Methods to alter the menu in an Fl_Menu_ widget.

// These are for Forms emulation and for dynamically changing the
// menus.  They are in this source file so they are not linked in if
// not used, which is what will happen if the program only uses
// constant menu tables.

// Not at all guaranteed to be Forms compatible, especially with any
// string with a % sign in it!

#include <FL/Fl_Menu_.H>
#include "flstring.h"
#include <stdio.h>
#include <stdlib.h>

// If the array is this, we will double-reallocate as necessary:
static Fl_Menu_Item* local_array = 0;
static int local_array_alloc = 0; // number allocated
static int local_array_size = 0; // == size(local_array)
extern Fl_Menu_* fl_menu_array_owner; // in Fl_Menu_.cxx

// For historical reasons there are matching methods that work on a
// user-allocated array of Fl_Menu_Item.  These methods are quite
// depreciated and should not be used.  These old methods use the
// above pointers to detect if the array belongs to an Fl_Menu_
// widget, and if so it reallocates as necessary.



// Insert a single Fl_Menu_Item into an array of size at offset n,
// if this is local_array it will be reallocated if needed.
static Fl_Menu_Item* array_insert(
  Fl_Menu_Item* array,  // array to modify
  int size,             // size of array
  int n,                // index of new insert position
  const char *text,     // text of new item (copy is made)
  int flags             // flags for new item
) {
  if (array == local_array && size >= local_array_alloc) {
    local_array_alloc = 2*size;
    Fl_Menu_Item* newarray = new Fl_Menu_Item[local_array_alloc];
    memmove(newarray, array, size*sizeof(Fl_Menu_Item));
    delete[] local_array;
    local_array = array = newarray;
  }
  // move all the later items:
  memmove(array+n+1, array+n, sizeof(Fl_Menu_Item)*(size-n));
  // create the new item:
  Fl_Menu_Item* m = array+n;
  m->text = text ? strdup(text) : 0;
  m->shortcut_ = 0;
  m->callback_ = 0;
  m->user_data_ = 0;
  m->flags = flags;
  m->labeltype_ = m->labelsize_ = m->labelcolor_ = 0;
  m->labelfont_ = FL_HELVETICA; 
  return array;
}



// Comparison that does not care about deleted '&' signs:
static int compare(const char* a, const char* b) {
  for (;;) {
    int n = *a-*b;
    if (n) {
      if (*a == '&') a++;
      else if (*b == '&') b++;
      else return n;
    } else if (*a) {
      a++; b++;
    } else {
      return 0;
    }
  }
}



/** Adds an item.  The text is split at '/' characters to automatically
   produce submenus (actually a totally unnecessary feature as you can
   now add submenu titles directly by setting SUBMENU in the flags):
*/
int Fl_Menu_Item::add(
  const char *mytext,
  int sc,
  Fl_Callback *cb,	
  void *data,
  int myflags
) {
  return(insert(-1,mytext,sc,cb,data,myflags));		// -1: append
}



/** 
 Inserts an item at position \p index.
    
 If \p index is -1, the item is added the same way as Fl_Menu_Item::add().

 If 'mytext' contains any un-escaped front slashes (/), it's assumed 
 a menu pathname is being specified, and the value of \p index 
 will be ignored.

 In all other aspects, the behavior of insert() is the same as add().
 
 \param index insert new items here
 \param mytext new label string, details see above
 \param sc keyboard shortcut for new item
 \param cb callback function for new item
 \param data user data for new item
 \param myflags menu flags as described in FL_Menu_Item
 \returns the index into the menu() array, where the entry was added
*/
int Fl_Menu_Item::insert(
  int index,
  const char *mytext,
  int sc,
  Fl_Callback *cb,	
  void *data,
  int myflags
) {
  Fl_Menu_Item *array = this;
  Fl_Menu_Item *m = this;
  const char *p;
  char *q;
  char buf[1024];

  int msize = array==local_array ? local_array_size : array->size();
  int flags1 = 0;
  const char* item;

  // split at slashes to make submenus:
  for (;;) {

    // leading slash makes us assume it is a filename:
    if (*mytext == '/') {item = mytext; break;}

    // leading underscore causes divider line:
    if (*mytext == '_') {mytext++; flags1 = FL_MENU_DIVIDER;}

    // copy to buf, changing \x to x:
    q = buf;
    for (p=mytext; *p && *p != '/'; *q++ = *p++) if (*p=='\\' && p[1]) p++;
    *q = 0;

    item = buf;
    if (*p != '/') break; /* not a menu title */
    index = -1;           /* any submenu specified overrides insert position */
    mytext = p+1;         /* point at item title */

    /* find a matching menu title: */
    for (; m->text; m = m->next())
      if (m->flags&FL_SUBMENU && !compare(item, m->text)) break;

    if (!m->text) { /* create a new menu */
      int n = (index==-1) ? m-array : index;
      array = array_insert(array, msize, n, item, FL_SUBMENU|flags1);
      msize++;
      array = array_insert(array, msize, n+1, 0, 0);
      msize++;
      m = array+n;
    }
    m++;	/* go into the submenu */
    flags1 = 0;
  }

  /* find a matching menu item: */
  for (; m->text; m = m->next())
    if (!(m->flags&FL_SUBMENU) && !compare(m->text,item)) break;

  if (!m->text) {	/* add a new menu item */
    int n = (index==-1) ? m-array : index;
    array = array_insert(array, msize, n, item, myflags|flags1);
    msize++;
    if (myflags & FL_SUBMENU) { // add submenu delimiter
      array = array_insert(array, msize, n+1, 0, 0);
      msize++;
    }
    m = array+n;
  }

  /* fill it in */
  m->shortcut_ = sc;
  m->callback_ = cb;
  m->user_data_ = data;
  m->flags = myflags|flags1;

  if (array == local_array) local_array_size = msize;
  return m-array;
}



/**
  Adds a new menu item.
  
  \param[in] label    The text label for the menu item.
  \param[in] shortcut Optional keyboard shortcut that can be an int or string; (FL_CTRL+'a') or "^a". Default 0 if none.
  \param[in] callback Optional callback invoked when user clicks the item. Default 0 if none.
  \param[in] userdata Optional user data passed as an argument to the callback. Default 0 if none.
  \param[in] flags    Optional flags that control the type of menu item; see below. Default is 0 for none.
  \returns            The index into the menu() array, where the entry was added.
  
  \par Description
  If the menu array was directly set with menu(x), then copy() is done 
  to make a private array.
  \par 
  Since this method can change the internal menu array, any menu item
  pointers or indecies the application may have cached can become stale,
  and should be recalculated/refreshed.
  \par
  A menu item's callback must not add() items to its parent menu during the callback.

  <B>Detailed Description of Parameters</B>
  \par label
  The menu item's label. This option is required.
  \par
  The characters "&", "/", "\", and "_" are treated as special characters in the label string. 
  The "&" character specifies that the following character is an accelerator and will be underlined.
  The "\" character is used to escape the next character in the string. 
  Labels starting with the "_" character cause a divider to be placed after that menu item.
  \par 
  A label of the form "File/Quit" will create the submenu "File"
  with a menu item called "Quit". The "/" character is ignored if it appears
  as the first character of the label string, e.g.  "/File/Quit".
  \par 
  The label string is copied to new memory and can be freed.
  The other arguments (including the shortcut) are copied into the
  menu item unchanged.
  \par 
  If an item exists already with that name then it is replaced with
  this new one.  Otherwise this new one is added to the end of the
  correct menu or submenu.  The return value is the offset into the array
  that the new entry was placed at.
  
  \par shortcut
  The keyboard shortcut for this menu item. 
  \par 
  This parameter is optional, and defaults to 0 to indicate no shortcut.
  \par
  The shortcut can either be a raw integer value (eg. FL_CTRL+'A')
  or a string (eg. "^c" or "^97").
  \par
  Raw integer shortcuts can be a combination of keyboard chars (eg. 'A')
  and optional keyboard modifiers (see Fl::event_state(), e.g. FL_SHIFT, etc).
  In addition, FL_COMMAND can be used to denote FL_META under Mac OS X and
  FL_CTRL under other platforms.
  \par
  String shortcuts can be specified in one of two ways:
  \par
  \verbatim
   [#+^]<ascii_value>    e.g. "97", "^97", "+97", "#97"
   [#+^]<ascii_char>     e.g. "a", "^a", "+a", "#a"
  \endverbatim
  \par
  ..where \<ascii_value\> is a decimal value representing an
  ascii character (eg. 97 is the ascii code for 'a'), and the optional
  prefixes enhance the value that follows. Multiple prefixes must
  appear in the order below.
  \par
  \verbatim
   # - Alt
   + - Shift
   ^ - Control
  \endverbatim
  \par
  Internally, the text shortcuts are converted to integer values using
  fl_old_shortcut(const char*).

  \par callback
  The callback to invoke when this menu item is selected. 
  \par 
  This parameter is optional, and defaults to 0 for no callback.

  \par userdata
  The callback's 'user data' that is passed to the callback. 
  \par 
  This parameter is optional, and defaults to 0.

  \par flags
  These are bit flags to define what kind of menu item this is. 
  \par
  This parameter is optional, and defaults to 0 to define a 'regular' menu item.
  \par 
  These flags can be 'OR'ed together:
  \code
      FL_MENU_INACTIVE     // Deactivate menu item (gray out)
      FL_MENU_TOGGLE       // Item is a checkbox toggle (shows checkbox for on/off state)
      FL_MENU_VALUE        // The on/off state for checkbox/radio buttons (if set, state is 'on')
      FL_MENU_RADIO        // Item is a radio button (one checkbox of many can be on)
      FL_MENU_INVISIBLE    // Item will not show up (shortcut will work)
      FL_SUBMENU_POINTER   // Indicates user_data() is a pointer to another menu array
      FL_SUBMENU           // This item is a submenu to other items
      FL_MENU_DIVIDER      // Creates divider line below this item. Also ends a group of radio buttons.
  \endcode

  \todo Raw integer shortcut needs examples. 
        Dependent on responses to http://fltk.org/newsgroups.php?gfltk.development+v:10086 and results of STR#2344
 */
int Fl_Menu_::add(const char *label,int shortcut,Fl_Callback *callback,void *userdata,int flags) {
  return(insert(-1,label,shortcut,callback,userdata,flags));	// -1: append
}



/**
  Inserts a new menu item at the specified \p index position.

  If \p index is -1, the menu item is appended; same behavior as add().

  To properly insert a menu item, \p label must be the name of the item (eg. "Quit"),
  and not a 'menu pathname' (eg. "File/Quit").  If a menu pathname is specified, 
  the value of \p index is \em ignored, the new item's position defined by the pathname.
  
  For more details, see add(). Except for the \p index parameter, add()
  has more detailed information on parameters and behavior, and is
  functionally equivalent.

  \param[in] index    The menu array's index position where the new item
                      is inserted. If -1, behavior is the same as add().
  \param[in] label    The text label for the menu item. If the label 
                      is a menu pathname, \p index is ignored, and the pathname
		      indicates the position of the new item.
  \param[in] shortcut Optional keyboard shortcut. Can be an int (FL_CTRL+'a')
                      or a string ("^a"). Default is 0.
  \param[in] callback Optional callback invoked when user clicks the item.
                      Default 0 if none.
  \param[in] userdata Optional user data passed as an argument to the callback.
                      Default 0 if none.
  \param[in] flags    Optional flags that control the type of menu item; 
                      see add() for more info. Default is 0 for none.
  \returns            The index into the menu() array, where the entry was added.

  \see                add()

 */
int Fl_Menu_::insert(
  int index,
  const char *label,
  int shortcut,
  Fl_Callback *callback,
  void *userdata,
  int flags
) {
  // make this widget own the local array:
  if (this != fl_menu_array_owner) {
    if (fl_menu_array_owner) {
      Fl_Menu_* o = fl_menu_array_owner;
      // the previous owner get's its own correctly-sized array:
      int value_offset = o->value_-local_array;
      int n = local_array_size;
      Fl_Menu_Item* newMenu = o->menu_ = new Fl_Menu_Item[n];
      memcpy(newMenu, local_array, n*sizeof(Fl_Menu_Item));
      if (o->value_) o->value_ = newMenu+value_offset;
    }
    if (menu_) {
      // this already has a menu array, use it as the local one:
      delete[] local_array;
      if (!alloc) copy(menu_); // duplicate a user-provided static array
      // add to the menu's current array:
      local_array_alloc = local_array_size = size();
      local_array = menu_;
    } else {
      // start with a blank array:
      alloc = 2; // indicates that the strings can be freed
      if (local_array) {
	menu_ = local_array;
      } else {
	local_array_alloc = 15;
	local_array = menu_ = new Fl_Menu_Item[local_array_alloc];
        memset(local_array, 0, sizeof(Fl_Menu_Item) * local_array_alloc);
      }
      memset(menu_, 0, sizeof(Fl_Menu_Item));
      local_array_size = 1;
    }
    fl_menu_array_owner = this;
  }
  int r = menu_->insert(index,label,shortcut,callback,userdata,flags);
  // if it rellocated array we must fix the pointer:
  int value_offset = value_-menu_;
  menu_ = local_array; // in case it reallocated it
  if (value_) value_ = menu_+value_offset;
  return r;
}



/**
  This is a Forms (and SGI GL library) compatible add function, it
  adds many menu items, with '|' separating the menu items, and tab
  separating the menu item names from an optional shortcut string.

  The passed string is split at any '|' characters and then
  add(s,0,0,0,0) is done with each section. This is
  often useful if you are just using the value, and is compatible
  with Forms and other GL programs. The section strings use the
  same special characters as described for the long version of add().

  No items must be added to a menu during a callback to the same menu.
 
  \param str string containing multiple menu labels as described above
  \returns the index into the menu() array, where the entry was added
*/
int Fl_Menu_::add(const char *str) {
  char buf[1024];
  int r = 0;
  while (*str) {
    int sc = 0;
    char *c;
    for (c = buf; c < (buf + sizeof(buf) - 2) && *str && *str != '|'; str++) {
      if (*str == '\t') {*c++ = 0; sc = fl_old_shortcut(str);}
      else *c++ = *str;
    }
    *c = 0;
    r = add(buf, sc, 0, 0, 0);
    if (*str) str++;
  }
  return r;
}



/**
  Changes the text of item \p i.  This is the only way to get
  slash into an add()'ed menu item.  If the menu array was directly set
  with menu(x) then copy() is done to make a private array.
 
  \param i index into menu array
  \param str new label for menu item at index i
*/
void Fl_Menu_::replace(int i, const char *str) {
  if (i<0 || i>=size()) return;
  if (!alloc) copy(menu_);
  if (alloc > 1) {
    free((void *)menu_[i].text);
    str = strdup(str);
  }
  menu_[i].text = str;
}



/**
  Deletes item \p i from the menu.  If the menu array was directly
  set with menu(x) then copy() is done to make a private array.
  
  No items must be removed from a menu during a callback to the same menu.
 
  \param i index into menu array
*/
void Fl_Menu_::remove(int i) {
  int n = size();
  if (i<0 || i>=n) return;
  if (!alloc) copy(menu_);
  // find the next item, skipping submenus:
  Fl_Menu_Item* item = menu_+i;
  const Fl_Menu_Item* next_item = item->next();
  // delete the text only if all items were created with add():
  if (alloc > 1) {
    for (Fl_Menu_Item* m = item; m < next_item; m++)
      if (m->text) free((void*)(m->text));
  }
  // MRS: "n" is the menu size(), which includes the trailing NULL entry...
  memmove(item, next_item, (menu_+n-next_item)*sizeof(Fl_Menu_Item));
}

//
// End of "$Id: Fl_Menu_add.cxx 8110 2010-12-23 08:02:52Z manolo $".
//
