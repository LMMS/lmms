//
// "$Id: Fl_Menu_.H 4288 2005-04-16 00:13:17Z mike $"
//
// Menu base class header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Menu__H
#define Fl_Menu__H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif
#include "Fl_Menu_Item.H"

class FL_EXPORT Fl_Menu_ : public Fl_Widget {

  Fl_Menu_Item *menu_;
  const Fl_Menu_Item *value_;

protected:

  uchar alloc;
  uchar down_box_;
  uchar textfont_;
  uchar textsize_;
  unsigned textcolor_;

public:
  Fl_Menu_(int,int,int,int,const char * =0);
  ~Fl_Menu_();

  int item_pathname(char *name, int namelen, const Fl_Menu_Item *finditem=0) const;
  const Fl_Menu_Item* picked(const Fl_Menu_Item*);
  const Fl_Menu_Item* find_item(const char *name);

  const Fl_Menu_Item* test_shortcut() {return picked(menu()->test_shortcut());}
  void global();

  const Fl_Menu_Item *menu() const {return menu_;}
  void menu(const Fl_Menu_Item *m);
  void copy(const Fl_Menu_Item *m, void* user_data = 0);
  int  add(const char*, int shortcut, Fl_Callback*, void* = 0, int = 0);
  int  add(const char* a, const char* b, Fl_Callback* c,
	  void* d = 0, int e = 0) {return add(a,fl_old_shortcut(b),c,d,e);}
  int  size() const ;
  void size(int W, int H) { Fl_Widget::size(W, H); }
  void clear();
  int  add(const char *);
  void replace(int,const char *);
  void remove(int);
  void shortcut(int i, int s) {menu_[i].shortcut(s);}
  void mode(int i,int fl) {menu_[i].flags = fl;}
  int  mode(int i) const {return menu_[i].flags;}

  const Fl_Menu_Item *mvalue() const {return value_;}
  int value() const {return value_ ? (int)(value_-menu_) : -1;}
  int value(const Fl_Menu_Item*);
  int value(int i) {return value(menu_+i);}
  const char *text() const {return value_ ? value_->text : 0;}
  const char *text(int i) const {return menu_[i].text;}

  Fl_Font textfont() const {return (Fl_Font)textfont_;}
  void textfont(uchar c) {textfont_=c;}
  uchar textsize() const {return textsize_;}
  void textsize(uchar c) {textsize_=c;}
  Fl_Color textcolor() const {return (Fl_Color)textcolor_;}
  void textcolor(unsigned c) {textcolor_=c;}

  Fl_Boxtype down_box() const {return (Fl_Boxtype)down_box_;}
  void down_box(Fl_Boxtype b) {down_box_ = b;}

  // back compatability:
  Fl_Color down_color() const {return selection_color();}
  void down_color(unsigned c) {selection_color(c);}
};

#endif

//
// End of "$Id: Fl_Menu_.H 4288 2005-04-16 00:13:17Z mike $".
//
