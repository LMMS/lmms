//
// "$Id: Fl_Value_Input.H 8337 2011-01-30 09:04:59Z manolo $"
//
// Value input header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Value_Input widget . */

#ifndef Fl_Value_Input_H
#define Fl_Value_Input_H

#include "Fl_Valuator.H"
#include "Fl_Input.H"

/**
  The Fl_Value_Input widget displays a numeric value.
  The user can click in the text field and edit it - there is in
  fact a hidden Fl_Input widget with
  type(FL_FLOAT_INPUT) or type(FL_INT_INPUT) in
  there - and when they hit return or tab the value updates to
  what they typed and the callback is done.
  
  <P>If step() is non-zero and integral, then the range of numbers
  is limited to integers instead of floating point numbers. As
  well as displaying the value as an integer, typed input is also
  limited to integer values, even if the hidden Fl_Input widget
  is of type(FL_FLOAT_INPUT).</P>
  
  <P>If step() is non-zero, the user can also drag the
  mouse across the object and thus slide the value. The left
  button moves one step() per pixel, the middle by 10
  * step(), and the right button by 100 * step(). It
  is therefore impossible to select text by dragging across it,
  although clicking can still move the insertion cursor.</P>
  
  <P>If step() is non-zero and integral, then the range
  of numbers are limited to integers instead of floating point
  values.
  
  <P ALIGN="CENTER">\image html Fl_Value_Input.png 
  \image latex  Fl_Value_Input.png "Fl_Value_Input" width=4cm
*/
class FL_EXPORT Fl_Value_Input : public Fl_Valuator {
public:
  /* This is the encapsulated Fl_input attribute to which 
  this class delegates the value font, color and shortcut */
  Fl_Input input;
private:
  char soft_;
  static void input_cb(Fl_Widget*,void*);
  virtual void value_damage(); // cause damage() due to value() changing
public:
  int handle(int);
protected:
  void draw();
public:
  void resize(int,int,int,int);
  Fl_Value_Input(int x,int y,int w,int h,const char *l=0);
  ~Fl_Value_Input();

  /** See void Fl_Value_Input::soft(char s) */
  void soft(char s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag
    the value outside the range. If they drag the value to one of
    the ends, let go, then grab again and continue to drag, they can
    get to any value. The default is true.
  */
  char soft() const {return soft_;}
  /**
   Returns the current shortcut key for the Input.
   \see Fl_Value_Input::shortcut(int) 
  */
  int shortcut() const {return input.shortcut();}
  /** 
   Sets the shortcut key to \p s. Setting this
   overrides the use of '&' in the label().  The value is a bitwise
   OR of a key and a set of shift flags, for example FL_ALT | 'a'
   , FL_ALT | (FL_F + 10), or just 'a'.  A value
   of 0 disables the shortcut.
   
   The key can be any value returned by 
   Fl::event_key(), but will usually be an ASCII letter.  Use
   a lower-case letter unless you require the shift key to be held down.
   
   The shift flags can be any set of values accepted by 
   Fl::event_state().  If the bit is on that shift key must
   be pushed.  Meta, Alt, Ctrl, and Shift must be off if they are not in
   the shift flags (zero for the other bits indicates a "don't care"
   setting).
   */
  void shortcut(int s) {input.shortcut(s);}

  /** Gets the typeface of the text in the value box.  */
  Fl_Font textfont() const {return input.textfont();}
  /** Sets the typeface of the text in the value box.  */
  void textfont(Fl_Font s) {input.textfont(s);}
  /** Gets the size of the text in the value box.  */
  Fl_Fontsize textsize() const {return input.textsize();}
  /** Sets the size of the text in the value box.  */
  void textsize(Fl_Fontsize s) {input.textsize(s);}
  /** Gets the color of the text in the value box.  */
  Fl_Color textcolor() const {return input.textcolor();}
  /** Sets the color of the text in the value box.*/
  void textcolor(Fl_Color n) {input.textcolor(n);}
  /** Gets the color of the text cursor. The text cursor is black by default. */
  Fl_Color cursor_color() const {return input.cursor_color();}
  /** Sets the color of the text cursor. The text cursor is black by default. */
  void cursor_color(Fl_Color n) {input.cursor_color(n);}

};

#endif

//
// End of "$Id: Fl_Value_Input.H 8337 2011-01-30 09:04:59Z manolo $".
//
