//
// "$Id: Fl_compose.cxx 8626 2011-04-27 11:21:57Z manolo $"
//
// Character compose processing for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/x.H>

#ifndef FL_DOXYGEN
int Fl::compose_state = 0;
#endif

#if !defined(WIN32) && !defined(__APPLE__)
extern XIC fl_xim_ic;
#endif

/** Any text editing widget should call this for each FL_KEYBOARD event.
 Use of this function is very simple.
 
 <p>If <i>true</i> is returned, then it has modified the
 Fl::event_text() and Fl::event_length() to a set of <i>bytes</i> to
 insert (it may be of zero length!).  In will also set the "del"
 parameter to the number of <i>bytes</i> to the left of the cursor to
 delete, this is used to delete the results of the previous call to
 Fl::compose().
 
 <p>If <i>false</i> is returned, the keys should be treated as function
 keys, and del is set to zero. You could insert the text anyways, if
 you don't know what else to do.
 
 <p>Though the current implementation returns immediately, future
 versions may take quite awhile, as they may pop up a window or do
 other user-interface things to allow characters to be selected.
 */
int Fl::compose(int& del) {
  // character composition is now handled by the OS
  del = 0;
#if defined(__APPLE__)
  // this stuff is to be treated as a function key
  if(Fl::e_length == 0 || Fl::e_keysym == FL_Enter || Fl::e_keysym == FL_KP_Enter || 
     Fl::e_keysym == FL_Tab || Fl::e_keysym == FL_Escape || Fl::e_state&(FL_META | FL_CTRL) ) {
    return 0;
  }
#elif defined(WIN32)
  unsigned char ascii = (unsigned)e_text[0];
  if ((e_state & (FL_ALT | FL_META)) && !(ascii & 128)) return 0;
#else
  unsigned char ascii = (unsigned)e_text[0];
  if ((e_state & (FL_ALT | FL_META | FL_CTRL)) && !(ascii & 128)) return 0;
#endif
  if(Fl::compose_state) {
    del = Fl::compose_state;
    Fl::compose_state = 0;
#ifndef __APPLE__
  } else {
    // Only insert non-control characters:
    if (! (ascii & ~31 && ascii!=127)) { return 0; }
#endif
  }
  return 1;
}

/**
 If the user moves the cursor, be sure to call Fl::compose_reset().
 The next call to Fl::compose() will start out in an initial state. In
 particular it will not set "del" to non-zero. This call is very fast
 so it is ok to call it many times and in many places.
 */
void Fl::compose_reset()
{
  Fl::compose_state = 0;
#if !defined(WIN32) && !defined(__APPLE__)
  if (fl_xim_ic) XmbResetIC(fl_xim_ic);
#endif
}

//
// End of "$Id: Fl_compose.cxx 8626 2011-04-27 11:21:57Z manolo $"
//

