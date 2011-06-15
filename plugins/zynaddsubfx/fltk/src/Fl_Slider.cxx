//
// "$Id: Fl_Slider.cxx 8726 2011-05-23 18:32:47Z AlbrechtS $"
//
// Slider widget for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>
#include <math.h>
#include "flstring.h"

#if defined(FL_DLL)	// really needed for c'tors for MS VC++ only
#include <FL/Fl_Hor_Slider.H>
#endif

void Fl_Slider::_Fl_Slider() {
  slider_size_ = 0;
  slider_ = 0; // FL_UP_BOX;
}

/**
  Creates a new Fl_Slider widget using the given position,
  size, and label string. The default boxtype is FL_DOWN_BOX.
*/
Fl_Slider::Fl_Slider(int X, int Y, int W, int H, const char* L)
: Fl_Valuator(X, Y, W, H, L) {
  box(FL_DOWN_BOX);
  _Fl_Slider();
}

/**
  Creates a new Fl_Slider widget using the given box type, position,
  size, and label string.
*/
Fl_Slider::Fl_Slider(uchar t, int X, int Y, int W, int H, const char* L)
  : Fl_Valuator(X, Y, W, H, L) {
  type(t);
  box(t==FL_HOR_NICE_SLIDER || t==FL_VERT_NICE_SLIDER ?
      FL_FLAT_BOX : FL_DOWN_BOX);
  _Fl_Slider();
}

void Fl_Slider::slider_size(double v) {
  if (v <  0) v = 0;
  if (v > 1) v = 1;
  if (slider_size_ != float(v)) {
    slider_size_ = float(v); 
    damage(FL_DAMAGE_EXPOSE);
  }
}

/** 
  Sets the minimum (a) and maximum (b) values for the valuator widget. 
  if at least one of the values is changed, a partial redraw is asked.
*/
void Fl_Slider::bounds(double a, double b) {
  if (minimum() != a || maximum() != b) {
    Fl_Valuator::bounds(a, b); 
    damage(FL_DAMAGE_EXPOSE);
  }
}

/**
  Sets the size and position of the sliding knob in the box.
  \param[in] pos position of first line displayed
  \param[in] size size of window in lines
  \param[in] first number of first line
  \param[in] total total number of lines
  Returns Fl_Valuator::value(p)
 */
int Fl_Slider::scrollvalue(int pos, int size, int first, int total) {
  step(1, 1);
  if (pos+size > first+total) total = pos+size-first;
  slider_size(size >= total ? 1.0 : double(size)/double(total));
  bounds(first, total-size+first);
  return value(pos);
}

// All slider interaction is done as though the slider ranges from
// zero to one, and the left (bottom) edge of the slider is at the
// given position.  Since when the slider is all the way to the
// right (top) the left (bottom) edge is not all the way over, a
// position on the widget itself covers a wider range than 0-1,
// actually it ranges from 0 to 1/(1-size).

void Fl_Slider::draw_bg(int X, int Y, int W, int H) {
  fl_push_clip(X, Y, W, H);
  draw_box();
  fl_pop_clip();

  Fl_Color black = active_r() ? FL_FOREGROUND_COLOR : FL_INACTIVE_COLOR;
  if (type() == FL_VERT_NICE_SLIDER) {
    draw_box(FL_THIN_DOWN_BOX, X+W/2-2, Y, 4, H, black);
  } else if (type() == FL_HOR_NICE_SLIDER) {
    draw_box(FL_THIN_DOWN_BOX, X, Y+H/2-2, W, 4, black);
  }
}

void Fl_Slider::draw(int X, int Y, int W, int H) {

  double val;
  if (minimum() == maximum())
    val = 0.5;
  else {
    val = (value()-minimum())/(maximum()-minimum());
    if (val > 1.0) val = 1.0;
    else if (val < 0.0) val = 0.0;
  }

  int ww = (horizontal() ? W : H);
  int xx, S;
  if (type()==FL_HOR_FILL_SLIDER || type() == FL_VERT_FILL_SLIDER) {
    S = int(val*ww+.5);
    if (minimum()>maximum()) {S = ww-S; xx = ww-S;}
    else xx = 0;
  } else {
    S = int(slider_size_*ww+.5);
    int T = (horizontal() ? H : W)/2+1;
    if (type()==FL_VERT_NICE_SLIDER || type()==FL_HOR_NICE_SLIDER) T += 4;
    if (S < T) S = T;
    xx = int(val*(ww-S)+.5);
  }
  int xsl, ysl, wsl, hsl;
  if (horizontal()) {
    xsl = X+xx;
    wsl = S;
    ysl = Y;
    hsl = H;
  } else {
    ysl = Y+xx;
    hsl = S;
    xsl = X;
    wsl = W;
  }

  draw_bg(X, Y, W, H);

  Fl_Boxtype box1 = slider();
  if (!box1) {box1 = (Fl_Boxtype)(box()&-2); if (!box1) box1 = FL_UP_BOX;}
  if (type() == FL_VERT_NICE_SLIDER) {
    draw_box(box1, xsl, ysl, wsl, hsl, FL_GRAY);
    int d = (hsl-4)/2;
    draw_box(FL_THIN_DOWN_BOX, xsl+2, ysl+d, wsl-4, hsl-2*d,selection_color());
  } else if (type() == FL_HOR_NICE_SLIDER) {
    draw_box(box1, xsl, ysl, wsl, hsl, FL_GRAY);
    int d = (wsl-4)/2;
    draw_box(FL_THIN_DOWN_BOX, xsl+d, ysl+2, wsl-2*d, hsl-4,selection_color());
  } else {
    if (wsl>0 && hsl>0) draw_box(box1, xsl, ysl, wsl, hsl, selection_color());

    if (type()!=FL_HOR_FILL_SLIDER && type() != FL_VERT_FILL_SLIDER &&
        Fl::scheme_ && !strcmp(Fl::scheme_, "gtk+")) {
      if (W>H && wsl>(hsl+8)) {
        // Draw horizontal grippers
	int yy, hh;
	hh = hsl-8;
	xx = xsl+(wsl-hsl-4)/2;
	yy = ysl+3;

	fl_color(fl_darker(selection_color()));
	fl_line(xx, yy+hh, xx+hh, yy);
	fl_line(xx+6, yy+hh, xx+hh+6, yy);
	fl_line(xx+12, yy+hh, xx+hh+12, yy);

        xx++;
	fl_color(fl_lighter(selection_color()));
	fl_line(xx, yy+hh, xx+hh, yy);
	fl_line(xx+6, yy+hh, xx+hh+6, yy);
	fl_line(xx+12, yy+hh, xx+hh+12, yy);
      } else if (H>W && hsl>(wsl+8)) {
        // Draw vertical grippers
	int yy;
	xx = xsl+4;
	ww = wsl-8;
	yy = ysl+(hsl-wsl-4)/2;

	fl_color(fl_darker(selection_color()));
	fl_line(xx, yy+ww, xx+ww, yy);
	fl_line(xx, yy+ww+6, xx+ww, yy+6);
	fl_line(xx, yy+ww+12, xx+ww, yy+12);

        yy++;
	fl_color(fl_lighter(selection_color()));
	fl_line(xx, yy+ww, xx+ww, yy);
	fl_line(xx, yy+ww+6, xx+ww, yy+6);
	fl_line(xx, yy+ww+12, xx+ww, yy+12);
      }
    }
  }

  draw_label(xsl, ysl, wsl, hsl);
  if (Fl::focus() == this) {
    if (type() == FL_HOR_FILL_SLIDER || type() == FL_VERT_FILL_SLIDER) draw_focus();
    else draw_focus(box1, xsl, ysl, wsl, hsl);
  }
}

void Fl_Slider::draw() {
  if (damage()&FL_DAMAGE_ALL) draw_box();
  draw(x()+Fl::box_dx(box()),
       y()+Fl::box_dy(box()),
       w()-Fl::box_dw(box()),
       h()-Fl::box_dh(box()));
}

int Fl_Slider::handle(int event, int X, int Y, int W, int H) {
  // Fl_Widget_Tracker wp(this);
  switch (event) {
  case FL_PUSH: {
    Fl_Widget_Tracker wp(this);
    if (!Fl::event_inside(X, Y, W, H)) return 0;
    handle_push();
    if (wp.deleted()) return 1; }
    // fall through ...
  case FL_DRAG: {

    double val;
    if (minimum() == maximum())
      val = 0.5;
    else {
      val = (value()-minimum())/(maximum()-minimum());
      if (val > 1.0) val = 1.0;
      else if (val < 0.0) val = 0.0;
    }

    int ww = (horizontal() ? W : H);
    int mx = (horizontal() ? Fl::event_x()-X : Fl::event_y()-Y);
    int S;
    static int offcenter;

    if (type() == FL_HOR_FILL_SLIDER || type() == FL_VERT_FILL_SLIDER) {

      S = 0;
      if (event == FL_PUSH) {
	int xx = int(val*ww+.5);
	offcenter = mx-xx;
	if (offcenter < -10 || offcenter > 10) offcenter = 0;
	else return 1;
      }

    } else {

      S = int(slider_size_*ww+.5); if (S >= ww) return 0;
      int T = (horizontal() ? H : W)/2+1;
      if (type()==FL_VERT_NICE_SLIDER || type()==FL_HOR_NICE_SLIDER) T += 4;
      if (S < T) S = T;
      if (event == FL_PUSH) {
	int xx = int(val*(ww-S)+.5);
	offcenter = mx-xx;
	if (offcenter < 0) offcenter = 0;
	else if (offcenter > S) offcenter = S;
	else return 1;
      }
    }

    int xx = mx-offcenter;
    double v;
    char tryAgain = 1;
    while (tryAgain)
    {
      tryAgain = 0;
      if (xx < 0) {
        xx = 0;
        offcenter = mx; if (offcenter < 0) offcenter = 0;
      } else if (xx > (ww-S)) {
        xx = ww-S;
        offcenter = mx-xx; if (offcenter > S) offcenter = S;
      }
      v = round(xx*(maximum()-minimum())/(ww-S) + minimum());
      // make sure a click outside the sliderbar moves it:
      if (event == FL_PUSH && v == value()) {
        offcenter = S/2;
        event = FL_DRAG;
        tryAgain = 1;
      }
    }
    handle_drag(clamp(v));
    } return 1;
  case FL_RELEASE:
    handle_release();
    return 1;
  case FL_KEYBOARD:
    { Fl_Widget_Tracker wp(this);
      switch (Fl::event_key()) {
	case FL_Up:
	  if (horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),-1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Down:
	  if (horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Left:
	  if (!horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),-1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Right:
	  if (!horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	default:
	  return 0;
      }
    }
    // break not required because of switch...
  case FL_FOCUS :
  case FL_UNFOCUS :
    if (Fl::visible_focus()) {
      redraw();
      return 1;
    } else return 0;
  case FL_ENTER :
  case FL_LEAVE :
    return 1;
  default:
    return 0;
  }
}

int Fl_Slider::handle(int event) {
  if (event == FL_PUSH && Fl::visible_focus()) {
    Fl::focus(this);
    redraw();
  }

  return handle(event,
		x()+Fl::box_dx(box()),
		y()+Fl::box_dy(box()),
		w()-Fl::box_dw(box()),
		h()-Fl::box_dh(box()));
}

/*
  The following constructor must not be in the header file if we
  build a shared object (DLL). Instead it is defined here to force
  the constructor (and default destructor as well) to be defined
  in the DLL and exported (STR #2632).
  
  Note: if you the ctor here, do the same changes in the specific
  header file as well.  This redundant definition was chosen to enable
  inline constructors in the header files (for static linking) as well
  as the one here for dynamic linking (Windows DLL).
*/

#if defined(FL_DLL)

Fl_Hor_Slider::Fl_Hor_Slider(int X,int Y,int W,int H,const char *l)
	: Fl_Slider(X,Y,W,H,l) {type(FL_HOR_SLIDER);}

#endif // FL_DLL

//
// End of "$Id: Fl_Slider.cxx 8726 2011-05-23 18:32:47Z AlbrechtS $".
//
