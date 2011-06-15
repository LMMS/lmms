//
// "$Id: Fl_Menu_Item.H 7983 2010-12-09 00:04:06Z AlbrechtS $"
//
// Menu item header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Menu_Item_H
#define Fl_Menu_Item_H

#  include "Fl_Widget.H"
#  include "Fl_Image.H"

#  if defined(__APPLE__) && defined(check)
#    undef check
#  endif

enum { // values for flags:
  FL_MENU_INACTIVE = 1,		///< Deactivate menu item (gray out)
  FL_MENU_TOGGLE= 2,		///< Item is a checkbox toggle (shows checkbox for on/off state)
  FL_MENU_VALUE = 4,		///< The on/off state for checkbox/radio buttons (if set, state is 'on')
  FL_MENU_RADIO = 8,		///< Item is a radio button (one checkbox of many can be on)
  FL_MENU_INVISIBLE = 0x10,	///< Item will not show up (shortcut will work)
  FL_SUBMENU_POINTER = 0x20,	///< Indicates user_data() is a pointer to another menu array
  FL_SUBMENU = 0x40,		///< This item is a submenu to other items
  FL_MENU_DIVIDER = 0x80,	///< Creates divider line below this item. Also ends a group of radio buttons.
  FL_MENU_HORIZONTAL = 0x100	///< ??? -- reserved
};

extern FL_EXPORT Fl_Shortcut fl_old_shortcut(const char*);

class Fl_Menu_;

/**
  The Fl_Menu_Item structure defines a single menu item that
  is used by the Fl_Menu_ class.  
  \code
  struct Fl_Menu_Item {
   const char*		text;     // label()
   ulong		shortcut_;
   Fl_Callback*		callback_;
   void*		user_data_;
   int			flags;
   uchar		labeltype_;
   uchar		labelfont_;
   uchar		labelsize_;
   uchar		labelcolor_;
  };
  
  enum { // values for flags:
   FL_MENU_INACTIVE	= 1,      // Deactivate menu item (gray out)
   FL_MENU_TOGGLE	= 2,      // Item is a checkbox toggle (shows checkbox for on/off state)
   FL_MENU_VALUE	= 4,      // The on/off state for checkbox/radio buttons (if set, state is 'on')
   FL_MENU_RADIO	= 8,      // Item is a radio button (one checkbox of many can be on)
   FL_MENU_INVISIBLE	= 0x10,   // Item will not show up (shortcut will work)
   FL_SUBMENU_POINTER	= 0x20,   // Indicates user_data() is a pointer to another menu array
   FL_SUBMENU		= 0x40,   // This item is a submenu to other items
   FL_MENU_DIVIDER	= 0x80,   // Creates divider line below this item. Also ends a group of radio buttons.
   FL_MENU_HORIZONTAL	= 0x100   // ??? -- reserved
  };
  \endcode
  Typically menu items are statically defined; for example:
  \code
  Fl_Menu_Item popup[] = {
   {"&alpha",   FL_ALT+'a', the_cb, (void*)1},
   {"&beta",    FL_ALT+'b', the_cb, (void*)2},
   {"gamma",    FL_ALT+'c', the_cb, (void*)3, FL_MENU_DIVIDER},
   {"&strange",  0,   strange_cb},
   {"&charm",    0,   charm_cb},
   {"&truth",    0,   truth_cb},
   {"b&eauty",   0,   beauty_cb},
   {"sub&menu",  0,   0, 0, FL_SUBMENU},
   {"one"},
   {"two"},
   {"three"},
   {0},
   {"inactive", FL_ALT+'i', 0, 0, FL_MENU_INACTIVE|FL_MENU_DIVIDER},
   {"invisible",FL_ALT+'i', 0, 0, FL_MENU_INVISIBLE},
   {"check",    FL_ALT+'i', 0, 0, FL_MENU_TOGGLE|FL_MENU_VALUE},
   {"box",      FL_ALT+'i', 0, 0, FL_MENU_TOGGLE},
   {0}};
  \endcode
  produces:

  \image html   menu.png
  \image latex  menu.png "menu" width=10cm

  A submenu title is identified by the bit FL_SUBMENU in the 
  flags field, and ends with a label() that is NULL.
  You can nest menus to any depth.  A pointer to the first item in the
  submenu can be treated as an Fl_Menu array itself.  It is also
  possible to make separate submenu arrays with FL_SUBMENU_POINTER flags.

  You should use the method functions to access structure members and
  not access them directly to avoid compatibility problems with future
  releases of FLTK.
*/
struct FL_EXPORT Fl_Menu_Item {
  const char *text;	    ///< menu item text, returned by label()
  int shortcut_;	    ///< menu item shortcut
  Fl_Callback *callback_;   ///< menu item callback
  void *user_data_;	    ///< menu item user_data for the menu's callback
  int flags;		    ///< menu item flags like FL_MENU_TOGGLE, FL_MENU_RADIO
  uchar labeltype_;	    ///< how the menu item text looks like
  Fl_Font labelfont_;	    ///< which font for this menu item text
  Fl_Fontsize labelsize_;   ///< size of menu item text
  Fl_Color labelcolor_;	    ///< menu item text color

  // advance N items, skipping submenus:
  const Fl_Menu_Item *next(int=1) const;

  /**
    Advances a pointer by n items through a menu array, skipping
    the contents of submenus and invisible items. There are two calls so
    that you can advance through const and non-const data.
  */
  Fl_Menu_Item *next(int i=1) {
    return (Fl_Menu_Item*)(((const Fl_Menu_Item*)this)->next(i));}

  /** Returns the first menu item, same as next(0). */
  const Fl_Menu_Item *first() const { return next(0); }

  /** Returns the first menu item, same as next(0). */
  Fl_Menu_Item *first() { return next(0); }

  // methods on menu items:
  /**
    Returns the title of the item.
    A NULL here indicates the end of the menu (or of a submenu).
    A '&' in the item will print an underscore under the next letter,
    and if the menu is popped up that letter will be a "shortcut" to pick
    that item.  To get a real '&' put two in a row.
  */
  const char* label() const {return text;}

  /**    See const char* Fl_Menu_Item::label() const   */
  void label(const char* a) {text=a;}

  /**    See const char* Fl_Menu_Item::label() const   */
  void label(Fl_Labeltype a,const char* b) {labeltype_ = a; text = b;}

  /**
    Returns the menu item's labeltype.
    A labeltype identifies a routine that draws the label of the
    widget.  This can be used for special effects such as emboss, or to use
    the label() pointer as another form of data such as a bitmap.
    The value FL_NORMAL_LABEL prints the label as text.
  */
  Fl_Labeltype labeltype() const {return (Fl_Labeltype)labeltype_;}

  /**
    Sets the menu item's labeltype.
    A labeltype identifies a routine that draws the label of the
    widget.  This can be used for special effects such as emboss, or to use
    the label() pointer as another form of data such as a bitmap.
    The value FL_NORMAL_LABEL prints the label as text.
  */
  void labeltype(Fl_Labeltype a) {labeltype_ = a;}

  /**
    Gets the menu item's label color.
    This color is passed to the labeltype routine, and is typically the
    color of the label text.  This defaults to FL_BLACK.  If this
    color is not black fltk will \b not use overlay bitplanes to draw
    the menu - this is so that images put in the menu draw correctly.
  */
  Fl_Color labelcolor() const {return labelcolor_;}

  /**
    Sets the menu item's label color.
    \see Fl_Color Fl_Menu_Item::labelcolor() const
  */
  void labelcolor(Fl_Color a) {labelcolor_ = a;}
  /**
    Gets the menu item's label font.
    Fonts are identified by small 8-bit indexes into a table. See the
    enumeration list for predefined fonts. The default value is a
    Helvetica font. The function Fl::set_font() can define new fonts.
  */
  Fl_Font labelfont() const {return labelfont_;}

  /**
    Sets the menu item's label font.
    Fonts are identified by small 8-bit indexes into a table. See the
    enumeration list for predefined fonts. The default value is a
    Helvetica font.  The function Fl::set_font() can define new fonts.
  */
  void labelfont(Fl_Font a) {labelfont_ = a;}

  /** Gets the label font pixel size/height. */
  Fl_Fontsize labelsize() const {return labelsize_;}

  /** Sets the label font pixel size/height.*/
  void labelsize(Fl_Fontsize a) {labelsize_ = a;}

  /**
    Returns the callback function that is set for the menu item.
    Each item has space for a callback function and an argument for that
    function. Due to back compatibility, the Fl_Menu_Item itself
    is not passed to the callback, instead you have to get it by calling
    ((Fl_Menu_*)w)->mvalue() where w is the widget argument.
  */
  Fl_Callback_p callback() const {return callback_;}

  /**
    Sets the menu item's callback function and userdata() argument.
    \see Fl_Callback_p Fl_MenuItem::callback() const
  */
  void callback(Fl_Callback* c, void* p) {callback_=c; user_data_=p;}

  /**
    Sets the menu item's callback function.
    This method does not set the userdata() argument.
    \see Fl_Callback_p Fl_MenuItem::callback() const
   */
  void callback(Fl_Callback* c) {callback_=c;}

  /**
    Sets the menu item's callback function.
    This method does not set the userdata() argument.
    \see Fl_Callback_p Fl_MenuItem::callback() const
   */
  void callback(Fl_Callback0*c) {callback_=(Fl_Callback*)c;}

  /**
    Sets the menu item's callback function and userdata() argument.
    This method does not set the userdata() argument.
    The argument \p is cast to void* and stored as the userdata()
    for the menu item's callback function.
    \see Fl_Callback_p Fl_MenuItem::callback() const
   */
  void callback(Fl_Callback1*c, long p=0) {callback_=(Fl_Callback*)c; user_data_=(void*)p;}

  /**
    Gets the user_data() argument that is sent to the callback function.
  */
  void* user_data() const {return user_data_;}
  /**
    Sets the user_data() argument that is sent to the callback function.
  */
  void user_data(void* v) {user_data_ = v;}
  /**
    Gets the user_data() argument that is sent to the callback function.
    For convenience you can also define the callback as taking a long
    argument.  This method casts the stored userdata() argument to long
    and returns it as a \e long value.
  */
  long argument() const {return (long)(fl_intptr_t)user_data_;}
  /**
    Sets the user_data() argument that is sent to the callback function.
    For convenience you can also define the callback as taking a long
    argument.  This method casts the given argument \p v to void*
    and stores it in the menu item's userdata() member.
    This may not be portable to some machines.
  */
  void argument(long v) {user_data_ = (void*)v;}

  /** Gets what key combination shortcut will trigger the menu item. */
  int shortcut() const {return shortcut_;}

  /**
    Sets exactly what key combination will trigger the menu item.  The
    value is a logical 'or' of a key and a set of shift flags, for instance 
    FL_ALT+'a' or FL_ALT+FL_F+10 or just 'a'.  A value of
    zero disables the shortcut.

    The key can be any value returned by Fl::event_key(), but will usually 
    be an ASCII letter. Use a lower-case letter unless you require the shift 
    key to be held down.

    The shift flags can be any set of values accepted by Fl::event_state().
    If the bit is on that shift key must be pushed.  Meta, Alt, Ctrl, 
    and Shift must be off if they are not in the shift flags (zero for the 
    other bits indicates a "don't care" setting).
  */
  void shortcut(int s) {shortcut_ = s;}
  /**
    Returns true if either FL_SUBMENU or FL_SUBMENU_POINTER
    is on in the flags. FL_SUBMENU indicates an embedded submenu
    that goes from the next item through the next one with a NULL
    label(). FL_SUBMENU_POINTER indicates that user_data()
    is a pointer to another menu array.
  */
  int submenu() const {return flags&(FL_SUBMENU|FL_SUBMENU_POINTER);}
  /**
    Returns true if a checkbox will be drawn next to this item.
    This is true if FL_MENU_TOGGLE or FL_MENU_RADIO is set in the flags.
  */
  int checkbox() const {return flags&FL_MENU_TOGGLE;}
  /**
    Returns true if this item is a radio item.
    When a radio button is selected all "adjacent" radio buttons are
    turned off.  A set of radio items is delimited by an item that has
    radio() false, or by an item with FL_MENU_DIVIDER turned on.
  */
  int radio() const {return flags&FL_MENU_RADIO;}
  /** Returns the current value of the check or radio item. */
  int value() const {return flags&FL_MENU_VALUE;}
  /**
    Turns the check or radio item "on" for the menu item. Note that this
    does not turn off any adjacent radio items like set_only() does.
  */
  void set() {flags |= FL_MENU_VALUE;}

  /** Turns the check or radio item "off" for the menu item. */
  void clear() {flags &= ~FL_MENU_VALUE;}

  void setonly();

  /** Gets the visibility of an item. */
  int visible() const {return !(flags&FL_MENU_INVISIBLE);}

  /** Makes an item visible in the menu. */
  void show() {flags &= ~FL_MENU_INVISIBLE;}

  /** Hides an item in the menu. */
  void hide() {flags |= FL_MENU_INVISIBLE;}

  /** Gets whether or not the item can be picked. */
  int active() const {return !(flags&FL_MENU_INACTIVE);}

  /** Allows a menu item to be picked. */
  void activate() {flags &= ~FL_MENU_INACTIVE;}
  /**
    Prevents a menu item from being picked. Note that this will also cause
    the menu item to appear grayed-out.
  */
  void deactivate() {flags |= FL_MENU_INACTIVE;}
  /** Returns non 0 if FL_INACTIVE and FL_INVISIBLE are cleared, 0 otherwise. */
  int activevisible() const {return !(flags & (FL_MENU_INACTIVE|FL_MENU_INVISIBLE));}

  // compatibility for FLUID so it can set the image of a menu item...

  /** compatibility api for FLUID, same as a->label(this) */
  void image(Fl_Image* a) {a->label(this);}

  /** compatibility api for FLUID, same as a.label(this) */
  void image(Fl_Image& a) {a.label(this);}

  // used by menubar:
  int measure(int* h, const Fl_Menu_*) const;
  void draw(int x, int y, int w, int h, const Fl_Menu_*, int t=0) const;

  // popup menus without using an Fl_Menu_ widget:
  const Fl_Menu_Item* popup(
    int X, int Y,
    const char *title = 0,
    const Fl_Menu_Item* picked=0,
    const Fl_Menu_* = 0) const;
  const Fl_Menu_Item* pulldown(
    int X, int Y, int W, int H,
    const Fl_Menu_Item* picked = 0,
    const Fl_Menu_* = 0,
    const Fl_Menu_Item* title = 0,
    int menubar=0) const;
  const Fl_Menu_Item* test_shortcut() const;
  const Fl_Menu_Item* find_shortcut(int *ip=0, const bool require_alt = false) const;

  /**
    Calls the Fl_Menu_Item item's callback, and provides the Fl_Widget argument.
    The callback is called with the stored user_data() as its second argument.
    You must first check that callback() is non-zero before calling this.
  */
  void do_callback(Fl_Widget* o) const {callback_(o, user_data_);}

  /**
    Calls the Fl_Menu_Item item's callback, and provides the Fl_Widget argument.
    This call overrides the callback's second argument with the given value \p arg.
    You must first check that callback() is non-zero before calling this.
  */
  void do_callback(Fl_Widget* o,void* arg) const {callback_(o, arg);}

  /**
    Calls the Fl_Menu_Item item's callback, and provides the Fl_Widget argument.
    This call overrides the callback's second argument with the
    given value \p arg. long \p arg is cast to void* when calling
    the callback.
    You must first check that callback() is non-zero before calling this.
  */
  void do_callback(Fl_Widget* o,long arg) const {callback_(o, (void*)arg);}

  // back-compatibility, do not use:

  /** back compatibility only \deprecated. */
  int checked() const {return flags&FL_MENU_VALUE;}

  /** back compatibility only \deprecated. */
  void check() {flags |= FL_MENU_VALUE;}

  /** back compatibility only \deprecated. */
  void uncheck() {flags &= ~FL_MENU_VALUE;}

  int insert(int,const char*,int,Fl_Callback*,void* =0, int =0);
  int add(const char*, int shortcut, Fl_Callback*, void* =0, int = 0);

  /** See int add(const char*, int shortcut, Fl_Callback*, void*, int) */
  int add(const char*a, const char* b, Fl_Callback* c,
	  void* d = 0, int e = 0) {
    return add(a,fl_old_shortcut(b),c,d,e);}

  int size() const ;
};

typedef Fl_Menu_Item Fl_Menu; // back compatibility

enum {	// back-compatibility enum:
  FL_PUP_NONE	= 0,
  FL_PUP_GREY	= FL_MENU_INACTIVE,
  FL_PUP_GRAY	= FL_MENU_INACTIVE,
  FL_MENU_BOX	= FL_MENU_TOGGLE,
  FL_PUP_BOX	= FL_MENU_TOGGLE,
  FL_MENU_CHECK	= FL_MENU_VALUE,
  FL_PUP_CHECK	= FL_MENU_VALUE,
  FL_PUP_RADIO	= FL_MENU_RADIO,
  FL_PUP_INVISIBLE = FL_MENU_INVISIBLE,
  FL_PUP_SUBMENU = FL_SUBMENU_POINTER
};

#endif

//
// End of "$Id: Fl_Menu_Item.H 7983 2010-12-09 00:04:06Z AlbrechtS $".
//
