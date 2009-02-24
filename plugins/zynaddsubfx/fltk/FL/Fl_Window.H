//
// "$Id: Fl_Window.H 4421 2005-07-15 09:34:53Z matt $"
//
// Window header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Window_H
#define Fl_Window_H

#include "Fl_Group.H"

#define FL_WINDOW 0xF0	// all subclasses have type() >= this
#define FL_DOUBLE_WINDOW 0xF1

class Fl_X;

class FL_EXPORT Fl_Window : public Fl_Group {

  friend class Fl_X;
  Fl_X *i; // points at the system-specific stuff

  const char* iconlabel_;
  const char* xclass_;
  const void* icon_;
  // size_range stuff:
  short minw, minh, maxw, maxh;
  uchar dw, dh, aspect, size_range_set;
  // cursor stuff
  Fl_Cursor cursor_default;
  Fl_Color cursor_fg, cursor_bg;
  void size_range_();
  // values for flags():
  enum {
    FL_MODAL = 64,
    FL_NOBORDER = 8,
    FL_FORCE_POSITION = 16,
    FL_NON_MODAL = 32,
    FL_OVERRIDE = 256
  };
  void _Fl_Window(); // constructor innards

  // unimplemented copy ctor and assignment operator
  Fl_Window(const Fl_Window&);
  Fl_Window& operator=(const Fl_Window&);

protected:

  static Fl_Window *current_;
  virtual void draw();
  virtual void flush();

public:

  Fl_Window(int,int,int,int, const char* = 0);
  Fl_Window(int,int, const char* = 0);
  virtual ~Fl_Window();

  virtual int handle(int);

  virtual void resize(int,int,int,int);
  void border(int b);
  void clear_border()	{set_flag(FL_NOBORDER);}
  int border() const	{return !(flags() & FL_NOBORDER);}
  void set_override()	{set_flag(FL_NOBORDER|FL_OVERRIDE);}
  int override() const  { return flags()&FL_OVERRIDE; }
  void set_modal()	{set_flag(FL_MODAL);}
  int modal() const	{return flags() & FL_MODAL;}
  void set_non_modal()	{set_flag(FL_NON_MODAL);}
  int non_modal() const {return flags() & (FL_NON_MODAL|FL_MODAL);}

  void hotspot(int x, int y, int offscreen = 0);
  void hotspot(const Fl_Widget*, int offscreen = 0);
  void hotspot(const Fl_Widget& p, int offscreen = 0) {hotspot(&p,offscreen);}
  void free_position()	{clear_flag(FL_FORCE_POSITION);}
  void size_range(int a, int b, int c=0, int d=0, int e=0, int f=0, int g=0) {
    minw=(short)a; minh=(short)b; maxw=(short)c; maxh=(short)d; dw=(uchar)e; dh=(uchar)f; aspect=(uchar)g; size_range_();}

  const char* label() const	{return Fl_Widget::label();}
  const char* iconlabel() const	{return iconlabel_;}
  void label(const char*);
  void iconlabel(const char*);
  void label(const char* label, const char* iconlabel);
  void copy_label(const char* a);
  const char* xclass() const	{return xclass_;}
  void xclass(const char* c)	{xclass_ = c;}
  const void* icon() const	{return icon_;}
  void icon(const void * ic)	{icon_ = ic;}

  int shown() {return i != 0;}
  virtual void show();
  virtual void hide();
  void show(int, char**);
  void fullscreen();
  void fullscreen_off(int,int,int,int);
  void iconize();

  int x_root() const ;
  int y_root() const ;

  static Fl_Window *current();
  void make_current();

  // for back-compatability only:
  void cursor(Fl_Cursor, Fl_Color=FL_BLACK, Fl_Color=FL_WHITE);
  void default_cursor(Fl_Cursor, Fl_Color=FL_BLACK, Fl_Color=FL_WHITE);
  static void default_callback(Fl_Window*, void* v);

};

#endif

//
// End of "$Id: Fl_Window.H 4421 2005-07-15 09:34:53Z matt $".
//
