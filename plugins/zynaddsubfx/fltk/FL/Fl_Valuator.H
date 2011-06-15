//
// "$Id: Fl_Valuator.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Valuator header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Valuator widget . */

#ifndef Fl_Valuator_H
#define Fl_Valuator_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

// shared type() values for classes that work in both directions:
#define FL_VERTICAL		0 ///< The valuator can work vertically
#define FL_HORIZONTAL		1 ///< The valuator can work horizontally

/**
  The Fl_Valuator class controls a single floating-point value
  and provides a consistent interface to set the value, range, and step,
  and insures that callbacks are done the same for every object.
  <P>There are probably more of these classes in FLTK than any others:
  <P ALIGN=CENTER>\image html  valuators.png</P> 
  \image latex   valuators.png "Valuators derived from Fl_Valuators" width=10cm
  <P>In the above diagram each box surrounds an actual subclass.  These
  are further differentiated by setting the type() of the widget t
  o the symbolic value labeling the widget.  
  The ones labelled "0" are the default versions with a type(0).  
  For consistency the symbol FL_VERTICAL is defined as zero.
*/
class FL_EXPORT Fl_Valuator : public Fl_Widget {

  double value_;
  double previous_value_;
  double min, max; // truncates to this range *after* rounding
  double A; int B; // rounds to multiples of A/B, or no rounding if A is zero

protected:
  /** Tells if the valuator is an FL_HORIZONTAL one */
  int horizontal() const {return type()& FL_HORIZONTAL;}
  Fl_Valuator(int X, int Y, int W, int H, const char* L);

  /** Gets the previous floating point value before an event changed it */
  double previous_value() const {return previous_value_;}
  /** Stores the current value in the previous value */
  void handle_push() {previous_value_ = value_;}
  double softclamp(double);
  void handle_drag(double newvalue);
  void handle_release(); // use drag() value
  virtual void value_damage(); // cause damage() due to value() changing
  /** Sets the current floating point value. */
  void set_value(double v) {value_ = v;}

public:

    /**    Sets the minimum (a) and maximum (b) values for the valuator widget. */
    void bounds(double a, double b) {min=a; max=b;}
    /**    Gets the minimum value for the valuator.  */
    double minimum() const {return min;}
    /**    Sets the minimum value for the valuator.  */
    void minimum(double a) {min = a;}
    /**    Gets the maximum value for the valuator.  */
    double maximum() const {return max;}
    /**    Sets the maximum value for the valuator.  */
    void maximum(double a) {max = a;}
  /**
    Sets the minimum and maximum values for the valuator. When
    the user manipulates the widget, the value is limited to this
    range. This clamping is done <I>after</I> rounding to the step
    value (this makes a difference if the range is not a multiple of
    the step).
    
    <P>The minimum may be greater than the maximum. This has the
    effect of "reversing" the object so the larger values
    are in the opposite direction. This also switches which end of
    the filled sliders is filled.</P>
    
    <P>Some widgets consider this a "soft" range.  This
    means they will stop at the range, but if the user releases and
    grabs the control again and tries to move it further, it is
    allowed.</P>
    
    <P>The range may affect the display. You must redraw()
    the widget after changing the range.
  */
  void range(double a, double b) {min = a; max = b;}
  /**    See double Fl_Valuator::step() const   */
  void step(int a) {A = a; B = 1;}
  /**    See double Fl_Valuator::step() const   */
  void step(double a, int b) {A = a; B = b;}
  void step(double s);
  /**
    Gets or sets the step value. As the user moves the mouse the
    value is rounded to the nearest multiple of the step value. This
    is done <I>before</I> clamping it to the range. For most widgets
    the default step is zero.
    
    <P>For precision the step is stored as the ratio of two
    integers, A/B. You can set these integers directly. Currently
    setting a floating point value sets the nearest A/1 or 1/B value
    possible.
  */
  double step() const {return A/B;}
  void precision(int);

  /** Gets the floating point(double) value. See int value(double) */
  double value() const {return value_;}
  int value(double);

  virtual int format(char*);
  double round(double); // round to nearest multiple of step
  double clamp(double); // keep in range
  double increment(double, int); // add n*step to value
};

#endif

//
// End of "$Id: Fl_Valuator.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
