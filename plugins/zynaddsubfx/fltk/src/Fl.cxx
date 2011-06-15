//
// "$Id: Fl.cxx 8723 2011-05-23 16:49:02Z manolo $"
//
// Main event handling code for the Fast Light Tool Kit (FLTK).
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


// warning: the Apple Quartz version still uses some Quickdraw calls,
//          mostly to get around the single active context in QD and
//          to implement clipping. This should be changed into pure
//          Quartz calls in the near future.
#include <config.h>

/* We require Windows 2000 features (e.g. VK definitions) */
#if defined(WIN32)
# if !defined(WINVER) || (WINVER < 0x0500)
#  ifdef WINVER
#   undef WINVER
#  endif
#  define WINVER 0x0500
# endif
# if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#  ifdef _WIN32_WINNT
#   undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT 0x0500
# endif
#endif

// recent versions of MinGW warn: "Please include winsock2.h before windows.h",
// hence we must include winsock2.h before FL/Fl.H (A.S. Dec. 2010, IMM May 2011)
#if defined(WIN32) && !defined(__CYGWIN__)
#  include <winsock2.h>
#endif

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tooltip.H>
#include <FL/x.H>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"

#if defined(DEBUG) || defined(DEBUG_WATCH)
#  include <stdio.h>
#endif // DEBUG || DEBUG_WATCH

#ifdef WIN32
#  include <ole2.h>
void fl_free_fonts(void);
HBRUSH fl_brush_action(int action);
void fl_cleanup_pens(void);
void fl_release_dc(HWND,HDC);
void fl_cleanup_dc_list(void);
#elif defined(__APPLE__)
extern double fl_mac_flush_and_wait(double time_to_wait, char in_idle);
#endif // WIN32

//
// Globals...
//
#if defined(__APPLE__) || defined(FL_DOXYGEN)
const char *Fl_Mac_App_Menu::about = "About ";
const char *Fl_Mac_App_Menu::print = "Print Front Window";
const char *Fl_Mac_App_Menu::services = "Services";
const char *Fl_Mac_App_Menu::hide = "Hide ";
const char *Fl_Mac_App_Menu::hide_others = "Hide Others";
const char *Fl_Mac_App_Menu::show = "Show All";
const char *Fl_Mac_App_Menu::quit = "Quit ";
#endif // __APPLE__
#ifndef FL_DOXYGEN
Fl_Widget	*Fl::belowmouse_,
		*Fl::pushed_,
		*Fl::focus_,
		*Fl::selection_owner_;
int		Fl::damage_,
		Fl::e_number,
		Fl::e_x,
		Fl::e_y,
		Fl::e_x_root,
		Fl::e_y_root,
		Fl::e_dx,
		Fl::e_dy,
		Fl::e_state,
		Fl::e_clicks,
		Fl::e_is_click,
		Fl::e_keysym,
                Fl::e_original_keysym,
		Fl::scrollbar_size_ = 16;

char		*Fl::e_text = (char *)"";
int		Fl::e_length;

Fl_Event_Dispatch Fl::e_dispatch = 0;

unsigned char   Fl::options_[] = { 0, 0 };
unsigned char   Fl::options_read_ = 0;


Fl_Window *fl_xfocus;	// which window X thinks has focus
Fl_Window *fl_xmousewin;// which window X thinks has FL_ENTER
Fl_Window *Fl::grab_;	// most recent Fl::grab()
Fl_Window *Fl::modal_;	// topmost modal() window

#endif // FL_DOXYGEN

//
// 'Fl::version()' - Return the API version number...
//

double
/**
  Returns the compiled-in value of the FL_VERSION constant. This
  is useful for checking the version of a shared library.
*/
Fl::version() {
  return FL_VERSION;
}

/**
  Gets the default scrollbar size used by
  Fl_Browser_,
  Fl_Help_View,
  Fl_Scroll, and
  Fl_Text_Display widgets.
  \returns The default size for widget scrollbars, in pixels.
*/
int Fl::scrollbar_size() {
  return scrollbar_size_;
}

/**
  Sets the default scrollbar size that is used by the
  Fl_Browser_,
  Fl_Help_View,
  Fl_Scroll, and
  Fl_Text_Display widgets.
  \param[in] W The new default size for widget scrollbars, in pixels.
*/
void Fl::scrollbar_size(int W) {
  scrollbar_size_ = W;
}


/** Returns whether or not the mouse event is inside the given rectangle.

    Returns non-zero if the current Fl::event_x() and Fl::event_y()
    put it inside the given arbitrary bounding box.

    You should always call this rather than doing your own comparison
    so you are consistent about edge effects.

    To find out, whether the event is inside a child widget of the
    current window, you can use Fl::event_inside(const Fl_Widget *).

    \param[in] xx,yy,ww,hh	bounding box
    \return			non-zero, if mouse event is inside
*/
int Fl::event_inside(int xx,int yy,int ww,int hh) /*const*/ {
  int mx = e_x - xx;
  int my = e_y - yy;
  return (mx >= 0 && mx < ww && my >= 0 && my < hh);
}

/** Returns whether or not the mouse event is inside a given child widget.

    Returns non-zero if the current Fl::event_x() and Fl::event_y()
    put it inside the given child widget's bounding box.

    This method can only be used to check whether the mouse event is
    inside a \b child widget of the window that handles the event, and
    there must not be an intermediate subwindow (i.e. the widget must
    not be inside a subwindow of the current window). However, it is
    valid if the widget is inside a nested Fl_Group.

    You must not use it with the window itself as the \p o argument
    in a window's handle() method.

    \note The mentioned restrictions are necessary, because this method
    does not transform coordinates of child widgets, and thus the given
    widget \p o must be within the \e same window that is handling the
    current event. Otherwise the results are undefined.

    You should always call this rather than doing your own comparison
    so you are consistent about edge effects.

    \see Fl::event_inside(int, int, int, int)

    \param[in] o	child widget to be tested
    \return		non-zero, if mouse event is inside the widget
*/
int Fl::event_inside(const Fl_Widget *o) /*const*/ {
  int mx = e_x - o->x();
  int my = e_y - o->y();
  return (mx >= 0 && mx < o->w() && my >= 0 && my < o->h());
}

//
//
// timer support
//

#ifdef WIN32

// implementation in Fl_win32.cxx

#elif defined(__APPLE__)

// implementation in Fl_mac.cxx

#else

//
// X11 timers
//


////////////////////////////////////////////////////////////////////////
// Timeouts are stored in a sorted list (*first_timeout), so only the
// first one needs to be checked to see if any should be called.
// Allocated, but unused (free) Timeout structs are stored in another
// linked list (*free_timeout).

struct Timeout {
  double time;
  void (*cb)(void*);
  void* arg;
  Timeout* next;
};
static Timeout* first_timeout, *free_timeout;

#include <sys/time.h>

// I avoid the overhead of getting the current time when we have no
// timeouts by setting this flag instead of getting the time.
// In this case calling elapse_timeouts() does nothing, but records
// the current time, and the next call will actually elapse time.
static char reset_clock = 1;

static void elapse_timeouts() {
  static struct timeval prevclock;
  struct timeval newclock;
  gettimeofday(&newclock, NULL);
  double elapsed = newclock.tv_sec - prevclock.tv_sec +
    (newclock.tv_usec - prevclock.tv_usec)/1000000.0;
  prevclock.tv_sec = newclock.tv_sec;
  prevclock.tv_usec = newclock.tv_usec;
  if (reset_clock) {
    reset_clock = 0;
  } else if (elapsed > 0) {
    for (Timeout* t = first_timeout; t; t = t->next) t->time -= elapsed;
  }
}

// Continuously-adjusted error value, this is a number <= 0 for how late
// we were at calling the last timeout. This appears to make repeat_timeout
// very accurate even when processing takes a significant portion of the
// time interval:
static double missed_timeout_by;

void Fl::add_timeout(double time, Fl_Timeout_Handler cb, void *argp) {
  elapse_timeouts();
  repeat_timeout(time, cb, argp);
}

void Fl::repeat_timeout(double time, Fl_Timeout_Handler cb, void *argp) {
  time += missed_timeout_by; if (time < -.05) time = 0;
  Timeout* t = free_timeout;
  if (t) {
      free_timeout = t->next;
  } else {
      t = new Timeout;
  }
  t->time = time;
  t->cb = cb;
  t->arg = argp;
  // insert-sort the new timeout:
  Timeout** p = &first_timeout;
  while (*p && (*p)->time <= time) p = &((*p)->next);
  t->next = *p;
  *p = t;
}

/**
  Returns true if the timeout exists and has not been called yet.
*/
int Fl::has_timeout(Fl_Timeout_Handler cb, void *argp) {
  for (Timeout* t = first_timeout; t; t = t->next)
    if (t->cb == cb && t->arg == argp) return 1;
  return 0;
}

/**
  Removes a timeout callback. It is harmless to remove a timeout
  callback that no longer exists.

  \note	This version removes all matching timeouts, not just the first one.
	This may change in the future.
*/
void Fl::remove_timeout(Fl_Timeout_Handler cb, void *argp) {
  for (Timeout** p = &first_timeout; *p;) {
    Timeout* t = *p;
    if (t->cb == cb && (t->arg == argp || !argp)) {
      *p = t->next;
      t->next = free_timeout;
      free_timeout = t;
    } else {
      p = &(t->next);
    }
  }
}

#endif

////////////////////////////////////////////////////////////////
// Checks are just stored in a list. They are called in the reverse
// order that they were added (this may change in the future).
// This is a bit messy because I want to allow checks to be added,
// removed, and have wait() called from inside them. To do this
// next_check points at the next unprocessed one for the outermost
// call to Fl::wait().

struct Check {
  void (*cb)(void*);
  void* arg;
  Check* next;
};
static Check *first_check, *next_check, *free_check;

/**
  FLTK will call this callback just before it flushes the display and
  waits for events.  This is different than an idle callback because it
  is only called once, then FLTK calls the system and tells it not to
  return until an event happens.

  This can be used by code that wants to monitor the
  application's state, such as to keep a display up to date. The
  advantage of using a check callback is that it is called only when no
  events are pending. If events are coming in quickly, whole blocks of
  them will be processed before this is called once. This can save
  significant time and avoid the application falling behind the events.

  Sample code:

  \code
  bool state_changed; // anything that changes the display turns this on

  void callback(void*) {
   if (!state_changed) return;
   state_changed = false;
   do_expensive_calculation();
   widget-&gt;redraw();
  }

  main() {
   Fl::add_check(callback);
   return Fl::run();
  }
  \endcode
*/
void Fl::add_check(Fl_Timeout_Handler cb, void *argp) {
  Check* t = free_check;
  if (t) free_check = t->next;
  else t = new Check;
  t->cb = cb;
  t->arg = argp;
  t->next = first_check;
  if (next_check == first_check) next_check = t;
  first_check = t;
}

/**
  Removes a check callback. It is harmless to remove a check
  callback that no longer exists.
*/
void Fl::remove_check(Fl_Timeout_Handler cb, void *argp) {
  for (Check** p = &first_check; *p;) {
    Check* t = *p;
    if (t->cb == cb && t->arg == argp) {
      if (next_check == t) next_check = t->next;
      *p = t->next;
      t->next = free_check;
      free_check = t;
    } else {
      p = &(t->next);
    }
  }
}

/**
  Returns 1 if the check exists and has not been called yet, 0 otherwise.
*/
int Fl::has_check(Fl_Timeout_Handler cb, void *argp) {
  for (Check** p = &first_check; *p;) {
    Check* t = *p;
    if (t->cb == cb && t->arg == argp) {
      return 1;
    } else {
      p = &(t->next);
    }
  }
  return 0;
}

static void run_checks()
{
  // checks are a bit messy so that add/remove and wait may be called
  // from inside them without causing an infinite loop:
  if (next_check == first_check) {
    while (next_check) {
      Check* checkp = next_check;
      next_check = checkp->next;
      (checkp->cb)(checkp->arg);
    }
    next_check = first_check;
  }
}

#ifndef WIN32
static char in_idle;
#endif

////////////////////////////////////////////////////////////////
// wait/run/check/ready:

void (*Fl::idle)(); // see Fl::add_idle.cxx for the add/remove functions

extern int fl_ready(); // in Fl_<platform>.cxx
extern int fl_wait(double time); // in Fl_<platform>.cxx

/**
  See int Fl::wait()
*/
double Fl::wait(double time_to_wait) {
  // delete all widgets that were listed during callbacks
  do_widget_deletion();

#ifdef WIN32

  return fl_wait(time_to_wait);

#elif defined(__APPLE__)

  run_checks();
  if (idle) {
    if (!in_idle) {
      in_idle = 1;
      idle();
      in_idle = 0;
    }
    // the idle function may turn off idle, we can then wait:
    if (idle) time_to_wait = 0.0;
  }
  return fl_mac_flush_and_wait(time_to_wait, in_idle);

#else

  if (first_timeout) {
    elapse_timeouts();
    Timeout *t;
    while ((t = first_timeout)) {
      if (t->time > 0) break;
      // The first timeout in the array has expired.
      missed_timeout_by = t->time;
      // We must remove timeout from array before doing the callback:
      void (*cb)(void*) = t->cb;
      void *argp = t->arg;
      first_timeout = t->next;
      t->next = free_timeout;
      free_timeout = t;
      // Now it is safe for the callback to do add_timeout:
      cb(argp);
    }
  } else {
    reset_clock = 1; // we are not going to check the clock
  }
  run_checks();
//  if (idle && !fl_ready()) {
  if (idle) {
    if (!in_idle) {
      in_idle = 1;
      idle();
      in_idle = 0;
    }
    // the idle function may turn off idle, we can then wait:
    if (idle) time_to_wait = 0.0;
  }
  if (first_timeout && first_timeout->time < time_to_wait)
    time_to_wait = first_timeout->time;
  if (time_to_wait <= 0.0) {
    // do flush second so that the results of events are visible:
    int ret = fl_wait(0.0);
    flush();
    return ret;
  } else {
    // do flush first so that user sees the display:
    flush();
    if (idle && !in_idle) // 'idle' may have been set within flush()
      time_to_wait = 0.0;
    return fl_wait(time_to_wait);
  }
#endif
}

#define FOREVER 1e20

/**
  As long as any windows are displayed this calls Fl::wait()
  repeatedly.  When all the windows are closed it returns zero
  (supposedly it would return non-zero on any errors, but FLTK calls
  exit directly for these).  A normal program will end main()
  with return Fl::run();.
*/
int Fl::run() {
  while (Fl_X::first) wait(FOREVER);
  return 0;
}

#ifdef WIN32

// Function to initialize COM/OLE for usage. This must be done only once.
// We define a flag to register whether we called it:
static char oleInitialized = 0;

// This calls the Windows function OleInitialize() exactly once.
void fl_OleInitialize() {
  if (!oleInitialized) {
    OleInitialize(0L);
    oleInitialized = 1;
  }
}

// This calls the Windows function OleUninitialize() only, if
// OleInitialize has been called before.
void fl_OleUninitialize() {
  if (oleInitialized) {
    OleUninitialize();
    oleInitialized = 0;
  }
}

class Fl_Win32_At_Exit {
public:
  Fl_Win32_At_Exit() { }
  ~Fl_Win32_At_Exit() {
    fl_free_fonts();        // do some WIN32 cleanup
    fl_cleanup_pens();
    fl_OleUninitialize();
    fl_brush_action(1);
    fl_cleanup_dc_list();
  }
};
static Fl_Win32_At_Exit win32_at_exit;
#endif



/**
  Waits until "something happens" and then returns.  Call this
  repeatedly to "run" your program.  You can also check what happened
  each time after this returns, which is quite useful for managing
  program state.

  What this really does is call all idle callbacks, all elapsed
  timeouts, call Fl::flush() to get the screen to update, and
  then wait some time (zero if there are idle callbacks, the shortest of
  all pending timeouts, or infinity), for any events from the user or
  any Fl::add_fd() callbacks.  It then handles the events and
  calls the callbacks and then returns.

  The return value of Fl::wait() is non-zero if there are any
  visible windows - this may change in future versions of FLTK.

  Fl::wait(time) waits a maximum of \e time seconds.
  <i>It can return much sooner if something happens.</i>

  The return value is positive if an event or fd happens before the
  time elapsed.  It is zero if nothing happens (on Win32 this will only
  return zero if \e time is zero).  It is negative if an error
  occurs (this will happen on UNIX if a signal happens).
*/
int Fl::wait() {
  if (!Fl_X::first) return 0;
  wait(FOREVER);
  return Fl_X::first != 0; // return true if there is a window
}

/**
  Same as Fl::wait(0).  Calling this during a big calculation
  will keep the screen up to date and the interface responsive:

  \code
  while (!calculation_done()) {
  calculate();
  Fl::check();
  if (user_hit_abort_button()) break;
  }
  \endcode

  This returns non-zero if any windows are displayed, and 0 if no
  windows are displayed (this is likely to change in future versions of
  FLTK).
*/
int Fl::check() {
  wait(0.0);
  return Fl_X::first != 0; // return true if there is a window
}

/**
  This is similar to Fl::check() except this does \e not
  call Fl::flush() or any callbacks, which is useful if your
  program is in a state where such callbacks are illegal.  This returns
  true if Fl::check() would do anything (it will continue to
  return true until you call Fl::check() or Fl::wait()).

  \code
  while (!calculation_done()) {
    calculate();
    if (Fl::ready()) {
      do_expensive_cleanup();
      Fl::check();
      if (user_hit_abort_button()) break;
    }
  }
  \endcode
*/
int Fl::ready() {
#if ! defined( WIN32 )  &&  ! defined(__APPLE__)
  if (first_timeout) {
    elapse_timeouts();
    if (first_timeout->time <= 0) return 1;
  } else {
    reset_clock = 1;
  }
#endif
  return fl_ready();
}

////////////////////////////////////////////////////////////////
// Window list management:

#ifndef FL_DOXYGEN
Fl_X* Fl_X::first;
#endif

Fl_Window* fl_find(Window xid) {
  Fl_X *window;
  for (Fl_X **pp = &Fl_X::first; (window = *pp); pp = &window->next)
#if defined(WIN32) || defined(USE_X11)
   if (window->xid == xid)
#elif defined(__APPLE_QUARTZ__)
   if (window->xid == xid && !window->w->window())
#else
# error unsupported platform
#endif // __APPLE__
	{
      if (window != Fl_X::first && !Fl::modal()) {
	// make this window be first to speed up searches
	// this is not done if modal is true to avoid messing up modal stack
	*pp = window->next;
	window->next = Fl_X::first;
	Fl_X::first = window;
      }
      return window->w;
    }
  return 0;
}

/**
  Returns the first top-level window in the list of shown() windows.  If
  a modal() window is shown this is the top-most modal window, otherwise
  it is the most recent window to get an event.
*/
Fl_Window* Fl::first_window() {
  Fl_X* i = Fl_X::first;
  return i ? i->w : 0;
}

/**
  Returns the next top-level window in the list of shown() windows.
  You can use this call to iterate through all the windows that are shown().
  \param[in] window	must be shown and not NULL
*/
Fl_Window* Fl::next_window(const Fl_Window* window) {
  Fl_X* i = Fl_X::i(window)->next;
  return i ? i->w : 0;
}

/**
 Sets the window that is returned by first_window().
 The window is removed from wherever it is in the
 list and inserted at the top.  This is not done if Fl::modal()
 is on or if the window is not shown(). Because the first window
 is used to set the "parent" of modal windows, this is often
 useful.
 */
void Fl::first_window(Fl_Window* window) {
  if (!window || !window->shown()) return;
  fl_find( Fl_X::i(window)->xid );
}

/**
  Redraws all widgets.
*/
void Fl::redraw() {
  for (Fl_X* i = Fl_X::first; i; i = i->next) i->w->redraw();
}

/**
  Causes all the windows that need it to be redrawn and graphics forced
  out through the pipes.

  This is what wait() does before looking for events.

  Note: in multi-threaded applications you should only call Fl::flush()
  from the main thread. If a child thread needs to trigger a redraw event,
  it should instead call Fl::awake() to get the main thread to process the
  event queue.
*/
void Fl::flush() {
  if (damage()) {
    damage_ = 0;
    for (Fl_X* i = Fl_X::first; i; i = i->next) {
      if (i->wait_for_expose) {damage_ = 1; continue;}
      Fl_Window* wi = i->w;
      if (!wi->visible_r()) continue;
      if (wi->damage()) {i->flush(); wi->clear_damage();}
      // destroy damage regions for windows that don't use them:
      if (i->region) {XDestroyRegion(i->region); i->region = 0;}
    }
  }
#if defined(USE_X11)
  if (fl_display) XFlush(fl_display);
#elif defined(WIN32)
  GdiFlush();
#elif defined (__APPLE_QUARTZ__)
  if (fl_gc)
    CGContextFlush(fl_gc);
#else
# error unsupported platform
#endif
}


////////////////////////////////////////////////////////////////
// Event handlers:


struct handler_link {
  int (*handle)(int);
  handler_link *next;
};


static handler_link *handlers = 0;


/**
  Install a function to parse unrecognized events.  If FLTK cannot
  figure out what to do with an event, it calls each of these functions
  (most recent first) until one of them returns non-zero.  If none of
  them returns non-zero then the event is ignored.  Events that cause
  this to be called are:

  - FL_SHORTCUT events that are not recognized by any widget.
    This lets you provide global shortcut keys.
  - System events that FLTK does not recognize.  See fl_xevent.
  - \e Some other events when the widget FLTK selected returns
    zero from its handle() method.  Exactly which ones may change
    in future versions, however.

 \see Fl::remove_handler(Fl_Event_Handler)
 \see Fl::event_dispatch(Fl_Event_Dispatch d)
 \see Fl::handle(int, Fl_Window*)
*/
void Fl::add_handler(Fl_Event_Handler ha) {
  handler_link *l = new handler_link;
  l->handle = ha;
  l->next = handlers;
  handlers = l;
}


/**
 Removes a previously added event handler.
 \see Fl::handle(int, Fl_Window*)
*/
void Fl::remove_handler(Fl_Event_Handler ha) {
  handler_link *l, *p;

  // Search for the handler in the list...
  for (l = handlers, p = 0; l && l->handle != ha; p = l, l = l->next);

  if (l) {
    // Found it, so remove it from the list...
    if (p) p->next = l->next;
    else handlers = l->next;

    // And free the record...
    delete l;
  }
}

int (*fl_local_grab)(int); // used by fl_dnd.cxx

static int send_handlers(int e) {
  for (const handler_link *hl = handlers; hl; hl = hl->next)
    if (hl->handle(e)) return 1;
  return 0;
}

////////////////////////////////////////////////////////////////

Fl_Widget* fl_oldfocus; // kludge for Fl_Group...

/**
    Sets the widget that will receive FL_KEYBOARD events.

    If you change Fl::focus(), the previous widget and all
    parents (that don't contain the new widget) are sent FL_UNFOCUS
    events.  Changing the focus does \e not send FL_FOCUS to
    this or any widget, because sending FL_FOCUS is supposed to
    \e test if the widget wants the focus (by it returning non-zero from
    handle()).

    \see Fl_Widget::take_focus()
*/
void Fl::focus(Fl_Widget *o) {
  if (o && !o->visible_focus()) return;
  if (grab()) return; // don't do anything while grab is on
  Fl_Widget *p = focus_;
  if (o != p) {
    Fl::compose_reset();
    focus_ = o;
    // make sure that fl_xfocus is set to the top level window
    // of this widget, or fl_fix_focus will clear our focus again
    if (o) {
      Fl_Window *win = 0, *w1 = o->as_window();
      if (!w1) w1 = o->window();
      while (w1) { win=w1; w1=win->window(); }
      if (win) {
#ifdef __APPLE__
	if (fl_xfocus != win) {
	  Fl_X *x = Fl_X::i(win);
	  if (x) x->set_key_window();
	  }
#endif
	fl_xfocus = win;
	}
    }
    // take focus from the old focused window
    fl_oldfocus = 0;
    int old_event = e_number;
    e_number = FL_UNFOCUS;
    for (; p; p = p->parent()) {
      p->handle(FL_UNFOCUS);
      fl_oldfocus = p;
    }
    e_number = old_event;
  }
}

static char dnd_flag = 0; // make 'belowmouse' send DND_LEAVE instead of LEAVE

/**
    Sets the widget that is below the mouse.  This is for
    highlighting buttons.  It is not used to send FL_PUSH or
    FL_MOVE directly, for several obscure reasons, but those events
    typically go to this widget.  This is also the first widget tried for
    FL_SHORTCUT events.

    If you change the belowmouse widget, the previous one and all
    parents (that don't contain the new widget) are sent FL_LEAVE
    events.  Changing this does \e not send FL_ENTER to this
    or any widget, because sending FL_ENTER is supposed to \e test
    if the widget wants the mouse (by it returning non-zero from
    handle()).
*/
void Fl::belowmouse(Fl_Widget *o) {
  if (grab()) return; // don't do anything while grab is on
  Fl_Widget *p = belowmouse_;
  if (o != p) {
    belowmouse_ = o;
    int old_event = e_number;
    e_number = dnd_flag ? FL_DND_LEAVE : FL_LEAVE;
    for (; p && !p->contains(o); p = p->parent()) {
      p->handle(e_number);
    }
    e_number = old_event;
  }
}

/**
    Sets the widget that is being pushed. FL_DRAG or
    FL_RELEASE (and any more FL_PUSH) events will be sent to
    this widget.

    If you change the pushed widget, the previous one and all parents
    (that don't contain the new widget) are sent FL_RELEASE
    events.  Changing this does \e not send FL_PUSH to this
    or any widget, because sending FL_PUSH is supposed to \e test
    if the widget wants the mouse (by it returning non-zero from
    handle()).
*/
 void Fl::pushed(Fl_Widget *o) {
  pushed_ = o;
}

static void nothing(Fl_Widget *) {}
void (*Fl_Tooltip::enter)(Fl_Widget *) = nothing;
void (*Fl_Tooltip::exit)(Fl_Widget *) = nothing;

// Update modal(), focus() and other state according to system state,
// and send FL_ENTER, FL_LEAVE, FL_FOCUS, and/or FL_UNFOCUS events.
// This is the only function that produces these events in response
// to system activity.
// This is called whenever a window is added or hidden, and whenever
// X says the focus or mouse window have changed.

void fl_fix_focus() {
#ifdef DEBUG
  puts("fl_fix_focus();");
#endif // DEBUG

  if (Fl::grab()) return; // don't do anything while grab is on.

  // set focus based on Fl::modal() and fl_xfocus
  Fl_Widget* w = fl_xfocus;
  if (w) {
    int saved = Fl::e_keysym;
    if (Fl::e_keysym < (FL_Button + FL_LEFT_MOUSE) ||
        Fl::e_keysym > (FL_Button + FL_RIGHT_MOUSE))
      Fl::e_keysym = 0; // make sure widgets don't think a keystroke moved focus
    while (w->parent()) w = w->parent();
    if (Fl::modal()) w = Fl::modal();
    if (!w->contains(Fl::focus()))
      if (!w->take_focus()) Fl::focus(w);
    Fl::e_keysym = saved;
  } else
    Fl::focus(0);

// MRS: Originally we checked the button state, but a user reported that it
//      broke click-to-focus in FLWM?!?
//  if (!(Fl::event_state() & 0x7f00000 /*FL_BUTTONS*/)) {
  if (!Fl::pushed()) {
    // set belowmouse based on Fl::modal() and fl_xmousewin:
    w = fl_xmousewin;
    if (w) {
      if (Fl::modal()) w = Fl::modal();
      if (!w->contains(Fl::belowmouse())) {
        int old_event = Fl::e_number;
	w->handle(Fl::e_number = FL_ENTER);
	Fl::e_number = old_event;
	if (!w->contains(Fl::belowmouse())) Fl::belowmouse(w);
      } else {
	// send a FL_MOVE event so the enter/leave state is up to date
	Fl::e_x = Fl::e_x_root-fl_xmousewin->x();
	Fl::e_y = Fl::e_y_root-fl_xmousewin->y();
        int old_event = Fl::e_number;
	w->handle(Fl::e_number = FL_MOVE);
	Fl::e_number = old_event;
      }
    } else {
      Fl::belowmouse(0);
      Fl_Tooltip::enter(0);
    }
  }
}

#if !(defined(WIN32) || defined(__APPLE__))
extern Fl_Widget *fl_selection_requestor; // from Fl_x.cxx
#endif

// This function is called by ~Fl_Widget() and by Fl_Widget::deactivate()
// and by Fl_Widget::hide().  It indicates that the widget does not want
// to receive any more events, and also removes all global variables that
// point at the widget.
// I changed this from the 1.0.1 behavior, the older version could send
// FL_LEAVE or FL_UNFOCUS events to the widget.  This appears to not be
// desirable behavior and caused flwm to crash.

void fl_throw_focus(Fl_Widget *o) {
#ifdef DEBUG
  printf("fl_throw_focus(o=%p)\n", o);
#endif // DEBUG

  if (o->contains(Fl::pushed())) Fl::pushed_ = 0;
#if !(defined(WIN32) || defined(__APPLE__))
  if (o->contains(fl_selection_requestor)) fl_selection_requestor = 0;
#endif
  if (o->contains(Fl::belowmouse())) Fl::belowmouse_ = 0;
  if (o->contains(Fl::focus())) Fl::focus_ = 0;
  if (o == fl_xfocus) fl_xfocus = 0;
  if (o == Fl_Tooltip::current()) Fl_Tooltip::current(0);
  if (o == fl_xmousewin) fl_xmousewin = 0;
  Fl_Tooltip::exit(o);
  fl_fix_focus();
}

////////////////////////////////////////////////////////////////

// Call to->handle(), but first replace the mouse x/y with the correct
// values to account for nested windows. 'window' is the outermost
// window the event was posted to by the system:
static int send(int event, Fl_Widget* to, Fl_Window* window) {
  int dx, dy;
  int old_event = Fl::e_number;
  if (window) {
    dx = window->x();
    dy = window->y();
  } else {
    dx = dy = 0;
  }
  for (const Fl_Widget* w = to; w; w = w->parent())
    if (w->type()>=FL_WINDOW) {dx -= w->x(); dy -= w->y();}
  int save_x = Fl::e_x; Fl::e_x += dx;
  int save_y = Fl::e_y; Fl::e_y += dy;
  int ret = to->handle(Fl::e_number = event);
  Fl::e_number = old_event;
  Fl::e_y = save_y;
  Fl::e_x = save_x;
  return ret;
}


/**
 \brief Set a new event dispatch function.

 The event dispatch function is called after native events are converted to
 FLTK events, but before they are handled by FLTK. If the dispatch function
 Fl_Event_Dispatch \p d is set, it is up to the dispatch function to call
 Fl::handle_(int, Fl_Window*) or to ignore the event.

 The dispatch function itself must return 0 if it ignored the event,
 or non-zero if it used the event. If you call Fl::handle_(), then
 this will return the correct value.

 The event dispatch can be used to handle exceptions in FLTK events and
 callbacks before they reach the native event handler:

 \code
 int myHandler(int e, Fl_Window *w) {
   try {
     return Fl::handle_(e, w);
   } catch () {
     ...
   }
 }

 main() {
   Fl::event_dispatch(myHandler);
   ...
   Fl::run();
 }
 \endcode

 \param d new dispatch function, or NULL
 \see Fl::add_handler(Fl_Event_Handler)
 \see Fl::handle(int, Fl_Window*)
 \see Fl::handle_(int, Fl_Window*)
 */
void Fl::event_dispatch(Fl_Event_Dispatch d)
{
  e_dispatch = d;
}


/**
 \brief Return the current event dispatch function.
 */
Fl_Event_Dispatch Fl::event_dispatch()
{
  return e_dispatch;
}


/**
 \brief Handle events from the window system.

 This is called from the native event dispatch after native events have been
 converted to FLTK notation. This function calls Fl::handle_(int, Fl_Window*)
 unless the user sets a dispatch function. If a user dispatch function is set,
 the user must make sure that Fl::handle_() is called, or the event will be
 ignored.

 \param e the event type (Fl::event_number() is not yet set)
 \param window the window that caused this event
 \return 0 if the event was not handled

 \see Fl::add_handler(Fl_Event_Handler)
 \see Fl::event_dispatch(Fl_Event_Dispatch)
 */
int Fl::handle(int e, Fl_Window* window)
{
  if (e_dispatch) {
    return e_dispatch(e, window);
  } else {
    return handle_(e, window);
  }
}


/**
 \brief Handle events from the window system.

 This function is called from the native event dispatch, unless the user sets
 another dispatch function. In that case, the user dispatch function must
 decide when to call Fl::handle_(int, Fl_Window*)

 \param e the event type (Fl::event_number() is not yet set)
 \param window the window that caused this event
 \return 0 if the event was not handled

 \see Fl::event_dispatch(Fl_Event_Dispatch)
 */
int Fl::handle_(int e, Fl_Window* window)
{
  e_number = e;
  if (fl_local_grab) return fl_local_grab(e);

  Fl_Widget* wi = window;

  switch (e) {

  case FL_CLOSE:
    if ( grab() || (modal() && window != modal()) ) return 0;
    wi->do_callback();
    return 1;

  case FL_SHOW:
    wi->Fl_Widget::show(); // this calls Fl_Widget::show(), not Fl_Window::show()
    return 1;

  case FL_HIDE:
    wi->Fl_Widget::hide(); // this calls Fl_Widget::hide(), not Fl_Window::hide()
    return 1;

  case FL_PUSH:
#ifdef DEBUG
    printf("Fl::handle(e=%d, window=%p);\n", e, window);
#endif // DEBUG

    if (grab()) wi = grab();
    else if (modal() && wi != modal()) return 0;
    pushed_ = wi;
    Fl_Tooltip::current(wi);
    if (send(e, wi, window)) return 1;
    // raise windows that are clicked on:
    window->show();
    return 1;

  case FL_DND_ENTER:
  case FL_DND_DRAG:
    dnd_flag = 1;
    break;

  case FL_DND_LEAVE:
    dnd_flag = 1;
    belowmouse(0);
    dnd_flag = 0;
    return 1;

  case FL_DND_RELEASE:
    wi = belowmouse();
    break;

  case FL_MOVE:
  case FL_DRAG:
    fl_xmousewin = window; // this should already be set, but just in case.
    if (pushed()) {
      wi = pushed();
      if (grab()) wi = grab();
      e_number = e = FL_DRAG;
      break;
    }
    if (modal() && wi != modal()) wi = 0;
    if (grab()) wi = grab();
    { int ret;
      Fl_Widget* pbm = belowmouse();
#ifdef __APPLE__
      if (fl_mac_os_version < 100500) {
        // before 10.5, mouse moved events aren't sent to borderless windows such as tooltips
	Fl_Window *tooltip = Fl_Tooltip::current_window();
	int inside = 0;
	if (tooltip && tooltip->shown() ) { // check if a tooltip window is currently opened
	  // check if mouse is inside the tooltip
	  inside = (Fl::event_x_root() >= tooltip->x() && Fl::event_x_root() < tooltip->x() + tooltip->w() &&
	  Fl::event_y_root() >= tooltip->y() && Fl::event_y_root() < tooltip->y() + tooltip->h() );
	}
	// if inside, send event to tooltip window instead of background window
	if (inside) ret = send(e, tooltip, window);
	else ret = (wi && send(e, wi, window));
      } else
#endif
      ret = (wi && send(e, wi, window));
   if (pbm != belowmouse()) {
#ifdef DEBUG
      printf("Fl::handle(e=%d, window=%p);\n", e, window);
#endif // DEBUG
      Fl_Tooltip::enter(belowmouse());
    }
    return ret;}

  case FL_RELEASE: {
//    printf("FL_RELEASE: window=%p, pushed() = %p, grab() = %p, modal() = %p\n",
//           window, pushed(), grab(), modal());

    if (grab()) {
      wi = grab();
      pushed_ = 0; // must be zero before callback is done!
    } else if (pushed()) {
      wi = pushed();
      pushed_ = 0; // must be zero before callback is done!
    } else if (modal() && wi != modal()) return 0;
    int r = send(e, wi, window);
    fl_fix_focus();
    return r;}

  case FL_UNFOCUS:
    window = 0;
  case FL_FOCUS:
    fl_xfocus = window;
    fl_fix_focus();
    return 1;

  case FL_KEYUP:
    // Send the key-up to the current focus widget. This is not
    // always the same widget that received the corresponding
    // FL_KEYBOARD event because focus may have changed.
    // Sending the KEYUP to the right KEYDOWN is possible, but
    // would require that we track the KEYDOWN for every possible
    // key stroke (users may hold down multiple keys!) and then
    // make sure that the widget still exists before sending
    // a KEYUP there. I believe that the current solution is
    // "close enough".
    for (wi = grab() ? grab() : focus(); wi; wi = wi->parent())
      if (send(FL_KEYUP, wi, window)) return 1;
    return 0;

  case FL_KEYBOARD:
#ifdef DEBUG
    printf("Fl::handle(e=%d, window=%p);\n", e, window);
#endif // DEBUG

    Fl_Tooltip::enter((Fl_Widget*)0);

    fl_xfocus = window; // this should not happen!  But maybe it does:

    // Try it as keystroke, sending it to focus and all parents:
    for (wi = grab() ? grab() : focus(); wi; wi = wi->parent())
      if (send(FL_KEYBOARD, wi, window)) return 1;

    // recursive call to try shortcut:
    if (handle(FL_SHORTCUT, window)) return 1;

    // and then try a shortcut with the case of the text swapped, by
    // changing the text and falling through to FL_SHORTCUT case:
    {unsigned char* c = (unsigned char*)event_text(); // cast away const
    if (!isalpha(*c)) return 0;
    *c = isupper(*c) ? tolower(*c) : toupper(*c);}
    e_number = e = FL_SHORTCUT;

  case FL_SHORTCUT:
    if (grab()) {wi = grab(); break;} // send it to grab window

    // Try it as shortcut, sending to mouse widget and all parents:
    wi = belowmouse();
    if (!wi) {
      wi = modal();
      if (!wi) wi = window;
    } else if (wi->window() != first_window()) {
      if (send(FL_SHORTCUT, first_window(), first_window())) return 1;
    }

    for (; wi; wi = wi->parent()) {
      if (send(FL_SHORTCUT, wi, wi->window())) return 1;
    }

    // try using add_handle() functions:
    if (send_handlers(FL_SHORTCUT)) return 1;

    // make Escape key close windows:
    if (event_key()==FL_Escape) {
      wi = modal(); if (!wi) wi = window;
      wi->do_callback();
      return 1;
    }

    return 0;

  case FL_ENTER:
#ifdef DEBUG
    printf("Fl::handle(e=%d, window=%p);\n", e, window);
#endif // DEBUG

    fl_xmousewin = window;
    fl_fix_focus();
    Fl_Tooltip::enter(belowmouse());
    return 1;

  case FL_LEAVE:
#ifdef DEBUG
    printf("Fl::handle(e=%d, window=%p);\n", e, window);
#endif // DEBUG

    if (!pushed_) {
      belowmouse(0);
      Fl_Tooltip::enter(0);
    }
    if (window == fl_xmousewin) {fl_xmousewin = 0; fl_fix_focus();}
    return 1;

  case FL_MOUSEWHEEL:
    fl_xfocus = window; // this should not happen!  But maybe it does:

    // Try sending it to the "grab" first
    if (grab() && grab()!=modal() && grab()!=window) {
      if (send(FL_MOUSEWHEEL, grab(), window)) return 1;
    }
    // Now try sending it to the "modal" window
    if (modal()) {
      send(FL_MOUSEWHEEL, modal(), window);
      return 1;
    }
    // Finally try sending it to the window, the event occured in
    if (send(FL_MOUSEWHEEL, window, window)) return 1;
  default:
    break;
  }
  if (wi && send(e, wi, window)) {
    dnd_flag = 0;
    return 1;
  }
  dnd_flag = 0;
  return send_handlers(e);
}

////////////////////////////////////////////////////////////////
// hide() destroys the X window, it does not do unmap!

#if !defined(WIN32) && USE_XFT
extern void fl_destroy_xft_draw(Window);
#endif

void Fl_Window::hide() {
  clear_visible();

  if (!shown()) return;

  // remove from the list of windows:
  Fl_X* ip = i;
  Fl_X** pp = &Fl_X::first;
  for (; *pp != ip; pp = &(*pp)->next) if (!*pp) return;
  *pp = ip->next;
#ifdef __APPLE__
  ip->unlink();
  // MacOS X manages a single pointer per application. Make sure that hiding
  // a toplevel window will not leave us with some random pointer shape, or
  // worst case, an invisible pointer
  if (!parent()) cursor(FL_CURSOR_DEFAULT);
#endif
  i = 0;

  // recursively remove any subwindows:
  for (Fl_X *wi = Fl_X::first; wi;) {
    Fl_Window* W = wi->w;
    if (W->window() == this) {
      W->hide();
      W->set_visible();
      wi = Fl_X::first;
    } else wi = wi->next;
  }

  if (this == Fl::modal_) { // we are closing the modal window, find next one:
    Fl_Window* W;
    for (W = Fl::first_window(); W; W = Fl::next_window(W))
      if (W->modal()) break;
    Fl::modal_ = W;
  }

  // Make sure no events are sent to this window:
  fl_throw_focus(this);
  handle(FL_HIDE);

#if defined(WIN32)
  // this little trick keeps the current clipboard alive, even if we are about
  // to destroy the window that owns the selection.
  if (GetClipboardOwner()==ip->xid) {
    Fl_Window *w1 = Fl::first_window();
    if (w1 && OpenClipboard(fl_xid(w1))) {
      EmptyClipboard();
      SetClipboardData(CF_TEXT, NULL);
      CloseClipboard();
    }
  }
  // Send a message to myself so that I'll get out of the event loop...
  PostMessage(ip->xid, WM_APP, 0, 0);
  if (ip->private_dc) fl_release_dc(ip->xid, ip->private_dc);
    if (ip->xid == fl_window && fl_gc) {
      fl_release_dc(fl_window, fl_gc);
      fl_window = (HWND)-1;
      fl_gc = 0;
# ifdef FLTK_USE_CAIRO
      if (Fl::cairo_autolink_context()) Fl::cairo_make_current((Fl_Window*) 0);
# endif
    }
#elif defined(__APPLE_QUARTZ__)
  Fl_X::q_release_context(ip);
  if ( ip->xid == fl_window && !parent() )
    fl_window = 0;
#endif

  if (ip->region) XDestroyRegion(ip->region);

#if defined(USE_X11)
# if USE_XFT
  fl_destroy_xft_draw(ip->xid);
# endif
  // this test makes sure ip->xid has not been destroyed already
  if (ip->xid) XDestroyWindow(fl_display, ip->xid);
#elif defined(WIN32)
  // this little trickery seems to avoid the popup window stacking problem
  HWND p = GetForegroundWindow();
  if (p==GetParent(ip->xid)) {
    ShowWindow(ip->xid, SW_HIDE);
    ShowWindow(p, SW_SHOWNA);
  }
  XDestroyWindow(fl_display, ip->xid);
#elif defined(__APPLE_QUARTZ__)
  ip->destroy();
#else
# error unsupported platform
#endif

#ifdef WIN32
  // Try to stop the annoying "raise another program" behavior
  if (non_modal() && Fl::first_window() && Fl::first_window()->shown())
    Fl::first_window()->show();
#endif
  delete ip;
}

Fl_Window::~Fl_Window() {
  hide();
  if (xclass_) {
    free(xclass_);
  }
}

// FL_SHOW and FL_HIDE are called whenever the visibility of this widget
// or any parent changes.  We must correctly map/unmap the system's window.

// For top-level windows it is assumed the window has already been
// mapped or unmapped!!!  This is because this should only happen when
// Fl_Window::show() or Fl_Window::hide() is called, or in response to
// iconize/deiconize events from the system.

int Fl_Window::handle(int ev)
{
  if (parent()) {
    switch (ev) {
    case FL_SHOW:
      if (!shown()) show();
      else {
#if defined(USE_X11) || defined(WIN32)
        XMapWindow(fl_display, fl_xid(this)); // extra map calls are harmless
#elif defined(__APPLE_QUARTZ__)
	i->map();
#else
# error unsupported platform
#endif // __APPLE__
      }
      break;
    case FL_HIDE:
      if (shown()) {
	// Find what really turned invisible, if it was a parent window
	// we do nothing.  We need to avoid unnecessary unmap calls
	// because they cause the display to blink when the parent is
	// remapped.  However if this or any intermediate non-window
	// widget has really had hide() called directly on it, we must
	// unmap because when the parent window is remapped we don't
	// want to reappear.
	if (visible()) {
	 Fl_Widget* p = parent(); for (;p->visible();p = p->parent()) {}
	 if (p->type() >= FL_WINDOW) break; // don't do the unmap
	}
#if defined(USE_X11) || defined(WIN32)
	XUnmapWindow(fl_display, fl_xid(this));
#elif defined(__APPLE_QUARTZ__)
	i->unmap();
#else
# error platform unsupported
#endif
      }
      break;
    }
//  } else if (ev == FL_FOCUS || ev == FL_UNFOCUS) {
//    Fl_Tooltip::exit(Fl_Tooltip::current());
  }

  return Fl_Group::handle(ev);
}

////////////////////////////////////////////////////////////////
// Back compatibility cut & paste functions for fltk 1.1 only:

/** Back-compatibility only: The single-argument call can be used to
    move the selection to another widget or to set the owner to
    NULL, without changing the actual text of the
    selection. FL_SELECTIONCLEAR is sent to the previous
    selection owner, if any.

    <i>Copying the buffer every time the selection is changed is
    obviously wasteful, especially for large selections.  An interface will
    probably be added in a future version to allow the selection to be made
    by a callback function.  The current interface will be emulated on top
    of this.</i>
*/
void Fl::selection_owner(Fl_Widget *owner) {selection_owner_ = owner;}

/**
  Changes the current selection.  The block of text is
  copied to an internal buffer by FLTK (be careful if doing this in
  response to an FL_PASTE as this \e may be the same buffer
  returned by event_text()).  The selection_owner()
  widget is set to the passed owner.
*/
void Fl::selection(Fl_Widget &owner, const char* text, int len) {
  selection_owner_ = &owner;
  Fl::copy(text, len, 0);
}

/** Backward compatibility only.
  This calls Fl::paste(receiver, 0);
  \see Fl::paste(Fl_Widget &receiver, int clipboard)
*/
void Fl::paste(Fl_Widget &receiver) {
  Fl::paste(receiver, 0);
}

////////////////////////////////////////////////////////////////

#include <FL/fl_draw.H>

void Fl_Widget::redraw() {
  damage(FL_DAMAGE_ALL);
}

void Fl_Widget::redraw_label() {
  if (window()) {
    if (box() == FL_NO_BOX) {
      // Widgets with the FL_NO_BOX boxtype need a parent to
      // redraw, since it is responsible for redrawing the
      // background...
      int X = x() > 0 ? x() - 1 : 0;
      int Y = y() > 0 ? y() - 1 : 0;
      window()->damage(FL_DAMAGE_ALL, X, Y, w() + 2, h() + 2);
    }

    if (align() && !(align() & FL_ALIGN_INSIDE) && window()->shown()) {
      // If the label is not inside the widget, compute the location of
      // the label and redraw the window within that bounding box...
      int W = 0, H = 0;
      label_.measure(W, H);
      W += 5; // Add a little to the size of the label to cover overflow
      H += 5;

      // FIXME:
      // This assumes that measure() returns the correct outline, which it does
      // not in all possible cases of alignment combinedwith image and symbols.
      switch (align() & 0x0f) {
        case FL_ALIGN_TOP_LEFT:
          window()->damage(FL_DAMAGE_EXPOSE, x(), y()-H, W, H); break;
        case FL_ALIGN_TOP:
          window()->damage(FL_DAMAGE_EXPOSE, x()+(w()-W)/2, y()-H, W, H); break;
        case FL_ALIGN_TOP_RIGHT:
          window()->damage(FL_DAMAGE_EXPOSE, x()+w()-W, y()-H, W, H); break;
        case FL_ALIGN_LEFT_TOP:
          window()->damage(FL_DAMAGE_EXPOSE, x()-W, y(), W, H); break;
        case FL_ALIGN_RIGHT_TOP:
          window()->damage(FL_DAMAGE_EXPOSE, x()+w(), y(), W, H); break;
        case FL_ALIGN_LEFT:
          window()->damage(FL_DAMAGE_EXPOSE, x()-W, y()+(h()-H)/2, W, H); break;
        case FL_ALIGN_RIGHT:
          window()->damage(FL_DAMAGE_EXPOSE, x()+w(), y()+(h()-H)/2, W, H); break;
        case FL_ALIGN_LEFT_BOTTOM:
          window()->damage(FL_DAMAGE_EXPOSE, x()-W, y()+h()-H, W, H); break;
        case FL_ALIGN_RIGHT_BOTTOM:
          window()->damage(FL_DAMAGE_EXPOSE, x()+w(), y()+h()-H, W, H); break;
        case FL_ALIGN_BOTTOM_LEFT:
          window()->damage(FL_DAMAGE_EXPOSE, x(), y()+h(), W, H); break;
        case FL_ALIGN_BOTTOM:
          window()->damage(FL_DAMAGE_EXPOSE, x()+(w()-W)/2, y()+h(), W, H); break;
        case FL_ALIGN_BOTTOM_RIGHT:
          window()->damage(FL_DAMAGE_EXPOSE, x()+w()-W, y()+h(), W, H); break;
        default:
          window()->damage(FL_DAMAGE_ALL); break;
      }
    } else {
      // The label is inside the widget, so just redraw the widget itself...
      damage(FL_DAMAGE_ALL);
    }
  }
}

void Fl_Widget::damage(uchar fl) {
  if (type() < FL_WINDOW) {
    // damage only the rectangle covered by a child widget:
    damage(fl, x(), y(), w(), h());
  } else {
    // damage entire window by deleting the region:
    Fl_X* i = Fl_X::i((Fl_Window*)this);
    if (!i) return; // window not mapped, so ignore it
    if (i->region) {XDestroyRegion(i->region); i->region = 0;}
    damage_ |= fl;
    Fl::damage(FL_DAMAGE_CHILD);
  }
}

void Fl_Widget::damage(uchar fl, int X, int Y, int W, int H) {
  Fl_Widget* wi = this;
  // mark all parent widgets between this and window with FL_DAMAGE_CHILD:
  while (wi->type() < FL_WINDOW) {
    wi->damage_ |= fl;
    wi = wi->parent();
    if (!wi) return;
    fl = FL_DAMAGE_CHILD;
  }
  Fl_X* i = Fl_X::i((Fl_Window*)wi);
  if (!i) return; // window not mapped, so ignore it

  // clip the damage to the window and quit if none:
  if (X < 0) {W += X; X = 0;}
  if (Y < 0) {H += Y; Y = 0;}
  if (W > wi->w()-X) W = wi->w()-X;
  if (H > wi->h()-Y) H = wi->h()-Y;
  if (W <= 0 || H <= 0) return;

  if (!X && !Y && W==wi->w() && H==wi->h()) {
    // if damage covers entire window delete region:
    wi->damage(fl);
    return;
  }

  if (wi->damage()) {
    // if we already have damage we must merge with existing region:
    if (i->region) {
#if defined(USE_X11)
      XRectangle R;
      R.x = X; R.y = Y; R.width = W; R.height = H;
      XUnionRectWithRegion(&R, i->region, i->region);
#elif defined(WIN32)
      Fl_Region R = XRectangleRegion(X, Y, W, H);
      CombineRgn(i->region, i->region, R, RGN_OR);
      XDestroyRegion(R);
#elif defined(__APPLE_QUARTZ__)
      CGRect arg = fl_cgrectmake_cocoa(X, Y, W, H);
      int j; // don't add a rectangle totally inside the Fl_Region
      for(j = 0; j < i->region->count; j++) {
        if(CGRectContainsRect(i->region->rects[j], arg)) break;
      }
      if( j >= i->region->count) {
        i->region->rects = (CGRect*)realloc(i->region->rects, (++(i->region->count)) * sizeof(CGRect));
        i->region->rects[i->region->count - 1] = arg;
      }
#else
# error unsupported platform
#endif
    }
    wi->damage_ |= fl;
  } else {
    // create a new region:
    if (i->region) XDestroyRegion(i->region);
    i->region = XRectangleRegion(X,Y,W,H);
    wi->damage_ = fl;
  }
  Fl::damage(FL_DAMAGE_CHILD);
}
void Fl_Window::flush() {
  make_current();
//if (damage() == FL_DAMAGE_EXPOSE && can_boxcheat(box())) fl_boxcheat = this;
  fl_clip_region(i->region); i->region = 0;
  draw();
}

#ifdef WIN32
#  include "Fl_win32.cxx"
//#elif defined(__APPLE__)
#endif

//
// The following methods allow callbacks to schedule the deletion of
// widgets at "safe" times.
//

static int		num_dwidgets = 0, alloc_dwidgets = 0;
static Fl_Widget	**dwidgets = 0;

/**
  Schedules a widget for deletion at the next call to the event loop.
  Use this method to delete a widget inside a callback function.

  To avoid early deletion of widgets, this function should be called
  toward the end of a callback and only after any call to the event
  loop (Fl::wait(), Fl::flush(), Fl::check(), fl_ask(), etc.).

  When deleting groups or windows, you must only delete the group or
  window widget and not the individual child widgets.

  \since FLTK 1.3 it is not necessary to remove widgets from their parent
  groups or windows before calling this, because it will be done in the
  widget's destructor, but it is not a failure to do this nevertheless.

  \note In FLTK 1.1 you \b must remove widgets from their parent group
  (or window) before deleting them.

  \see Fl_Widget::~Fl_Widget()
*/
void Fl::delete_widget(Fl_Widget *wi) {
  if (!wi) return;

  if (num_dwidgets >= alloc_dwidgets) {
    Fl_Widget	**temp;

    temp = new Fl_Widget *[alloc_dwidgets + 10];
    if (alloc_dwidgets) {
      memcpy(temp, dwidgets, alloc_dwidgets * sizeof(Fl_Widget *));
      delete[] dwidgets;
    }

    dwidgets = temp;
    alloc_dwidgets += 10;
  }

  dwidgets[num_dwidgets] = wi;
  num_dwidgets ++;
}

/**
    Deletes widgets previously scheduled for deletion.

    This is for internal use only. You should never call this directly.

    Fl::do_widget_deletion() is called from the FLTK event loop or whenever
    you call Fl::wait(). The previously scheduled widgets are deleted in the
    same order they were scheduled by calling Fl::delete_widget().

    \see Fl::delete_widget(Fl_Widget *wi)
*/
void Fl::do_widget_deletion() {
  if (!num_dwidgets) return;

  for (int i = 0; i < num_dwidgets; i ++)
    delete dwidgets[i];

  num_dwidgets = 0;
}

static Fl_Widget ***widget_watch = 0;
static int num_widget_watch = 0;
static int max_widget_watch = 0;

/**
  Adds a widget pointer to the widget watch list.

  \note Internal use only, please use class Fl_Widget_Tracker instead.

  This can be used, if it is possible that a widget might be deleted during
  a callback or similar function. The widget pointer must be added to the
  watch list before calling the callback. After the callback the widget
  pointer can be queried, if it is NULL. \e If it is NULL, then the widget has been
  deleted during the callback and must not be accessed anymore. If the widget
  pointer is \e not NULL, then the widget has not been deleted and can be accessed
  safely.

  After accessing the widget, the widget pointer must be released from the
  watch list by calling Fl::release_widget_pointer().

  Example for a button that is clicked (from its handle() method):
  \code
    Fl_Widget *wp = this;		// save 'this' in a pointer variable
    Fl::watch_widget_pointer(wp);	// add the pointer to the watch list
    set_changed();			// set the changed flag
    do_callback();			// call the callback
    if (!wp) {				// the widget has been deleted

      // DO NOT ACCESS THE DELETED WIDGET !

    } else {				// the widget still exists
      clear_changed();			// reset the changed flag
    }

    Fl::release_widget_pointer(wp);	// remove the pointer from the watch list
   \endcode

   This works, because all widgets call Fl::clear_widget_pointer() in their
   destructors.

   \see Fl::release_widget_pointer()
   \see Fl::clear_widget_pointer()

   An easier and more convenient method to control widget deletion during
   callbacks is to use the class Fl_Widget_Tracker with a local (automatic)
   variable.

   \see class Fl_Widget_Tracker
*/
void Fl::watch_widget_pointer(Fl_Widget *&w)
{
  Fl_Widget **wp = &w;
  int i;
  for (i=0; i<num_widget_watch; ++i) {
    if (widget_watch[i]==wp) return;
  }
  if (num_widget_watch==max_widget_watch) {
    max_widget_watch += 8;
    widget_watch = (Fl_Widget***)realloc(widget_watch, sizeof(Fl_Widget**)*max_widget_watch);
  }
  widget_watch[num_widget_watch++] = wp;
#ifdef DEBUG_WATCH
  printf ("\nwatch_widget_pointer:   (%d/%d) %8p => %8p\n",
    num_widget_watch,num_widget_watch,wp,*wp);
  fflush(stdout);
#endif // DEBUG_WATCH
}

/**
  Releases a widget pointer from the watch list.

  This is used to remove a widget pointer that has been added to the watch list
  with Fl::watch_widget_pointer(), when it is not needed anymore.

  \note Internal use only, please use class Fl_Widget_Tracker instead.

  \see Fl::watch_widget_pointer()
*/
void Fl::release_widget_pointer(Fl_Widget *&w)
{
  Fl_Widget **wp = &w;
  int i,j=0;
  for (i=0; i<num_widget_watch; ++i) {
    if (widget_watch[i]!=wp) {
      if (j<i) widget_watch[j] = widget_watch[i]; // fill gap
      j++;
    }
#ifdef DEBUG_WATCH
    else { // found widget pointer
      printf ("release_widget_pointer: (%d/%d) %8p => %8p\n",
	i+1,num_widget_watch,wp,*wp);
    }
#endif //DEBUG_WATCH
  }
  num_widget_watch = j;
#ifdef DEBUG_WATCH
  printf ("                        num_widget_watch = %d\n\n",num_widget_watch);
  fflush(stdout);
#endif // DEBUG_WATCH
  return;
}
/**
  Clears a widget pointer \e in the watch list.

  This is called when a widget is destroyed (by its destructor). You should never
  call this directly.

  \note Internal use only !

  This method searches the widget watch list for pointers to the widget and
  clears each pointer that points to it. Widget pointers can be added to the
  widget watch list by calling Fl::watch_widget_pointer() or by using the
  helper class Fl_Widget_Tracker (recommended).

  \see Fl::watch_widget_pointer()
  \see class Fl_Widget_Tracker
*/
void Fl::clear_widget_pointer(Fl_Widget const *w)
{
  if (w==0L) return;
  int i;
  for (i=0; i<num_widget_watch; ++i) {
    if (widget_watch[i] && *widget_watch[i]==w) {
      *widget_watch[i] = 0L;
    }
  }
}


/**
 \brief FLTK library options management.

 This function needs to be documented in more detail. It can be used for more
 optional settings, such as using a native file chooser instead of the FLTK one
 wherever possible, disabling tooltips, disabling visible focus, disabling
 FLTK file chooser preview, etc. .

 There should be a command line option interface.

 There should be an application that manages options system wide, per user, and
 per application.

 \note As of FLTK 1.3.0, options can be managed within fluid, using the menu
 <i>Edit/Global FLTK Settings</i>.

 \param opt which option
 \return true or false
 \see enum Fl::Fl_Option
 \see Fl::option(Fl_Option, bool)

 \since FLTK 1.3.0
 */
bool Fl::option(Fl_Option opt)
{
  if (!options_read_) {
    int tmp;
    { // first, read the system wide preferences
      Fl_Preferences prefs(Fl_Preferences::SYSTEM, "fltk.org", "fltk");
      Fl_Preferences opt_prefs(prefs, "options");
      opt_prefs.get("ArrowFocus", tmp, 0);                      // default: off
      options_[OPTION_ARROW_FOCUS] = tmp;
      //opt_prefs.get("NativeFilechooser", tmp, 1);             // default: on
      //options_[OPTION_NATIVE_FILECHOOSER] = tmp;
      //opt_prefs.get("FilechooserPreview", tmp, 1);            // default: on
      //options_[OPTION_FILECHOOSER_PREVIEW] = tmp;
      opt_prefs.get("VisibleFocus", tmp, 1);                    // default: on
      options_[OPTION_VISIBLE_FOCUS] = tmp;
      opt_prefs.get("DNDText", tmp, 1);                         // default: on
      options_[OPTION_DND_TEXT] = tmp;
      opt_prefs.get("ShowTooltips", tmp, 1);                    // default: on
      options_[OPTION_SHOW_TOOLTIPS] = tmp;
    }
    { // next, check the user preferences
      // override system options only, if the option is set ( >= 0 )
      Fl_Preferences prefs(Fl_Preferences::USER, "fltk.org", "fltk");
      Fl_Preferences opt_prefs(prefs, "options");
      opt_prefs.get("ArrowFocus", tmp, -1);
      if (tmp >= 0) options_[OPTION_ARROW_FOCUS] = tmp;
      //opt_prefs.get("NativeFilechooser", tmp, -1);
      //if (tmp >= 0) options_[OPTION_NATIVE_FILECHOOSER] = tmp;
      //opt_prefs.get("FilechooserPreview", tmp, -1);
      //if (tmp >= 0) options_[OPTION_FILECHOOSER_PREVIEW] = tmp;
      opt_prefs.get("VisibleFocus", tmp, -1);
      if (tmp >= 0) options_[OPTION_VISIBLE_FOCUS] = tmp;
      opt_prefs.get("DNDText", tmp, -1);
      if (tmp >= 0) options_[OPTION_DND_TEXT] = tmp;
      opt_prefs.get("ShowTooltips", tmp, -1);
      if (tmp >= 0) options_[OPTION_SHOW_TOOLTIPS] = tmp;
    }
    { // now, if the developer has registered this app, we could as for per-application preferences
    }
    options_read_ = 1;
  }
  if (opt<0 || opt>=OPTION_LAST)
    return false;
  return (bool)(options_[opt]!=0);
}

/**
 \brief Override an option while the application is running.

 This function does not change any system or user settings.

 \param opt which option
 \param val set to true or false
 \see enum Fl::Fl_Option
 \see bool Fl::option(Fl_Option)
 */
void Fl::option(Fl_Option opt, bool val)
{
  if (opt<0 || opt>=OPTION_LAST)
    return;
  if (!options_read_) {
    // first read this option, so we don't override our setting later
    option(opt);
  }
  options_[opt] = val;
}


// Helper class Fl_Widget_Tracker

/**
  The constructor adds a widget to the watch list.
*/
Fl_Widget_Tracker::Fl_Widget_Tracker(Fl_Widget *wi)
{
  wp_ = wi;
  Fl::watch_widget_pointer(wp_); // add pointer to watch list
}

/**
  The destructor removes a widget from the watch list.
*/
Fl_Widget_Tracker::~Fl_Widget_Tracker()
{
  Fl::release_widget_pointer(wp_); // remove pointer from watch list
}


//
// End of "$Id: Fl.cxx 8723 2011-05-23 16:49:02Z manolo $".
//
