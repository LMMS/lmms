//
// "$Id: Fl_Tooltip.H 8405 2011-02-08 20:59:46Z manolo $"
//
// Tooltip header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2011 by Bill Spitzak and others.
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
   Fl_Tooltip widget . */

#ifndef Fl_Tooltip_H
#define Fl_Tooltip_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>

/**
  The Fl_Tooltip class provides tooltip support for
  all FLTK widgets. It contains only static methods.
*/
class FL_EXPORT Fl_Tooltip {
public:
  /**    Gets the tooltip delay. The default delay is 1.0 seconds.  */
  static float delay() { return delay_; }
  /**    Sets the tooltip delay. The default delay is 1.0 seconds.  */
  static void delay(float f) { delay_ = f; }
  /**
    Gets the tooltip hover delay, the delay between tooltips.
    The default delay is 0.2 seconds.
  */
  static float hoverdelay() { return hoverdelay_; }
  /**
    Sets the tooltip hover delay, the delay between tooltips.
    The default delay is 0.2 seconds.
  */
  static void hoverdelay(float f) { hoverdelay_ = f; }
  /**    Returns non-zero if tooltips are enabled.  */
  static int enabled() { return Fl::option(Fl::OPTION_SHOW_TOOLTIPS); }
  /**    Enables tooltips on all widgets (or disables if <i>b</i> is false).  */
  static void enable(int b = 1) { Fl::option(Fl::OPTION_SHOW_TOOLTIPS, (b!=0));}
  /**    Same as enable(0), disables tooltips on all widgets.  */
  static void disable() { enable(0); }
  static void (*enter)(Fl_Widget* w);
  static void enter_area(Fl_Widget* w, int X, int Y, int W, int H, const char* tip);
  static void (*exit)(Fl_Widget *w);
  /** Gets the current widget target */
  static Fl_Widget* current() {return widget_;}
  static void current(Fl_Widget*);

  /**    Gets the typeface for the tooltip text.  */
  static Fl_Font font() { return font_; }
  /**    Sets the typeface for the tooltip text.  */
  static void font(Fl_Font i) { font_ = i; }
  /**    Gets the size of the tooltip text.  */
  static Fl_Fontsize size() { return (size_ == -1 ? FL_NORMAL_SIZE : size_); }
  /**    Sets the size of the tooltip text.  */
  static void size(Fl_Fontsize s) { size_ = s; }
  /** Gets the background color for tooltips. The default background color is a pale yellow.  */
  static Fl_Color color() { return color_; }
  /** Sets the background color for tooltips. The default background color is a pale yellow.  */
  static void color(Fl_Color c) { color_ = c; }
  /** Gets the color of the text in the tooltip. The default is  black. */
  static Fl_Color textcolor() { return textcolor_; }
  /** Sets the color of the text in the tooltip. The default is  black. */
  static void textcolor(Fl_Color c) { textcolor_ = c; }
#ifdef __APPLE__
  // the unique tooltip window
  static Fl_Window* current_window(void);
#endif

  // These should not be public, but Fl_Widget::tooltip() needs them...
  // fabien: made it private with only a friend function access
private:
  friend void Fl_Widget::tooltip(const char *);
  friend void Fl_Widget::copy_tooltip(const char *);
  static void enter_(Fl_Widget* w);
  static void exit_(Fl_Widget *w);
  static void set_enter_exit_once_();

private:
  static float delay_; //!< delay before a tooltip is shown
  static float hoverdelay_; //!< delay between tooltips
  static Fl_Color color_;
  static Fl_Color textcolor_;
  static Fl_Font font_;
  static Fl_Fontsize size_;
  static Fl_Widget* widget_; //!< Keeps track of the current target widget
};

#endif

//
// End of "$Id: Fl_Tooltip.H 8405 2011-02-08 20:59:46Z manolo $".
//
