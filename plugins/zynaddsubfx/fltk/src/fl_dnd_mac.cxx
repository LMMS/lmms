//
// "$Id: fl_dnd_mac.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Drag & Drop code for the Fast Light Tool Kit (FLTK).
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
// Please report all bugs and problems to "fltk-bugs@fltk.org

// This file contains win32-specific code for fltk which is always linked
// in.  Search other files for "WIN32" or filenames ending in _win32.cxx
// for other system-specific code.

#include <config.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>

// warning: this function is only implemented in Quickdraw. The function
//          below may not work If FLTK is compiled with Quartz enabled

extern EventRef fl_os_event;
extern char *fl_selection_buffer;
extern int fl_selection_length;


/**
 * drag and drop whatever is in the cut-copy-paste buffer
 * - create a selection first using: 
 *     Fl::copy(const char *stuff, int len, 0)
 */
int Fl::dnd()
{
  OSErr result;
  DragReference dragRef;
  result = NewDrag( &dragRef );
  if ( result != noErr ) return false;
  
  result = AddDragItemFlavor( dragRef, 1, 'TEXT', fl_selection_buffer, fl_selection_length, 0 );
  if ( result != noErr ) { DisposeDrag( dragRef ); return false; }
  
  Point mp;
  GetMouse(&mp);
  LocalToGlobal( &mp );
  RgnHandle region = NewRgn();
  SetRectRgn( region, mp.h-10, mp.v-10, mp.h+10, mp.v+10 );
  RgnHandle r2 = NewRgn();
  SetRectRgn( r2, mp.h-8, mp.v-8, mp.h+8, mp.v+8 );
  DiffRgn( region, r2, region );
  DisposeRgn( r2 );

  EventRecord event;
  ConvertEventRefToEventRecord( fl_os_event, &event );
  result = TrackDrag( dragRef, &event, region );

  Fl_Widget *w = Fl::pushed();
  if ( w )
  {
    int old_event = Fl::e_number;
    w->handle(Fl::e_number = FL_RELEASE);
    Fl::e_number = old_event;
    Fl::pushed( 0 );
  }

  if ( result != noErr ) { DisposeRgn( region ); DisposeDrag( dragRef ); return false; }
  
  DisposeRgn( region );
  DisposeDrag( dragRef );
  return true;
}
  

//
// End of "$Id: fl_dnd_mac.cxx 5190 2006-06-09 16:16:34Z mike $".
//
