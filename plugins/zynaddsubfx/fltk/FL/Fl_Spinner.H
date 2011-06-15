//
// "$Id: Fl_Spinner.H 8339 2011-01-30 12:50:19Z ianmacarthur $"
//
// Spinner widget for the Fast Light Tool Kit (FLTK).
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
   Fl_Spinner widget . */

#ifndef Fl_Spinner_H
#  define Fl_Spinner_H

//
// Include necessary headers...
//

#  include <FL/Enumerations.H>
#  include <FL/Fl_Group.H>
#  include <FL/Fl_Input.H>
#  include <FL/Fl_Repeat_Button.H>
#  include <stdio.h>
#  include <stdlib.h>


/**
  This widget is a combination of the input
  widget and repeat buttons. The user can either type into the
  input area or use the buttons to change the value.
*/
class FL_EXPORT Fl_Spinner : public Fl_Group {
  
  double	value_;			// Current value
  double	minimum_;		// Minimum value
  double	maximum_;		// Maximum value
  double	step_;			// Amount to add/subtract for up/down
  const char	*format_;		// Format string

  Fl_Input	input_;			// Input field for the value
  Fl_Repeat_Button
		up_button_,		// Up button
		down_button_;		// Down button


  static void	sb_cb(Fl_Widget *w, Fl_Spinner *sb) {
		  double v;		// New value

		  if (w == &(sb->input_)) {
		    // Something changed in the input field...
		    v = atof(sb->input_.value());

		    if (v < sb->minimum_) {
		      sb->value_ = sb->minimum_;
		      sb->update();
		    } else if (v > sb->maximum_) {
		      sb->value_ = sb->maximum_;
		      sb->update();
		    } else sb->value_ = v;
		  } else if (w == &(sb->up_button_)) {
		    // Up button pressed...
		    v = sb->value_ + sb->step_;

		    if (v > sb->maximum_) sb->value_ = sb->minimum_;
		    else sb->value_ = v;

		    sb->update();
		  } else if (w == &(sb->down_button_)) {
		    // Down button pressed...
		    v = sb->value_ - sb->step_;

		    if (v < sb->minimum_) sb->value_ = sb->maximum_;
		    else sb->value_ = v;

		    sb->update();
		  }

		  sb->do_callback();
		}
  void		update() {
		  char s[255];		// Value string

                  if (format_[0]=='%'&&format_[1]=='.'&&format_[2]=='*') {  // precision argument
                    // this code block is a simplified version of
                    // Fl_Valuator::format() and works well (but looks ugly)
                    int c = 0;
                    char temp[64], *sp = temp;
                    sprintf(temp, "%.12f", step_);
                    while (*sp) sp++;
                    sp--;
                    while (sp>temp && *sp=='0') sp--;
                    while (sp>temp && (*sp>='0' && *sp<='9')) { sp--; c++; }
		    sprintf(s, format_, c, value_);
                  } else {
		    sprintf(s, format_, value_);
                  }
		  input_.value(s);
		}

  public:

		/**
		  Creates a new Fl_Spinner widget using the given position, size,
		  and label string.
		  <P>Inherited destructor Destroys the widget and any value associated with it.
		*/
		Fl_Spinner(int X, int Y, int W, int H, const char *L = 0)
		  : Fl_Group(X, Y, W, H, L),
		    input_(X, Y, W - H / 2 - 2, H),
		    up_button_(X + W - H / 2 - 2, Y, H / 2 + 2, H / 2, "@-42<"),
		    down_button_(X + W - H / 2 - 2, Y + H - H / 2,
		                 H / 2 + 2, H / 2, "@-42>") {
		  end();

		  value_   = 1.0;
		  minimum_ = 1.0;
		  maximum_ = 100.0;
		  step_    = 1.0;
		  format_  = "%g";

		  align(FL_ALIGN_LEFT);

		  input_.value("1");
		  input_.type(FL_INT_INPUT);
		  input_.when(FL_WHEN_ENTER_KEY | FL_WHEN_RELEASE);
		  input_.callback((Fl_Callback *)sb_cb, this);

		  up_button_.callback((Fl_Callback *)sb_cb, this);

		  down_button_.callback((Fl_Callback *)sb_cb, this);
		}

  /** Sets or returns the format string for the value. */
  const char	*format() { return (format_); }
  /** Sets or returns the format string for the value. */
  void		format(const char *f) { format_ = f; update(); }

  int		handle(int event) {
		  switch (event) {
		    case FL_KEYDOWN :
		    case FL_SHORTCUT :
		      if (Fl::event_key() == FL_Up) {
			up_button_.do_callback();
			return 1;
		      } else if (Fl::event_key() == FL_Down) {
			down_button_.do_callback();
			return 1;
		      } else return 0;

		    case FL_FOCUS :
		      if (input_.take_focus()) return 1;
		      else return 0;
		  }

		  return Fl_Group::handle(event);
		}

  /** Speling mistakes retained for source compatibility \deprecated */
  double	maxinum() const { return (maximum_); }
  /** Gets the maximum value of the widget. */
  double	maximum() const { return (maximum_); }
  /** Sets the maximum value of the widget. */
  void		maximum(double m) { maximum_ = m; }
  /** Speling mistakes retained for source compatibility \deprecated */
  double	mininum() const { return (minimum_); }
  /** Gets the minimum value of the widget. */
  double	minimum() const { return (minimum_); }
  /** Sets the minimum value of the widget. */
  void		minimum(double m) { minimum_ = m; }
  /** Sets the minimum and maximum values for the widget. */
  void		range(double a, double b) { minimum_ = a; maximum_ = b; }
  void		resize(int X, int Y, int W, int H) {
		  Fl_Group::resize(X,Y,W,H);

		  input_.resize(X, Y, W - H / 2 - 2, H);
		  up_button_.resize(X + W - H / 2 - 2, Y, H / 2 + 2, H / 2);
		  down_button_.resize(X + W - H / 2 - 2, Y + H - H / 2,
		                      H / 2 + 2, H / 2);
		}
  /**
    Sets or returns the amount to change the value when the user clicks a button. 
    Before setting step to a non-integer value, the spinner 
    type() should be changed to floating point. 
  */
  double	step() const { return (step_); }
  /** See double Fl_Spinner::step() const */
  void		step(double s) {
		  step_ = s;
		  if (step_ != (int)step_) input_.type(FL_FLOAT_INPUT);
		  else input_.type(FL_INT_INPUT);
		  update();
		}
  /** Gets the color of the text in the input field. */
  Fl_Color	textcolor() const {
		  return (input_.textcolor());
		}
  /** Sets the color of the text in the input field. */
  void		textcolor(Fl_Color c) {
		  input_.textcolor(c);
		}
  /** Gets the font of the text in the input field. */
  Fl_Font       textfont() const {
		  return (input_.textfont());
		}
  /** Sets the font of the text in the input field. */
  void		textfont(Fl_Font f) {
		  input_.textfont(f);
		}
  /** Gets the size of the text in the input field. */
  Fl_Fontsize  textsize() const {
		  return (input_.textsize());
		}
  /** Sets the size of the text in the input field. */
  void		textsize(Fl_Fontsize s) {
		  input_.textsize(s);
		}
  /** Gets the numeric representation in the input field.
   \see Fl_Spinner::type(uchar) 
  */
  uchar		type() const { return (input_.type()); }
  /** Sets the numeric representation in the input field.
   Valid values are FL_INT_INPUT and FL_FLOAT_INPUT.
   Also changes the format() template.
   Setting a new spinner type via a superclass pointer will not work.
   \note  type is not a virtual function. 
   */
  void		type(uchar v) { 
                  if (v==FL_FLOAT_INPUT) {
                    format("%.*f");
                  } else {
                    format("%.0f");
                  }
                  input_.type(v); 
                }
  /** Gets the current value of the widget. */
  double	value() const { return (value_); }
  /**
    Sets the current value of the widget.
    Before setting value to a non-integer value, the spinner 
    type() should be changed to floating point. 
  */
  void		value(double v) { value_ = v; update(); }
};

#endif // !Fl_Spinner_H

//
// End of "$Id: Fl_Spinner.H 8339 2011-01-30 12:50:19Z ianmacarthur $".
//
