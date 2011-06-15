//
// "$Id: Fl_Scroll.H 7981 2010-12-08 23:53:04Z greg.ercolano $"
//
// Scroll header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Scroll widget . */

#ifndef Fl_Scroll_H
#define Fl_Scroll_H

#include "Fl_Group.H"
#include "Fl_Scrollbar.H"

/**
  This container widget lets you maneuver around a set of widgets much
  larger than your window.  If the child widgets are larger than the size
  of this object then scrollbars will appear so that you can scroll over
  to them:
  \image html Fl_Scroll.png
  \image latex  Fl_Scroll.png "Fl_Scroll" width=4cm

  If all of the child widgets are packed together into a solid
  rectangle then you want to set box() to FL_NO_BOX or
  one of the _FRAME types. This will result in the best output.
  However, if the child widgets are a sparse arrangement you must
  set box() to a real _BOX type. This can result in some
  blinking during redrawing, but that can be solved by using a 
  Fl_Double_Window.

  By default you can scroll in both directions, and the scrollbars
  disappear if the data will fit in the area of the scroll. 

  Use Fl_Scroll::type() to change this as follows :
  <UL>
  <LI>0                            - No scrollbars </LI>
  <LI>Fl_Scroll::HORIZONTAL        - Only a horizontal scrollbar. </LI>
  <LI>Fl_Scroll::VERTICAL          - Only a vertical scrollbar. </LI>
  <LI>Fl_Scroll::BOTH              - The default is both scrollbars. </LI>
  <LI>Fl_Scroll::HORIZONTAL_ALWAYS - Horizontal scrollbar always on, vertical always off. </LI>
  <LI>Fl_Scroll::VERTICAL_ALWAYS   - Vertical scrollbar always on, horizontal always off. </LI>
  <LI>Fl_Scroll::BOTH_ALWAYS       - Both always on. </LI>
  </UL>

  Use <B> scrollbar.align(int) ( see void Fl_Widget::align(Fl_Align) ) :</B>
  to change what side the scrollbars are drawn on.

  If the FL_ALIGN_LEFT bit is on, the vertical scrollbar is on the left.
  If the FL_ALIGN_TOP bit is on, the horizontal scrollbar is on
  the top. Note that only the alignment flags in scrollbar are 
  considered. The flags in hscrollbar however are ignored.

  This widget can also be used to pan around a single child widget
  "canvas".  This child widget should be of your own class, with a 
  draw() method that draws the contents.  The scrolling is done by
  changing the x() and y() of the widget, so this child
  must use the x() and y() to position its drawing.
  To speed up drawing it should test fl_push_clip().

  Another very useful child is a single Fl_Pack, which is itself a group
  that packs its children together and changes size to surround them.
  Filling the Fl_Pack with Fl_Tabs groups (and then putting
  normal widgets inside those) gives you a very powerful scrolling list
  of individually-openable panels.

  Fluid lets you create these, but you can only lay out objects that
  fit inside the Fl_Scroll without scrolling.  Be sure to leave
  space for the scrollbars, as Fluid won't show these either.

  <I>You cannot use Fl_Window as a child of this since the
  clipping is not conveyed to it when drawn, and it will draw over the
  scrollbars and neighboring objects.</I>
*/
class FL_EXPORT Fl_Scroll : public Fl_Group {

  int xposition_, yposition_;
  int oldx, oldy;
  int scrollbar_size_;
  static void hscrollbar_cb(Fl_Widget*, void*);
  static void scrollbar_cb(Fl_Widget*, void*);
  void fix_scrollbar_order();
  static void draw_clip(void*,int,int,int,int);

private:

  //
  //  Structure to manage scrollbar and widget interior sizes.
  //
  //  Private for now -- we'd like to expose some of this at 
  //  some point to solve STR#1895.)
  //
  typedef struct {
      int scrollsize;							// the scrollsize (global|local)
      int innerbox_x, innerbox_y, innerbox_w, innerbox_h;		// widget's inner box (excludes scrollbars)
      int innerchild_x, innerchild_y, innerchild_w, innerchild_h;	// widget's inner box including scrollbars
      int child_l, child_r, child_b, child_t;				// child bounding box: left/right/bottom/top
      int hneeded, vneeded;						// hor + ver scrollbar visibility
      int hscroll_x, hscroll_y, hscroll_w, hscroll_h;			// hor scrollbar size/position
      int vscroll_x, vscroll_y, vscroll_w, vscroll_h;			// ver scrollbar size/position
      int hpos, hsize, hfirst, htotal;					// hor scrollbar values (pos/size/first/total)
      int vpos, vsize, vfirst, vtotal;					// ver scrollbar values (pos/size/first/total)
  } ScrollInfo;
  void recalc_scrollbars(ScrollInfo &si);

protected:

  void bbox(int&,int&,int&,int&);
  void draw();

public:

  Fl_Scrollbar scrollbar;
  Fl_Scrollbar hscrollbar;

  void resize(int,int,int,int);
  int handle(int);

  Fl_Scroll(int X,int Y,int W,int H,const char*l=0);

  enum { // values for type()
    HORIZONTAL = 1,
    VERTICAL = 2,
    BOTH = 3,
    ALWAYS_ON = 4,
    HORIZONTAL_ALWAYS = 5,
    VERTICAL_ALWAYS = 6,
    BOTH_ALWAYS = 7
  };

  /**    Gets the current horizontal scrolling position.  */
  int xposition() const {return xposition_;}
  /**    Gets the current vertical scrolling position.  */
  int yposition() const {return yposition_;}
  void scroll_to(int, int);
  void clear();
  /**
    Gets the current size of the scrollbars' troughs, in pixels.

    If this value is zero (default), this widget will use the 
    Fl::scrollbar_size() value as the scrollbar's width.
  
    \returns Scrollbar size in pixels, or 0 if the global Fl::scrollsize() is being used.
    \see Fl::scrollbar_size(int)
  */
  int scrollbar_size() const {
      return(scrollbar_size_);
  }
  /**
    Sets the pixel size of the scrollbars' troughs to the \p size, in pixels.

    Normally you should not need this method, and should use
    Fl::scrollbar_size(int) instead to manage the size of ALL 
    your widgets' scrollbars. This ensures your application 
    has a consistent UI, is the default behavior, and is normally
    what you want.

    Only use THIS method if you really need to override the global
    scrollbar size. The need for this should be rare.
    
    Setting \p size to the special value of 0 causes the widget to
    track the global Fl::scrollbar_size(), which is the default.
    
    \param[in] size Sets the scrollbar size in pixels.\n
                    If 0 (default), scrollbar size tracks the global Fl::scrollbar_size()
    \see Fl::scrollbar_size()
  */
  void scrollbar_size(int size) {
      if ( size != scrollbar_size_ ) redraw();
      scrollbar_size_ = size;
  }   
};

#endif

//
// End of "$Id: Fl_Scroll.H 7981 2010-12-08 23:53:04Z greg.ercolano $".
//
