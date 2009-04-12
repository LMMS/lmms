//
// "$Id: Fl_Menu_Item.H 4288 2005-04-16 00:13:17Z mike $"
//
// Menu item header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Menu_Item_H
#define Fl_Menu_Item_H

#  include "Fl_Widget.H"
#  include "Fl_Image.H"

#  if defined(__APPLE__) && defined(check)
#    undef check
#  endif

enum { // values for flags:
  FL_MENU_INACTIVE = 1,
  FL_MENU_TOGGLE= 2,
  FL_MENU_VALUE = 4,
  FL_MENU_RADIO = 8,
  FL_MENU_INVISIBLE = 0x10,
  FL_SUBMENU_POINTER = 0x20,
  FL_SUBMENU = 0x40,
  FL_MENU_DIVIDER = 0x80,
  FL_MENU_HORIZONTAL = 0x100
};

extern FL_EXPORT int fl_old_shortcut(const char*);

class Fl_Menu_;

struct FL_EXPORT Fl_Menu_Item {
  const char *text;	// label()
  int shortcut_;
  Fl_Callback *callback_;
  void *user_data_;
  int flags;
  uchar labeltype_;
  uchar labelfont_;
  uchar labelsize_;
  unsigned labelcolor_;

  // advance N items, skipping submenus:
  const Fl_Menu_Item *next(int=1) const;
  Fl_Menu_Item *next(int i=1) {
    return (Fl_Menu_Item*)(((const Fl_Menu_Item*)this)->next(i));}
  const Fl_Menu_Item *first() const { return next(0); }
  Fl_Menu_Item *first() { return next(0); }

  // methods on menu items:
  const char* label() const {return text;}
  void label(const char* a) {text=a;}
  void label(Fl_Labeltype a,const char* b) {labeltype_ = a; text = b;}
  Fl_Labeltype labeltype() const {return (Fl_Labeltype)labeltype_;}
  void labeltype(Fl_Labeltype a) {labeltype_ = a;}
  Fl_Color labelcolor() const {return (Fl_Color)labelcolor_;}
  void labelcolor(unsigned a) {labelcolor_ = a;}
  Fl_Font labelfont() const {return (Fl_Font)labelfont_;}
  void labelfont(uchar a) {labelfont_ = a;}
  uchar labelsize() const {return labelsize_;}
  void labelsize(uchar a) {labelsize_ = a;}
  Fl_Callback_p callback() const {return callback_;}
  void callback(Fl_Callback* c, void* p) {callback_=c; user_data_=p;}
  void callback(Fl_Callback* c) {callback_=c;}
  void callback(Fl_Callback0*c) {callback_=(Fl_Callback*)c;}
  void callback(Fl_Callback1*c, long p=0) {callback_=(Fl_Callback*)c; user_data_=(void*)p;}
  void* user_data() const {return user_data_;}
  void user_data(void* v) {user_data_ = v;}
  long argument() const {return (long)user_data_;}
  void argument(long v) {user_data_ = (void*)v;}
  int shortcut() const {return shortcut_;}
  void shortcut(int s) {shortcut_ = s;}
  int submenu() const {return flags&(FL_SUBMENU|FL_SUBMENU_POINTER);}
  int checkbox() const {return flags&FL_MENU_TOGGLE;}
  int radio() const {return flags&FL_MENU_RADIO;}
  int value() const {return flags&FL_MENU_VALUE;}
  void set() {flags |= FL_MENU_VALUE;}
  void clear() {flags &= ~FL_MENU_VALUE;}
  void setonly();
  int visible() const {return !(flags&FL_MENU_INVISIBLE);}
  void show() {flags &= ~FL_MENU_INVISIBLE;}
  void hide() {flags |= FL_MENU_INVISIBLE;}
  int active() const {return !(flags&FL_MENU_INACTIVE);}
  void activate() {flags &= ~FL_MENU_INACTIVE;}
  void deactivate() {flags |= FL_MENU_INACTIVE;}
  int activevisible() const {return !(flags&0x11);}

  // compatibility for FLUID so it can set the image of a menu item...
  void image(Fl_Image* a) {a->label(this);}
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
  const Fl_Menu_Item* find_shortcut(int *ip=0) const;

  void do_callback(Fl_Widget* o) const {callback_(o, user_data_);}
  void do_callback(Fl_Widget* o,void* arg) const {callback_(o, arg);}
  void do_callback(Fl_Widget* o,long arg) const {callback_(o, (void*)arg);}

  // back-compatability, do not use:
  int checked() const {return flags&FL_MENU_VALUE;}
  void check() {flags |= FL_MENU_VALUE;}
  void uncheck() {flags &= ~FL_MENU_VALUE;}
  int add(const char*, int shortcut, Fl_Callback*, void* =0, int = 0);
  int add(const char*a, const char* b, Fl_Callback* c,
	  void* d = 0, int e = 0) {
    return add(a,fl_old_shortcut(b),c,d,e);}
  int size() const ;
};

typedef Fl_Menu_Item Fl_Menu; // back compatability

enum {	// back-compatability enum:
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
// End of "$Id: Fl_Menu_Item.H 4288 2005-04-16 00:13:17Z mike $".
//
