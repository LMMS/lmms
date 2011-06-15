//
// "$Id: Fl_Input_Choice.H 8022 2010-12-12 23:21:03Z AlbrechtS $"
//
// An input/chooser widget.
//            ______________  ____
//           |              || __ |
//           | input area   || \/ |
//           |______________||____|
//
// Copyright 1998-2010 by Bill Spitzak and others.
// Copyright 2004 by Greg Ercolano.
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
   Fl_Input_Choice widget . */



#ifndef Fl_Input_Choice_H
#define Fl_Input_Choice_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_draw.H>
#include <string.h>

/**
  A combination of the input widget and a menu button.
  The user can either type into the input area, or use the
  menu button chooser on the right, which loads the input area
  with predefined text.  Normally it is drawn with an inset box 
  and a white background. 
  <P>
  The application can directly access both the input and menu
  widgets directly, using the menubutton()
  and input() accessor methods.
*/
class FL_EXPORT Fl_Input_Choice : public Fl_Group {
  // Private class to handle slightly 'special' behavior of menu button
  class InputMenuButton : public Fl_Menu_Button {
    void draw() {
      draw_box(FL_UP_BOX, color());
      fl_color(active_r() ? labelcolor() : fl_inactive(labelcolor()));
      int xc = x()+w()/2, yc=y()+h()/2;
      fl_polygon(xc-5,yc-3,xc+5,yc-3,xc,yc+3);
      if (Fl::focus() == this) draw_focus();
    }
  public:
    InputMenuButton(int x,int y,int w,int h,const char*l=0) : 
	Fl_Menu_Button(x,y,w,h,l) { box(FL_UP_BOX); }
  };

  Fl_Input *inp_;
  InputMenuButton *menu_;

  static void menu_cb(Fl_Widget*, void *data) { 
    Fl_Input_Choice *o=(Fl_Input_Choice *)data;
    Fl_Widget_Tracker wp(o);
    const Fl_Menu_Item *item = o->menubutton()->mvalue();
    if (item && item->flags & (FL_SUBMENU|FL_SUBMENU_POINTER)) return;	// ignore submenus
    if (!strcmp(o->inp_->value(), o->menu_->text()))
    {
      o->Fl_Widget::clear_changed();
      if (o->when() & FL_WHEN_NOT_CHANGED)
	o->do_callback();
    }
    else
    {
      o->inp_->value(o->menu_->text());
      o->inp_->set_changed();
      o->Fl_Widget::set_changed();
      if (o->when() & (FL_WHEN_CHANGED|FL_WHEN_RELEASE))
	o->do_callback();
    }
    
    if (wp.deleted()) return;

    if (o->callback() != default_callback)
    {
      o->Fl_Widget::clear_changed();
      o->inp_->clear_changed();
    }
  }

  static void inp_cb(Fl_Widget*, void *data) { 
    Fl_Input_Choice *o=(Fl_Input_Choice *)data;
    Fl_Widget_Tracker wp(o);
    if (o->inp_->changed()) {
      o->Fl_Widget::set_changed();
      if (o->when() & (FL_WHEN_CHANGED|FL_WHEN_RELEASE))
	o->do_callback();
    } else {
      o->Fl_Widget::clear_changed();
      if (o->when() & FL_WHEN_NOT_CHANGED)
	o->do_callback();
    }
    
    if (wp.deleted()) return;

    if (o->callback() != default_callback)
      o->Fl_Widget::clear_changed();
  }

  // Custom resize behavior -- input stretches, menu button doesn't
  inline int inp_x() { return(x() + Fl::box_dx(box())); }
  inline int inp_y() { return(y() + Fl::box_dy(box())); }
  inline int inp_w() { return(w() - Fl::box_dw(box()) - 20); }
  inline int inp_h() { return(h() - Fl::box_dh(box())); }

  inline int menu_x() { return(x() + w() - 20 - Fl::box_dx(box())); }
  inline int menu_y() { return(y() + Fl::box_dy(box())); }
  inline int menu_w() { return(20); }
  inline int menu_h() { return(h() - Fl::box_dh(box())); }

public:
  /**
    Creates a new Fl_Input_Choice widget using the given position, size,
    and label string.
  <P> Inherited destructor Destroys the widget and any value associated with it.
  */
  Fl_Input_Choice (int x,int y,int w,int h,const char*l=0) : Fl_Group(x,y,w,h,l) {
    Fl_Group::box(FL_DOWN_BOX);
    align(FL_ALIGN_LEFT);				// default like Fl_Input
    inp_ = new Fl_Input(inp_x(), inp_y(),
			inp_w(), inp_h());
    inp_->callback(inp_cb, (void*)this);
    inp_->box(FL_FLAT_BOX);		// cosmetic
    inp_->when(FL_WHEN_CHANGED|FL_WHEN_NOT_CHANGED);
    menu_ = new InputMenuButton(menu_x(), menu_y(),
				menu_w(), menu_h());
    menu_->callback(menu_cb, (void*)this);
    menu_->box(FL_FLAT_BOX);				// cosmetic
    end();
  }
  
  /** Adds an item to the menu.*/
  void add(const char *s) { menu_->add(s);  }
  int changed() const { return inp_->changed() | Fl_Widget::changed();}
  void clear_changed() { 
    inp_->clear_changed();
    Fl_Widget::clear_changed();
  }
  void set_changed() { 
    inp_->set_changed();
    // no need to call Fl_Widget::set_changed()
  }
  /** Removes all items from the menu. */
  void clear() { menu_->clear(); }
  /** Gets the box type of the menu button */
  Fl_Boxtype down_box() const { return (menu_->down_box()); }
  /** Sets the box type of the menu button */
  void down_box(Fl_Boxtype b) { menu_->down_box(b); }
  /** Gets the Fl_Menu_Item array used for the menu. */
  const Fl_Menu_Item *menu() { return (menu_->menu()); }
  /** Sets the Fl_Menu_Item array used for the menu. */
  void menu(const Fl_Menu_Item *m) { menu_->menu(m); }
  void resize(int X, int Y, int W, int H) {
    Fl_Group::resize(X,Y,W,H);
    inp_->resize(inp_x(), inp_y(), inp_w(), inp_h());
    menu_->resize(menu_x(), menu_y(), menu_w(), menu_h());
  }
  /** Gets the encapsulated input text color attributes */
  Fl_Color textcolor() const { return (inp_->textcolor());}
  /** Sets the encapsulated input text color attributes */
  void textcolor(Fl_Color c) { inp_->textcolor(c);}
  /** Gets the encapsulated input text font  attributes */
  Fl_Font textfont() const { return (inp_->textfont());}
  /** Sets the encapsulated input text font attributes */
  void textfont(Fl_Font f) { inp_->textfont(f);}
  /** Gets the encapsulated input size attributes */
  Fl_Fontsize textsize() const { return (inp_->textsize()); }
  /** Sets the encapsulated input size attributes */
  void textsize(Fl_Fontsize s) { inp_->textsize(s); }
  /** See void Fl_Input_Choice::value(const char *s)  */
  const char* value() const { return (inp_->value()); }
  /**
    Sets or returns the input widget's current contents. The
    second form sets the contents using the index into the menu
    which you can set as an integer.  Setting the value effectively
    'chooses' this menu item, and sets it as the new input text,
    deleting the previous text.
  */
  void value(const char *val) { inp_->value(val); }
  /**    See void Fl_Input_Choice::value(const char *s)  */
  void value(int val) {
    menu_->value(val);
    inp_->value(menu_->text(val));
  }
  /**    Returns a reference to the internal Fl_Menu_Button widget.  */
  Fl_Menu_Button *menubutton() { return menu_; }
  /**
    Returns a reference to the internal Fl_Input widget.</p>
  */
  Fl_Input *input() { return inp_; }
};

#endif // !Fl_Input_Choice_H

//
// End of "$Id: Fl_Input_Choice.H 8022 2010-12-12 23:21:03Z AlbrechtS $".
//
