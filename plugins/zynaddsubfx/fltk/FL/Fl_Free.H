//
// "$Id: Fl_Free.H 4288 2005-04-16 00:13:17Z mike $"
//
// Forms free header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Free_H
#define Fl_Free_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

#define FL_NORMAL_FREE		1
#define FL_SLEEPING_FREE	2
#define FL_INPUT_FREE		3
#define FL_CONTINUOUS_FREE	4
#define FL_ALL_FREE		5

typedef int (*FL_HANDLEPTR)(Fl_Widget *, int , float, float, char);

class FL_EXPORT Fl_Free : public Fl_Widget {
    FL_HANDLEPTR hfunc;
    static void step(void *);
protected:
    void draw();
public:
    int handle(int);
    Fl_Free(uchar t,int x,int y,int w,int h,const char *l,FL_HANDLEPTR hdl);
    ~Fl_Free();
};

// old event names for compatability:
#define FL_MOUSE		FL_DRAG
#define FL_DRAW			100		// NOT USED
#define FL_STEP			101
#define FL_FREEMEM		102		// NOT USED
#define FL_FREEZE		103		// NOT USED
#define FL_THAW			104		// NOT USED

#endif

//
// End of "$Id: Fl_Free.H 4288 2005-04-16 00:13:17Z mike $".
//
