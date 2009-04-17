//
// "$Id: fl_dnd_x.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Drag & Drop code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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
#include <FL/Fl_Window.H>
#include <FL/x.H>
#include "flstring.h"


extern Atom fl_XdndAware;
extern Atom fl_XdndSelection;
extern Atom fl_XdndEnter;
extern Atom fl_XdndTypeList;
extern Atom fl_XdndPosition;
extern Atom fl_XdndLeave;
extern Atom fl_XdndDrop;
extern Atom fl_XdndStatus;
extern Atom fl_XdndActionCopy;
extern Atom fl_XdndFinished;
//extern Atom fl_XdndProxy;
extern Atom fl_XdndURIList;

extern char fl_i_own_selection[2];
extern char *fl_selection_buffer[2];

extern void fl_sendClientMessage(Window window, Atom message,
                                 unsigned long d0,
                                 unsigned long d1=0,
                                 unsigned long d2=0,
                                 unsigned long d3=0,
                                 unsigned long d4=0);

// return version # of Xdnd this window supports.  Also change the
// window to the proxy if it uses a proxy:
static int dnd_aware(Window& window) {
  Atom actual; int format; unsigned long count, remaining;
  unsigned char *data = 0;
  XGetWindowProperty(fl_display, window, fl_XdndAware,
		     0, 4, False, XA_ATOM,
		     &actual, &format,
		     &count, &remaining, &data);
  if (actual == XA_ATOM && format==32 && count && data)
    return int(*(Atom*)data);
  return 0;
}

static int grabfunc(int event) {
  if (event == FL_RELEASE) Fl::pushed(0);
  return 0;
}

extern int (*fl_local_grab)(int); // in Fl.cxx

// send an event to an fltk window belonging to this program:
static int local_handle(int event, Fl_Window* window) {
  fl_local_grab = 0;
  Fl::e_x = Fl::e_x_root-window->x();
  Fl::e_y = Fl::e_y_root-window->y();
  int ret = Fl::handle(event,window);
  fl_local_grab = grabfunc;
  return ret;
}

int Fl::dnd() {
  Fl_Window *source_fl_win = Fl::first_window();
  Fl::first_window()->cursor((Fl_Cursor)21);
  Window source_window = fl_xid(Fl::first_window());
  fl_local_grab = grabfunc;
  Window target_window = 0;
  Fl_Window* local_window = 0;
  int dndversion = 4; int dest_x, dest_y;
  XSetSelectionOwner(fl_display, fl_XdndSelection, fl_message_window, fl_event_time);

  while (Fl::pushed()) {

    // figure out what window we are pointing at:
    Window new_window = 0; int new_version = 0;
    Fl_Window* new_local_window = 0;
    for (Window child = RootWindow(fl_display, fl_screen);;) {
      Window root; unsigned int junk3;
      XQueryPointer(fl_display, child, &root, &child,
		    &e_x_root, &e_y_root, &dest_x, &dest_y, &junk3);
      if (!child) {
	if (!new_window && (new_version = dnd_aware(root))) new_window = root;
	break;
      }
      new_window = child;
      if ((new_local_window = fl_find(child))) break;
      if ((new_version = dnd_aware(new_window))) break;
    }

    if (new_window != target_window) {
      if (local_window) {
	local_handle(FL_DND_LEAVE, local_window);
      } else if (dndversion) {
	fl_sendClientMessage(target_window, fl_XdndLeave, source_window);
      }
      dndversion = new_version;
      target_window = new_window;
      local_window = new_local_window;
      if (local_window) {
	local_handle(FL_DND_ENTER, local_window);
      } else if (dndversion) {
        // Send an X-DND message to the target window.  In order to
	// support dragging of files/URLs as well as arbitrary text,
	// we look at the selection buffer - if the buffer starts
	// with a common URI scheme, does not contain spaces, and
	// contains at least one CR LF, then we flag the data as
	// both a URI list (MIME media type "text/uri-list") and
	// plain text.  Otherwise, we just say it is plain text.
        if ((!strncmp(fl_selection_buffer[0], "file:///", 8) ||
	     !strncmp(fl_selection_buffer[0], "ftp://", 6) ||
	     !strncmp(fl_selection_buffer[0], "http://", 7) ||
	     !strncmp(fl_selection_buffer[0], "https://", 8) ||
	     !strncmp(fl_selection_buffer[0], "ipp://", 6) ||
	     !strncmp(fl_selection_buffer[0], "ldap:", 5) ||
	     !strncmp(fl_selection_buffer[0], "mailto:", 7) ||
	     !strncmp(fl_selection_buffer[0], "news:", 5) ||
	     !strncmp(fl_selection_buffer[0], "smb://", 6)) &&
	    !strchr(fl_selection_buffer[0], ' ') &&
	    strstr(fl_selection_buffer[0], "\r\n")) {
	  // Send file/URI list...
	  fl_sendClientMessage(target_window, fl_XdndEnter, source_window,
			       dndversion<<24, fl_XdndURIList, XA_STRING, 0);
        } else {
	  // Send plain text...
	  fl_sendClientMessage(target_window, fl_XdndEnter, source_window,
			       dndversion<<24, XA_STRING, 0, 0);
	}
      }
    }
    if (local_window) {
      local_handle(FL_DND_DRAG, local_window);
    } else if (dndversion) {
      fl_sendClientMessage(target_window, fl_XdndPosition, source_window,
			   0, (e_x_root<<16)|e_y_root, fl_event_time,
			   fl_XdndActionCopy);
    }
    Fl::wait();
  }

  if (local_window) {
    fl_i_own_selection[0] = 1;
    if (local_handle(FL_DND_RELEASE, local_window)) paste(*belowmouse(), 0);
  } else if (dndversion) {
    fl_sendClientMessage(target_window, fl_XdndDrop, source_window,
			 0, fl_event_time);
  } else if (target_window) {
    // fake a drop by clicking the middle mouse button:
    XButtonEvent msg;
    msg.type = ButtonPress;
    msg.window = target_window;
    msg.root = RootWindow(fl_display, fl_screen);
    msg.subwindow = 0;
    msg.time = fl_event_time+1;
    msg.x = dest_x;
    msg.y = dest_y;
    msg.x_root = Fl::e_x_root;
    msg.y_root = Fl::e_y_root;
    msg.state = 0x0;
    msg.button = Button2;
    XSendEvent(fl_display, target_window, False, 0L, (XEvent*)&msg);
    msg.time++;
    msg.state = 0x200;
    msg.type = ButtonRelease;
    XSendEvent(fl_display, target_window, False, 0L, (XEvent*)&msg);
  }

  fl_local_grab = 0;
  source_fl_win->cursor(FL_CURSOR_DEFAULT);
  return 1;
}


//
// End of "$Id: fl_dnd_x.cxx 5190 2006-06-09 16:16:34Z mike $".
//
