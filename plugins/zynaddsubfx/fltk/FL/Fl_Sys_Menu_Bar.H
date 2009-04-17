//
// "$Id: Fl_Sys_Menu_Bar.H 4546 2005-08-29 20:05:38Z matt $"
//
// MacOS system menu bar header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Sys_Menu_Bar_H
#define Fl_Sys_Menu_Bar_H

#include "Fl_Menu_Bar.H"

#ifdef __APPLE__

class FL_EXPORT Fl_Sys_Menu_Bar : public Fl_Menu_Bar {
protected:
  void draw();
public:
  Fl_Sys_Menu_Bar(int x,int y,int w,int h,const char *l=0)
      : Fl_Menu_Bar(x,y,w,h,l) {
	   deactivate();			// don't let the old area take events
	  }
  void menu(const Fl_Menu_Item *m);
};

#else

typedef Fl_Menu_Bar Fl_Sys_Menu_Bar;

#endif

#endif

//
// End of "$Id: Fl_Sys_Menu_Bar.H 4546 2005-08-29 20:05:38Z matt $".
//
