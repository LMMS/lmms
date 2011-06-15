//
// "$Id: Fl_File_Input.cxx 8063 2010-12-19 21:20:10Z matt $"
//
// File_Input header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
// Original version Copyright 1998 by Curtis Edwards.
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
#include <FL/Fl_File_Input.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>
#include <FL/filename.H>
#include <stdio.h>
#include "flstring.h"


//
// Height of directory buttons...
//

#define DIR_HEIGHT	10


//
// Redraw bit for directory bar...
//

#define FL_DAMAGE_BAR	0x10


/**
  Creates a new Fl_File_Input widget using the given position,
  size, and label string. The default boxtype is FL_DOWN_BOX.
  \param[in] X, Y, W, H position and size of the widget
  \param[in] L widget label, default is no label
*/
Fl_File_Input::Fl_File_Input(int X, int Y, int W, int H, const char *L)
  : Fl_Input(X, Y, W, H, L) {
  buttons_[0] = 0;
  errorcolor_ = FL_RED;
  ok_entry_   = 1;
  pressed_    = -1;

  down_box(FL_UP_BOX);
}

/**
  Draw directory buttons.
*/
void Fl_File_Input::draw_buttons() {
  int	i,					// Looping var
	X;					// Current X position


  if (damage() & (FL_DAMAGE_BAR | FL_DAMAGE_ALL)) {
    update_buttons();
  }

  for (X = 0, i = 0; buttons_[i]; i ++)
  {
    if ((X + buttons_[i]) > xscroll()) {
      if (X < xscroll()) {
        draw_box(pressed_ == i ? fl_down(down_box()) : down_box(),
                 x(), y(), X + buttons_[i] - xscroll(), DIR_HEIGHT, FL_GRAY);
      } else if ((X + buttons_[i] - xscroll()) > w()) {
	draw_box(pressed_ == i ? fl_down(down_box()) : down_box(),
        	 x() + X - xscroll(), y(), w() - X + xscroll(), DIR_HEIGHT,
		 FL_GRAY);
      } else {
        draw_box(pressed_ == i ? fl_down(down_box()) : down_box(),
	         x() + X - xscroll(), y(), buttons_[i], DIR_HEIGHT, FL_GRAY);
      }
    }

    X += buttons_[i];
  }

  if (X < w()) {
    draw_box(pressed_ == i ? fl_down(down_box()) : down_box(),
             x() + X - xscroll(), y(), w() - X + xscroll(), DIR_HEIGHT, FL_GRAY);
  }
}

/**
  Update the sizes of the directory buttons.
*/
void Fl_File_Input::update_buttons() {
  int		i;				// Looping var
  const char	*start,				// Start of path component
		*end;				// End of path component


//  puts("update_buttons()");

  // Set the current font & size...
  fl_font(textfont(), textsize());

  // Loop through the value string, setting widths...
  for (i = 0, start = value();
       start && i < (int)(sizeof(buttons_) / sizeof(buttons_[0]) - 1);
       start = end, i ++) {
//    printf("    start = \"%s\"\n", start);
    if ((end = strchr(start, '/')) == NULL)
#if defined(WIN32) || defined(__EMX__)
      if ((end = strchr(start, '\\')) == NULL)
#endif // WIN32 || __EMX__
      break;

    end ++;

    buttons_[i] = (short)fl_width(start, end - start);
    if (!i) buttons_[i] += Fl::box_dx(box()) + 6;
  }

//  printf("    found %d components/buttons...\n", i);

  buttons_[i] = 0;
}


/**
  Sets the value of the widget given a new string value and its length.
  Returns non 0 on success.
  \param[in] str new string value
  \param[in] len lengh of value
*/
int						// O - TRUE on success
Fl_File_Input::value(const char *str,		// I - New string value
                     int        len) {		// I - Length of value
  damage(FL_DAMAGE_BAR);
  return Fl_Input::value(str,len);
}


/**
  Sets the value of the widget given a new string value.
  Returns non 0 on success.
  \param[in] str new string value
*/
int						// O - TRUE on success
Fl_File_Input::value(const char *str) {		// I - New string value
  damage(FL_DAMAGE_BAR);
  return Fl_Input::value(str);
}


/**
  Draws the file input widget
*/
void Fl_File_Input::draw() {
  Fl_Boxtype b = box();
  if (damage() & (FL_DAMAGE_BAR | FL_DAMAGE_ALL)) draw_buttons();
  // this flag keeps Fl_Input_::drawtext from drawing a bogus box!
  char must_trick_fl_input_ = 
    Fl::focus()!=this && !size() && !(damage()&FL_DAMAGE_ALL);
  if ((damage() & FL_DAMAGE_ALL) || must_trick_fl_input_) 
    draw_box(b,x(),y()+DIR_HEIGHT,w(),h()-DIR_HEIGHT,color());
  if (!must_trick_fl_input_) 
    Fl_Input_::drawtext(x()+Fl::box_dx(b)+3, y()+Fl::box_dy(b)+DIR_HEIGHT,
		        w()-Fl::box_dw(b)-6, h()-Fl::box_dh(b)-DIR_HEIGHT);
}



/**
  Handle events in the widget.
  Return non zero if event is handled.
  \param[in] event
*/
int						// O - TRUE if we handled event
Fl_File_Input::handle(int event) 		// I - Event
{
//  printf("handle(event = %d)\n", event);
  static char inButtonBar = 0;

  switch (event) {
    case FL_MOVE :
    case FL_ENTER :
      if (active_r()) {
	if (Fl::event_y() < (y() + DIR_HEIGHT)) 
          window()->cursor(FL_CURSOR_DEFAULT);
	else 
          window()->cursor(FL_CURSOR_INSERT);
      }

      return 1;

    case FL_PUSH :
      inButtonBar = (Fl::event_y() < (y() + DIR_HEIGHT));
    case FL_RELEASE :
    case FL_DRAG :
      if (inButtonBar) 
        return handle_button(event);
      else
        return Fl_Input::handle(event);

    default :
      { Fl_Widget_Tracker wp(this);
	if (Fl_Input::handle(event)) {
	  if (wp.exists())
	    damage(FL_DAMAGE_BAR);
	  return 1;
	}
      }
      return 0;
  }
}



/**
  Handles button events in the widget.
  Return non zero if event is handled.
  \param[in] event
*/
int						// O - TRUE if we handled event
Fl_File_Input::handle_button(int event)		// I - Event
{
  int		i,				// Looping var
		X;				// Current X position
  char		*start,				// Start of path component
		*end;				// End of path component
  char		newvalue[FL_PATH_MAX];		// New value


  // Figure out which button is being pressed...
  for (X = 0, i = 0; buttons_[i]; i ++)
  {
    X += buttons_[i];

    if (X > xscroll() && Fl::event_x() < (x() + X - xscroll())) break;
  }

//  printf("handle_button(event = %d), button = %d\n", event, i);

  // Redraw the directory bar...
  if (event == FL_RELEASE) pressed_ = -1;
  else pressed_ = (short)i;

  window()->make_current();
  draw_buttons();

  // Return immediately if the user is clicking on the last button or
  // has not released the mouse button...
  if (!buttons_[i] || event != FL_RELEASE) return 1;

  // Figure out where to truncate the path...
  strlcpy(newvalue, value(), sizeof(newvalue));

  for (start = newvalue, end = start; start && i >= 0; start = end, i --) {
//    printf("    start = \"%s\"\n", start);
    if ((end = strchr(start, '/')) == NULL)
#if defined(WIN32) || defined(__EMX__)
      if ((end = strchr(start, '\\')) == NULL)
#endif // WIN32 || __EMX__
      break;

    end ++;
  }

  if (i < 0) {
    // Found the end; truncate the value and update the buttons...
    *start = '\0';
    value(newvalue, start - newvalue);

    // Then do the callbacks, if necessary...
    set_changed();
    if (when() & (FL_WHEN_CHANGED|FL_WHEN_RELEASE) ) do_callback();
  }

  return 1;
}


//
// End of "$Id: Fl_File_Input.cxx 8063 2010-12-19 21:20:10Z matt $".
//
