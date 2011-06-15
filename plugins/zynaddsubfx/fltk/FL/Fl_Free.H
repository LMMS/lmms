//
// "$Id: Fl_Free.H 7903 2010-11-28 21:06:39Z matt $"
//
// Forms free header file for the Fast Light Tool Kit (FLTK).
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
   Fl_Free widget . */

#ifndef Fl_Free_H
#define Fl_Free_H

#ifndef Fl_Widget_H
#include "Fl_Widget.H"
#endif

#define FL_NORMAL_FREE		1 /**< normal event handling */
#define FL_SLEEPING_FREE	2 /**< deactivate event handling */
#define FL_INPUT_FREE		3 /**< accepts FL_FOCUS events */
#define FL_CONTINUOUS_FREE	4 /**< repeated timeout handling */
#define FL_ALL_FREE		5 /**< FL_INPUT_FREE and FL_CONTINOUS_FREE */

/** appropriate signature for handle function */
typedef int (*FL_HANDLEPTR)(Fl_Widget *, int , float, float, char);

/**
  Emulation of the Forms "free" widget.
  
  This emulation allows the free demo to run, and appears to be useful for
  porting programs written in Forms which use the free widget or make
  subclasses of the Forms widgets.

  There are five types of free, which determine when the handle function
  is called:
  
  \li \c FL_NORMAL_FREE      normal event handling.
  \li \c FL_SLEEPING_FREE    deactivates event handling (widget is inactive).
  \li \c FL_INPUT_FREE       accepts FL_FOCUS events.
  \li \c FL_CONTINUOUS_FREE  sets a timeout callback 100 times a second and
                             provides an FL_STEP event. This has obvious
			     detrimental effects on machine performance.
  \li \c FL_ALL_FREE         same as FL_INPUT_FREE and FL_CONTINUOUS_FREE.
 
*/
class FL_EXPORT Fl_Free : public Fl_Widget {
    FL_HANDLEPTR hfunc;
    static void step(void *);
protected:
    void draw();
public:
    int handle(int e);
  Fl_Free(uchar t,int X,int Y,int W,int H,const char *L,FL_HANDLEPTR hdl);
  ~Fl_Free();
};

// old event names for compatibility:
#define FL_MOUSE	FL_DRAG /**< for backward compatibility */
#define FL_DRAW		100     /**< for backward compatibility [UNUSED]*/
#define FL_STEP		101     /**< for backward compatibility */
#define FL_FREEMEM	102     /**< for backward compatibility [UNUSED]*/
#define FL_FREEZE	103     /**< for backward compatibility [UNUSED]*/
#define FL_THAW		104     /**< for backward compatibility [UNUSED]*/

#endif

//
// End of "$Id: Fl_Free.H 7903 2010-11-28 21:06:39Z matt $".
//
