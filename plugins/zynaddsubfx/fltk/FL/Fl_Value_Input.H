//
// "$Id: Fl_Value_Input.H 4288 2005-04-16 00:13:17Z mike $"
//
// Value input header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Value_Input_H
#define Fl_Value_Input_H

#include "Fl_Valuator.H"
#include "Fl_Input.H"

class FL_EXPORT Fl_Value_Input : public Fl_Valuator {
public:
  Fl_Input input;
private:
  char soft_;
  static void input_cb(Fl_Widget*,void*);
  virtual void value_damage(); // cause damage() due to value() changing
public:
  int handle(int);
  void draw();
  void resize(int,int,int,int);
  Fl_Value_Input(int x,int y,int w,int h,const char *l=0);

  void soft(char s) {soft_ = s;}
  char soft() const {return soft_;}

  Fl_Font textfont() const {return input.textfont();}
  void textfont(uchar s) {input.textfont(s);}
  uchar textsize() const {return input.textsize();}
  void textsize(uchar s) {input.textsize(s);}
  Fl_Color textcolor() const {return input.textcolor();}
  void textcolor(unsigned n) {input.textcolor(n);}
  Fl_Color cursor_color() const {return input.cursor_color();}
  void cursor_color(unsigned n) {input.cursor_color(n);}

};

#endif

//
// End of "$Id: Fl_Value_Input.H 4288 2005-04-16 00:13:17Z mike $".
//
