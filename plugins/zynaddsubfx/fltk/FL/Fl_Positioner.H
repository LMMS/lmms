//
// "$Id: Fl_Positioner.H 4288 2005-04-16 00:13:17Z mike $"
//
// Positioner header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Positioner_H
#define Fl_Positioner_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

class FL_EXPORT Fl_Positioner : public Fl_Widget {

  double xmin, ymin;
  double xmax, ymax;
  double xvalue_, yvalue_;
  double xstep_, ystep_;

protected:

  // these allow subclasses to put the dial in a smaller area:
  void draw(int, int, int, int);
  int handle(int, int, int, int, int);
  void draw();

public:

  int handle(int);
  Fl_Positioner(int x,int y,int w,int h, const char *l=0);
  double xvalue() const {return xvalue_;}
  double yvalue() const {return yvalue_;}
  int xvalue(double);
  int yvalue(double);
  int value(double,double);
  void xbounds(double, double);
  double xminimum() const {return xmin;}
  void xminimum(double a) {xbounds(a,xmax);}
  double xmaximum() const {return xmax;}
  void xmaximum(double a) {xbounds(xmin,a);}
  void ybounds(double, double);
  double yminimum() const {return ymin;}
  void yminimum(double a) {ybounds(a,ymax);}
  double ymaximum() const {return ymax;}
  void ymaximum(double a) {ybounds(ymin,a);}
  void xstep(double a) {xstep_ = a;}
  void ystep(double a) {ystep_ = a;}

};

#endif

//
// End of "$Id: Fl_Positioner.H 4288 2005-04-16 00:13:17Z mike $".
//
