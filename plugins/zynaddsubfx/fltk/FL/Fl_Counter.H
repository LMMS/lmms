//
// "$Id: Fl_Counter.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Counter header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Counter widget . */

// A numerical value with up/down step buttons.  From Forms.

#ifndef Fl_Counter_H
#define Fl_Counter_H

#ifndef Fl_Valuator_H
#include "Fl_Valuator.H"
#endif

// values for type():
#define FL_NORMAL_COUNTER	0	/**< type() for counter with fast buttons */
#define FL_SIMPLE_COUNTER	1	/**< type() for counter without fast buttons */

/**
  Controls a single floating point value with button (or keyboard) arrows.
  Double arrows buttons achieve larger steps than simple arrows.
  \see Fl_Spinner for value input with vertical step arrows.
  <P align=center>\image html counter.png</P>
  \image latex counter.png "Fl_Counter" width=4cm

  \todo Refactor the doxygen comments for Fl_Counter type() documentation.

  The type of an Fl_Counter object can be set using type(uchar t) to:
  \li \c FL_NORMAL_COUNTER: Displays a counter with 4 arrow buttons.
  \li \c FL_SIMPLE_COUNTER: Displays a counter with only 2 arrow buttons.
*/
class FL_EXPORT Fl_Counter : public Fl_Valuator {

  Fl_Font textfont_;
  Fl_Fontsize textsize_;
  Fl_Color textcolor_;
  double lstep_;
  uchar mouseobj;
  static void repeat_callback(void *);
  int calc_mouseobj();
  void increment_cb();

protected:

  void draw();

public:

  int handle(int);

  Fl_Counter(int X, int Y, int W, int H, const char* L = 0);
  ~Fl_Counter();

  /**
    Sets the increment for the large step buttons.
    The default value is 1.0.
    \param[in] a large step increment.
  */
  void lstep(double a) {lstep_ = a;}

  /**
    Sets the increments for the normal and large step buttons.
    \param[in] a, b normal and large step increments.
  */
  void step(double a,double b) {Fl_Valuator::step(a); lstep_ = b;}

  /**
    Sets the increment for the normal step buttons.
    \param[in] a normal step increment.
  */
  void step(double a) {Fl_Valuator::step(a);}

  /**
    Returns the increment for normal step buttons.
   */
  double step() const {return Fl_Valuator::step();}

  /** Gets the text font */
  Fl_Font textfont() const {return textfont_;}
  /** Sets the text font to \p s */
  void textfont(Fl_Font s) {textfont_ = s;}

  /** Gets the font size */
  Fl_Fontsize textsize() const {return textsize_;}
  /** Sets the font size to \p s */
  void textsize(Fl_Fontsize s) {textsize_ = s;}

  /** Gets the font color */
  Fl_Color textcolor() const {return textcolor_;}
  /** Sets the font color to \p s */
  void textcolor(Fl_Color s) {textcolor_ = s;}

};

#endif

//
// End of "$Id: Fl_Counter.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
