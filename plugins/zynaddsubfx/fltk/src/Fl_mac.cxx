//
// "$Id: Fl_mac.cxx 6033 2008-02-20 18:17:34Z matt $"
//
// MacOS specific code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2007 by Bill Spitzak and others.
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

//// From the inner edge of a MetroWerks CodeWarrior CD:
// (without permission)
//
// "Three Compiles for 68Ks under the sky,
// Seven Compiles for PPCs in their fragments of code,
// Nine Compiles for Mortal Carbon doomed to die,
// One Compile for Mach-O Cocoa on its Mach-O throne,
// in the Land of MacOS X where the Drop-Shadows lie.
// 
// One Compile to link them all, One Compile to merge them,
// One Compile to copy them all and in the bundle bind them,
// in the Land of MacOS X where the Drop-Shadows lie."

// warning: the Apple Quartz version still uses some Quickdraw calls,
//          mostly to get around the single active context in QD and 
//          to implement clipping. This should be changed into pure
//          Quartz calls in the near future.

// we don't need the following definition because we deliver only
// true mouse moves.  On very slow systems however, this flag may
// still be useful.
#define CONSOLIDATE_MOTION 0
extern "C" {
#include <pthread.h>
}

#include <config.h>
#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Fl_Sys_Menu_Bar.H>
#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <unistd.h>

// #define DEBUG_SELECT		// UNCOMMENT FOR SELECT()/THREAD DEBUGGING
#ifdef DEBUG_SELECT
#include <stdio.h>		// testing
#define DEBUGMSG(msg)		if ( msg ) fprintf(stderr, msg);
#define DEBUGPERRORMSG(msg)	if ( msg ) perror(msg)
#define DEBUGTEXT(txt)		txt
#else
#define DEBUGMSG(msg)
#define DEBUGPERRORMSG(msg)
#define DEBUGTEXT(txt)		NULL
#endif /*DEBUG_SELECT*/

// external functions
extern Fl_Window* fl_find(Window);
extern void fl_fix_focus();

// forward definition of functions in this file
static void handleUpdateEvent( WindowPtr xid );
//+ int fl_handle(const EventRecord &event);
static int FSSpec2UnixPath( FSSpec *fs, char *dst );

// public variables
int fl_screen;
CGContextRef fl_gc = 0;
Handle fl_system_menu;
Fl_Sys_Menu_Bar *fl_sys_menu_bar = 0;
CursHandle fl_default_cursor;
WindowRef fl_capture = 0;            // we need this to compensate for a missing(?) mouse capture
ulong fl_event_time;                 // the last timestamp from an x event
char fl_key_vector[32];              // used by Fl::get_key()
bool fl_show_iconic;                 // true if called from iconize() - shows the next created window in collapsed state
int fl_disable_transient_for;        // secret method of removing TRANSIENT_FOR
const Fl_Window* fl_modal_for;       // parent of modal() window
Fl_Region fl_window_region = 0;
Window fl_window;
Fl_Window *Fl_Window::current_;
EventRef fl_os_event;		// last (mouse) event

// forward declarations of variables in this file
static int got_events = 0;
static Fl_Window* resize_from_system;
static CursPtr default_cursor_ptr;
static Cursor default_cursor;
static WindowRef fl_os_capture = 0; // the dispatch handler will redirect mose move and drag events to these windows

#if CONSOLIDATE_MOTION
static Fl_Window* send_motion;
extern Fl_Window* fl_xmousewin;
#endif

enum { kEventClassFLTK = 'fltk' };
enum { kEventFLTKBreakLoop = 1, kEventFLTKDataReady };

/**
* Mac keyboard lookup table
 */
static unsigned short macKeyLookUp[128] =
{
    'a', 's', 'd', 'f', 'h', 'g', 'z', 'x',
    'c', 'v', '^', 'b', 'q', 'w', 'e', 'r',

    'y', 't', '1', '2', '3', '4', '6', '5',
    '=', '9', '7', '-', '8', '0', ']', 'o',

    'u', '[', 'i', 'p', FL_Enter, 'l', 'j', '\'',
    'k', ';', '\\', ',', '/', 'n', 'm', '.',

    FL_Tab, ' ', '`', FL_BackSpace, 
    FL_KP_Enter, FL_Escape, 0, 0/*FL_Meta_L*/,
    0/*FL_Shift_L*/, 0/*FL_Caps_Lock*/, 0/*FL_Alt_L*/, 0/*FL_Control_L*/, 
    0/*FL_Shift_R*/, 0/*FL_Alt_R*/, 0/*FL_Control_R*/, 0,

    0, FL_KP+'.', FL_Right, FL_KP+'*', 0, FL_KP+'+', FL_Left, FL_Num_Lock,
    FL_Down, 0, 0, FL_KP+'/', FL_KP_Enter, FL_Up, FL_KP+'-', 0,

    0, FL_KP+'=', FL_KP+'0', FL_KP+'1', FL_KP+'2', FL_KP+'3', FL_KP+'4', FL_KP+'5',
    FL_KP+'6', FL_KP+'7', 0, FL_KP+'8', FL_KP+'9', 0, 0, 0,

    FL_F+5, FL_F+6, FL_F+7, FL_F+3, FL_F+8, FL_F+9, 0, FL_F+11,
    0, 0/*FL_F+13*/, FL_Print, FL_Scroll_Lock, 0, FL_F+10, FL_Menu, FL_F+12,

    0, FL_Pause, FL_Help, FL_Home, FL_Page_Up, FL_Delete, FL_F+4, FL_End,
    FL_F+2, FL_Page_Down, FL_F+1, FL_Left, FL_Right, FL_Down, FL_Up, 0/*FL_Power*/,
};

/**
 * convert the current mouse chord into the FLTK modifier state
 */
static void mods_to_e_state( UInt32 mods )
{
  long state = 0;
  if ( mods & kEventKeyModifierNumLockMask ) state |= FL_NUM_LOCK;
  if ( mods & cmdKey ) state |= FL_META;
  if ( mods & (optionKey|rightOptionKey) ) state |= FL_ALT;
  if ( mods & (controlKey|rightControlKey) ) state |= FL_CTRL;
  if ( mods & (shiftKey|rightShiftKey) ) state |= FL_SHIFT;
  if ( mods & alphaLock ) state |= FL_CAPS_LOCK;
  Fl::e_state = ( Fl::e_state & 0xff000000 ) | state;
  //printf( "State 0x%08x (%04x)\n", Fl::e_state, mods );
}


/**
 * convert the current mouse chord into the FLTK keysym
 */
static void mods_to_e_keysym( UInt32 mods )
{
  if ( mods & cmdKey ) Fl::e_keysym = FL_Meta_L;
  else if ( mods & kEventKeyModifierNumLockMask ) Fl::e_keysym = FL_Num_Lock;
  else if ( mods & optionKey ) Fl::e_keysym = FL_Alt_L;
  else if ( mods & rightOptionKey ) Fl::e_keysym = FL_Alt_R;
  else if ( mods & controlKey ) Fl::e_keysym = FL_Control_L;
  else if ( mods & rightControlKey ) Fl::e_keysym = FL_Control_R;
  else if ( mods & shiftKey ) Fl::e_keysym = FL_Shift_L;
  else if ( mods & rightShiftKey ) Fl::e_keysym = FL_Shift_R;
  else if ( mods & alphaLock ) Fl::e_keysym = FL_Caps_Lock;
  else Fl::e_keysym = 0;
  //printf( "to sym 0x%08x (%04x)\n", Fl::e_keysym, mods );
}
// these pointers are set by the Fl::lock() function:
static void nothing() {}
void (*fl_lock_function)() = nothing;
void (*fl_unlock_function)() = nothing;

//
// Select interface -- how it's implemented:
//     When the user app configures one or more file descriptors to monitor
//     with Fl::add_fd(), we start a separate thread to select() the  data,
//     sending a custom OSX 'FLTK data ready event' to the parent  thread's
//     RunApplicationLoop(), so that it triggers the data  ready  callbacks
//     in the parent thread.                               -erco 04/04/04
//     
#define POLLIN  1
#define POLLOUT 4
#define POLLERR 8

// Class to handle select() 'data ready'
class DataReady
{
    struct FD
    {
      int fd;
      short events;
      void (*cb)(int, void*);
      void* arg;
    };
    int nfds, fd_array_size;
    FD *fds;
    pthread_t tid;		// select()'s thread id

    // Data that needs to be locked (all start with '_')
    pthread_mutex_t _datalock;	// data lock
    fd_set _fdsets[3];		// r/w/x sets user wants to monitor
    int _maxfd;			// max fd count to monitor
    int _cancelpipe[2];		// pipe used to help cancel thread
    void *_userdata;		// thread's userdata

public:
    DataReady()
    {
      nfds = 0;
      fd_array_size = 0;
      fds = 0;
      tid = 0;

      pthread_mutex_init(&_datalock, NULL);
      FD_ZERO(&_fdsets[0]); FD_ZERO(&_fdsets[1]); FD_ZERO(&_fdsets[2]);
      _cancelpipe[0] = _cancelpipe[1] = 0;
      _userdata = 0;
      _maxfd = 0;
    }

    ~DataReady()
    {
        CancelThread(DEBUGTEXT("DESTRUCTOR\n"));
        if (fds) { free(fds); fds = 0; }
	nfds = 0;
    }

    // Locks
    //    The convention for locks: volatile vars start with '_',
    //    and must be locked before use. Locked code is prefixed 
    //    with /*LOCK*/ to make painfully obvious esp. in debuggers. -erco
    //
    void DataLock() { pthread_mutex_lock(&_datalock); }
    void DataUnlock() { pthread_mutex_unlock(&_datalock); }

    // Accessors
    int IsThreadRunning() { return(tid ? 1 : 0); }
    int GetNfds() { return(nfds); }
    int GetCancelPipe(int ix) { return(_cancelpipe[ix]); }
    fd_set GetFdset(int ix) { return(_fdsets[ix]); }

    // Methods
    void AddFD(int n, int events, void (*cb)(int, void*), void *v);
    void RemoveFD(int n, int events);
    int CheckData(fd_set& r, fd_set& w, fd_set& x);
    void HandleData(fd_set& r, fd_set& w, fd_set& x);
    static void* DataReadyThread(void *self);
    void StartThread(void *userdata);
    void CancelThread(const char *reason);
};

static DataReady dataready;

void DataReady::AddFD(int n, int events, void (*cb)(int, void*), void *v)
{
  RemoveFD(n, events);
  int i = nfds++;
  if (i >= fd_array_size) 
  {
    FD *temp;
    fd_array_size = 2*fd_array_size+1;
    if (!fds) { temp = (FD*)malloc(fd_array_size*sizeof(FD)); }
    else { temp = (FD*)realloc(fds, fd_array_size*sizeof(FD)); }
    if (!temp) return;
    fds = temp;
  }
  fds[i].cb  = cb;
  fds[i].arg = v;
  fds[i].fd  = n;
  fds[i].events = events;
  DataLock();
  /*LOCK*/  if (events & POLLIN)  FD_SET(n, &_fdsets[0]);
  /*LOCK*/  if (events & POLLOUT) FD_SET(n, &_fdsets[1]);
  /*LOCK*/  if (events & POLLERR) FD_SET(n, &_fdsets[2]);
  /*LOCK*/  if (n > _maxfd) _maxfd = n;
  DataUnlock();
}

// Remove an FD from the array
void DataReady::RemoveFD(int n, int events)
{
  int i,j;
  for (i=j=0; i<nfds; i++)
  {
    if (fds[i].fd == n) 
    {
      int e = fds[i].events & ~events;
      if (!e) continue; // if no events left, delete this fd
      fds[i].events = e;
    }
    // move it down in the array if necessary:
    if (j<i)
      { fds[j] = fds[i]; }
    j++;
  }
  nfds = j;
  DataLock();
  /*LOCK*/  if (events & POLLIN)  FD_CLR(n, &_fdsets[0]);
  /*LOCK*/  if (events & POLLOUT) FD_CLR(n, &_fdsets[1]);
  /*LOCK*/  if (events & POLLERR) FD_CLR(n, &_fdsets[2]);
  /*LOCK*/  if (n == _maxfd) _maxfd--;
  DataUnlock();
}

// CHECK IF USER DATA READY, RETURNS r/w/x INDICATING WHICH IF ANY
int DataReady::CheckData(fd_set& r, fd_set& w, fd_set& x)
{
  int ret;
  DataLock();
  /*LOCK*/  timeval t = { 0, 1 };		// quick check
  /*LOCK*/  r = _fdsets[0], w = _fdsets[1], x = _fdsets[2];
  /*LOCK*/  ret = ::select(_maxfd+1, &r, &w, &x, &t);
  DataUnlock();
  if ( ret == -1 )
    { DEBUGPERRORMSG("CheckData(): select()"); }
  return(ret);
}

// HANDLE DATA READY CALLBACKS
void DataReady::HandleData(fd_set& r, fd_set& w, fd_set& x)
{
  for (int i=0; i<nfds; i++) 
  {
    int f = fds[i].fd;
    short revents = 0;
    if (FD_ISSET(f, &r)) revents |= POLLIN;
    if (FD_ISSET(f, &w)) revents |= POLLOUT;
    if (FD_ISSET(f, &x)) revents |= POLLERR;
    if (fds[i].events & revents) 
    {
      DEBUGMSG("DOING CALLBACK: ");
      fds[i].cb(f, fds[i].arg);
      DEBUGMSG("DONE\n");
    }
  }
}

// DATA READY THREAD
//    This thread watches for changes in user's file descriptors.
//    Sends a 'data ready event' to the main thread if any change.
//
void* DataReady::DataReadyThread(void *o)
{
  DataReady *self = (DataReady*)o;
  while ( 1 )					// loop until thread cancel or error
  {
    // Thread safe local copies of data before each select()
    self->DataLock();
    /*LOCK*/  int maxfd = self->_maxfd;
    /*LOCK*/  fd_set r = self->GetFdset(0);
    /*LOCK*/  fd_set w = self->GetFdset(1);
    /*LOCK*/  fd_set x = self->GetFdset(2);
    /*LOCK*/  void *userdata = self->_userdata;
    /*LOCK*/  int cancelpipe = self->GetCancelPipe(0);
    /*LOCK*/  if ( cancelpipe > maxfd ) maxfd = cancelpipe;
    /*LOCK*/  FD_SET(cancelpipe, &r);		// add cancelpipe to fd's to watch
    /*LOCK*/  FD_SET(cancelpipe, &x);
    self->DataUnlock();
    // timeval t = { 1000, 0 };	// 1000 seconds;
    timeval t = { 2, 0 };	// HACK: 2 secs prevents 'hanging' problem
    int ret = ::select(maxfd+1, &r, &w, &x, &t);
    pthread_testcancel();	// OSX 10.0.4 and older: needed for parent to cancel
    switch ( ret )
    {
      case 0:	// NO DATA
        continue;
      case -1:	// ERROR
      {
        DEBUGPERRORMSG("CHILD THREAD: select() failed");
        return(NULL);		// error? exit thread
      }
      default:	// DATA READY
      {
	if (FD_ISSET(cancelpipe, &r) || FD_ISSET(cancelpipe, &x)) 	// cancel?
	    { return(NULL); }						// just exit
        DEBUGMSG("CHILD THREAD: DATA IS READY\n");
        EventRef drEvent;
        CreateEvent( 0, kEventClassFLTK, kEventFLTKDataReady,
		     0, kEventAttributeUserEvent, &drEvent);
        EventQueueRef eventqueue = (EventQueueRef)userdata;
        PostEventToQueue(eventqueue, drEvent, kEventPriorityStandard );
        ReleaseEvent( drEvent );
        return(NULL);		// done with thread
      }
    }
  }
}

// START 'DATA READY' THREAD RUNNING, CREATE INTER-THREAD PIPE
void DataReady::StartThread(void *new_userdata)
{
  CancelThread(DEBUGTEXT("STARTING NEW THREAD\n"));
  DataLock();
  /*LOCK*/  pipe(_cancelpipe);	// pipe for sending cancel msg to thread
  /*LOCK*/  _userdata = new_userdata;
  DataUnlock();
  DEBUGMSG("*** START THREAD\n");
  pthread_create(&tid, NULL, DataReadyThread, (void*)this);
}

// CANCEL 'DATA READY' THREAD, CLOSE PIPE
void DataReady::CancelThread(const char *reason)
{
  if ( tid )
  {
    DEBUGMSG("*** CANCEL THREAD: ");
    DEBUGMSG(reason);
    if ( pthread_cancel(tid) == 0 )		// cancel first
    {
      DataLock();
      /*LOCK*/  write(_cancelpipe[1], "x", 1);	// wake thread from select
      DataUnlock();
      pthread_join(tid, NULL);			// wait for thread to finish
    }
    tid = 0;
    DEBUGMSG("(JOINED) OK\n");
  }
  // Close pipe if open
  DataLock();
  /*LOCK*/  if ( _cancelpipe[0] ) { close(_cancelpipe[0]); _cancelpipe[0] = 0; }
  /*LOCK*/  if ( _cancelpipe[1] ) { close(_cancelpipe[1]); _cancelpipe[1] = 0; }
  DataUnlock();
}

void Fl::add_fd( int n, int events, void (*cb)(int, void*), void *v )
    { dataready.AddFD(n, events, cb, v); }

void Fl::add_fd(int fd, void (*cb)(int, void*), void* v)
    { dataready.AddFD(fd, POLLIN, cb, v); }

void Fl::remove_fd(int n, int events)
    { dataready.RemoveFD(n, events); }

void Fl::remove_fd(int n)
    { dataready.RemoveFD(n, -1); }

/**
 * Check if there is actually a message pending!
 */
int fl_ready()
{
  EventRef event;
  return !ReceiveNextEvent(0, NULL, 0.0, false, &event);
}

/**
 * handle Apple Menu items (can be created using the Fl_Sys_Menu_Bar
 * returns eventNotHandledErr if the menu item could not be handled
 */
OSStatus HandleMenu( HICommand *cmd )
{
  OSStatus ret = eventNotHandledErr;
  // attributes, commandIDm menu.menuRef, menu.menuItemIndex
  UInt32 ref;
  OSErr rrc = GetMenuItemRefCon( cmd->menu.menuRef, cmd->menu.menuItemIndex, &ref );
  //printf( "%d, %08x, %08x, %d, %d, %8x\n", rrc, cmd->attributes, cmd->commandID, cmd->menu.menuRef, cmd->menu.menuItemIndex, rrc );
  if ( rrc==noErr && ref )
  {
    Fl_Menu_Item *m = (Fl_Menu_Item*)ref;
    //printf( "Menu: %s\n", m->label() );
    fl_sys_menu_bar->picked( m );
    if ( m->flags & FL_MENU_TOGGLE ) // update the menu toggle symbol
      SetItemMark( cmd->menu.menuRef, cmd->menu.menuItemIndex, (m->flags & FL_MENU_VALUE ) ? 0x12 : 0 );
    if ( m->flags & FL_MENU_RADIO ) // update all radio buttons in this menu
    {
      Fl_Menu_Item *j = m;
      int i = cmd->menu.menuItemIndex;
      for (;;)
      {
        if ( j->flags & FL_MENU_DIVIDER )
          break;
        j++; i++;
        if ( !j->text || !j->radio() )
          break;
        SetItemMark( cmd->menu.menuRef, i, ( j->flags & FL_MENU_VALUE ) ? 0x13 : 0 );
      }
      j = m-1; i = cmd->menu.menuItemIndex-1;
      for ( ; i>0; j--, i-- )
      {
        if ( !j->text || j->flags&FL_MENU_DIVIDER || !j->radio() )
          break;
        SetItemMark( cmd->menu.menuRef, i, ( j->flags & FL_MENU_VALUE ) ? 0x13 : 0 );
      }
      SetItemMark( cmd->menu.menuRef, cmd->menu.menuItemIndex, ( m->flags & FL_MENU_VALUE ) ? 0x13 : 0 );
    }
    ret = noErr; // done handling this event
  }
  HiliteMenu(0);
  return ret;
}


/**
 * We can make every event pass through this function
 * - mouse events need to be manipulated to use a mouse focus window
 * - keyboard, mouse and some window  events need to quit the Apple Event Loop
 *   so FLTK can continue its own management
 */
static pascal OSStatus carbonDispatchHandler( EventHandlerCallRef nextHandler, EventRef event, void *userData )
{
  OSStatus ret = eventNotHandledErr;
  HICommand cmd;

  fl_lock_function();

  got_events = 1;

  switch ( GetEventClass( event ) )
  {
  case kEventClassMouse:
    switch ( GetEventKind( event ) )
    {
    case kEventMouseUp:
    case kEventMouseMoved:
    case kEventMouseDragged:
      if ( fl_capture )
        ret = SendEventToEventTarget( event, GetWindowEventTarget( fl_capture ) );
      else if ( fl_os_capture ){
        ret = SendEventToEventTarget( event, GetWindowEventTarget( fl_os_capture ) );
	fl_os_capture = 0;
      }
      break;
    }
    break;
  case kEventClassCommand:
    switch (GetEventKind( event ) )
    {
      case kEventCommandProcess:
        GetEventParameter( event, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &cmd );
        ret = HandleMenu( &cmd );
        break;
    }
    break;
  case kEventClassFLTK:
    switch ( GetEventKind( event ) )
    {
    case kEventFLTKBreakLoop:
      ret = noErr;
      break;
    case kEventFLTKDataReady:
      {
	dataready.CancelThread(DEBUGTEXT("DATA READY EVENT\n"));

        // CHILD THREAD TELLS US DATA READY
	//     Check to see what's ready, and invoke user's cb's
	//
	fd_set r,w,x;
	switch(dataready.CheckData(r,w,x))
	{
	  case 0:	// NO DATA
	    break;
	  case -1:	// ERROR
	    break;
	  default:	// DATA READY
	    dataready.HandleData(r,w,x);
	    break;
        }
      }
      ret = noErr;
      break;
    }
  }
  if ( ret == eventNotHandledErr )
    ret = CallNextEventHandler( nextHandler, event ); // let the OS handle the activation, but continue to get a click-through effect

  fl_unlock_function();

  return ret;
}


/**
 * break the current event loop
 */
static void breakMacEventLoop()
{
  EventRef breakEvent;

  fl_lock_function();

  CreateEvent( 0, kEventClassFLTK, kEventFLTKBreakLoop, 0, kEventAttributeUserEvent, &breakEvent );
  PostEventToQueue( GetCurrentEventQueue(), breakEvent, kEventPriorityStandard );
  ReleaseEvent( breakEvent );

  fl_unlock_function();
}

//
// MacOS X timers
//

struct MacTimeout {
    Fl_Timeout_Handler callback;
    void* data;
    EventLoopTimerRef timer;
    EventLoopTimerUPP upp;
    char pending; 
};
static MacTimeout* mac_timers;
static int mac_timer_alloc;
static int mac_timer_used;


static void realloc_timers()
{
    if (mac_timer_alloc == 0) {
        mac_timer_alloc = 8;
    }
    mac_timer_alloc *= 2;
    MacTimeout* new_timers = new MacTimeout[mac_timer_alloc];
    memset(new_timers, 0, sizeof(MacTimeout)*mac_timer_alloc);
    memcpy(new_timers, mac_timers, sizeof(MacTimeout) * mac_timer_used);
    MacTimeout* delete_me = mac_timers;
    mac_timers = new_timers;
    delete [] delete_me;
}

static void delete_timer(MacTimeout& t)
{
    if (t.timer) {
        RemoveEventLoopTimer(t.timer);
        DisposeEventLoopTimerUPP(t.upp);
        memset(&t, 0, sizeof(MacTimeout));
    }
}


static pascal void do_timer(EventLoopTimerRef timer, void* data)
{
   for (int i = 0;  i < mac_timer_used;  ++i) {
        MacTimeout& t = mac_timers[i];
        if (t.timer == timer  &&  t.data == data) {
            t.pending = 0;
            (*t.callback)(data);
            if (t.pending==0)
              delete_timer(t);
            break;
        }
    }
    breakMacEventLoop();
}

/**
 * This function is the central event handler.
 * It reads events from the event queue using the given maximum time
 * Funny enough, it returns the same time that it got as the argument. 
 */
static double do_queued_events( double time = 0.0 ) 
{
  static bool been_here = 0;
  static RgnHandle rgn;
  
  // initialize events and a region that enables mouse move events
  if (!been_here) {
    rgn = NewRgn();
    Point mp;
    GetMouse(&mp);
    SetRectRgn(rgn, mp.h, mp.v, mp.h, mp.v);
    SetEventMask(everyEvent);
    been_here = 1;
  }
  OSStatus ret;
  static EventTargetRef target = 0;
  if ( !target ) 
  {
    target = GetEventDispatcherTarget();

    EventHandlerUPP dispatchHandler = NewEventHandlerUPP( carbonDispatchHandler ); // will not be disposed by Carbon...
    static EventTypeSpec dispatchEvents[] = {
        { kEventClassWindow, kEventWindowShown },
        { kEventClassWindow, kEventWindowHidden },
        { kEventClassWindow, kEventWindowActivated },
        { kEventClassWindow, kEventWindowDeactivated },
        { kEventClassWindow, kEventWindowClose },
        { kEventClassKeyboard, kEventRawKeyDown },
        { kEventClassKeyboard, kEventRawKeyRepeat },
        { kEventClassKeyboard, kEventRawKeyUp },
        { kEventClassKeyboard, kEventRawKeyModifiersChanged },
        { kEventClassMouse, kEventMouseDown },
        { kEventClassMouse, kEventMouseUp },
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, 11 }, // MightyMouse wheels
        { kEventClassMouse, kEventMouseWheelMoved },
        { kEventClassMouse, kEventMouseDragged },
        { kEventClassFLTK, kEventFLTKBreakLoop },
        { kEventClassFLTK, kEventFLTKDataReady } };
    ret = InstallEventHandler( target, dispatchHandler, GetEventTypeCount(dispatchEvents), dispatchEvents, 0, 0L );
    static EventTypeSpec appEvents[] = {
        { kEventClassCommand, kEventCommandProcess } };
    ret = InstallApplicationEventHandler( dispatchHandler, GetEventTypeCount(appEvents), appEvents, 0, 0L );
  }

  got_events = 0;

  // Check for re-entrant condition
  if ( dataready.IsThreadRunning() )
    { dataready.CancelThread(DEBUGTEXT("AVOID REENTRY\n")); }

  // Start thread to watch for data ready
  if ( dataready.GetNfds() )
      { dataready.StartThread((void*)GetCurrentEventQueue()); }

  fl_unlock_function();

  EventRef event;
  EventTimeout timeout = time;
  if (!ReceiveNextEvent(0, NULL, timeout, true, &event)) {
    got_events = 1;
    OSErr ret = SendEventToEventTarget( event, target );
    if (ret!=noErr) {
      EventRecord clevent;
      ConvertEventRefToEventRecord(event, &clevent);
      if (clevent.what==kHighLevelEvent) {
        ret = AEProcessAppleEvent(&clevent);
      }
    }
    if (   ret==eventNotHandledErr
        && GetEventClass(event)==kEventClassMouse
        && GetEventKind(event)==kEventMouseDown ) {
      WindowRef win; Point pos;
      GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
        NULL, sizeof(pos), NULL, &pos);
      if (MacFindWindow(pos, &win)==inMenuBar) {
        MenuSelect(pos);
      }
    }
    ReleaseEvent( event );
  }

  fl_lock_function();

#if CONSOLIDATE_MOTION
  if (send_motion && send_motion == fl_xmousewin) {
    send_motion = 0;
    Fl::handle(FL_MOVE, fl_xmousewin);
  }
#endif

  return time;
}


/**
 * This public function handles all events. It wait a maximum of 
 * 'time' secods for an event. This version returns 1 if events
 * other than the timeout timer were processed.
 *
 * \todo there is no socket handling in this code whatsoever
 */
int fl_wait( double time ) 
{
  do_queued_events( time );
  return (got_events);
}


/**
 * event handler for Apple-Q key combination
 * this is also called from the Carbon Window handler after all windows were closed
 */
static OSErr QuitAppleEventHandler( const AppleEvent *appleEvt, AppleEvent* reply, UInt32 refcon )
{
  fl_lock_function();

  while ( Fl_X::first ) {
    Fl_X *x = Fl_X::first;
    Fl::handle( FL_CLOSE, x->w );
    if ( Fl_X::first == x ) {
      fl_unlock_function();
      return noErr; // FLTK has not close all windows, so we return to the main program now
    }
  }

  fl_unlock_function();

  return noErr;
}


/**
 * Carbon Window handler
 * This needs to be linked into all new window event handlers
 */
static pascal OSStatus carbonWindowHandler( EventHandlerCallRef nextHandler, EventRef event, void *userData )
{
  UInt32 kind = GetEventKind( event );
  OSStatus ret = eventNotHandledErr;
  Fl_Window *window = (Fl_Window*)userData;
  Fl::first_window(window);

  Rect currentBounds, originalBounds;
  WindowClass winClass;
  static Fl_Window *activeWindow = 0;
  
  fl_lock_function();
  
  switch ( kind )
  {
  case kEventWindowBoundsChanging:
    GetEventParameter( event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &currentBounds );
    GetEventParameter( event, kEventParamOriginalBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &originalBounds );
    break;
  case kEventWindowDrawContent:
    handleUpdateEvent( fl_xid( window ) );
    ret = noErr;
    break;
  case kEventWindowBoundsChanged: {
    GetEventParameter( event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &currentBounds );
    GetEventParameter( event, kEventParamOriginalBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &originalBounds );
    int X = currentBounds.left, W = currentBounds.right-X;
    int Y = currentBounds.top, H = currentBounds.bottom-Y;
    resize_from_system = window;
    window->resize( X, Y, W, H );
    if ( ( originalBounds.right - originalBounds.left != W ) 
      || ( originalBounds.bottom - originalBounds.top != H ) )
    {
      if ( window->shown() ) 
        handleUpdateEvent( fl_xid( window ) );
    } 
    break; }
  case kEventWindowShown:
    if ( !window->parent() )
    {
      GetWindowClass( fl_xid( window ), &winClass );
      if ( winClass != kHelpWindowClass ) {	// help windows can't get the focus!
        Fl::handle( FL_FOCUS, window);
        activeWindow = window;
      }
      Fl::handle( FL_SHOW, window);
      mods_to_e_state(GetCurrentKeyModifiers());
    }
    break;
  case kEventWindowHidden:
    if ( !window->parent() ) Fl::handle( FL_HIDE, window);
    break;
  case kEventWindowActivated:
    if ( window->shown() && window!=activeWindow )
    {
      GetWindowClass( fl_xid( window ), &winClass );
      if ( winClass != kHelpWindowClass ) {	// help windows can't get the focus!
        Fl::handle( FL_FOCUS, window);
        activeWindow = window;
      }
    }
    break;
  case kEventWindowDeactivated:
    if ( window==activeWindow ) 
    {
      Fl::handle( FL_UNFOCUS, window);
      activeWindow = 0;
    }
    break;
  case kEventWindowClose:
    Fl::handle( FL_CLOSE, window ); // this might or might not close the window
    // if there are no more windows, send a high-level quit event
    if (!Fl_X::first) QuitAppleEventHandler( 0, 0, 0 );
    ret = noErr; // returning noErr tells Carbon to stop following up on this event
    break;
  case kEventWindowCollapsed:
    window->clear_visible();
    break;
  case kEventWindowExpanded:
    window->set_visible();
    break;
  }

  fl_unlock_function();

  return ret;
}


/**
 * Carbon Mousewheel handler
 * This needs to be linked into all new window event handlers
 */
static pascal OSStatus carbonMousewheelHandler( EventHandlerCallRef nextHandler, EventRef event, void *userData )
{
  // Handle the new "MightyMouse" mouse wheel events. Please, someone explain
  // to me why Apple changed the API on this even though the current API
  // supports two wheels just fine. Matthias,
  fl_lock_function();

  fl_os_event = event;
  Fl_Window *window = (Fl_Window*)userData;
  if ( !window->shown() )
  {
    fl_unlock_function();
    return noErr;
  }
  Fl::first_window(window);

  EventMouseWheelAxis axis;
  GetEventParameter( event, kEventParamMouseWheelAxis, typeMouseWheelAxis, NULL, sizeof(EventMouseWheelAxis), NULL, &axis );
  long delta;
  GetEventParameter( event, kEventParamMouseWheelDelta, typeLongInteger, NULL, sizeof(long), NULL, &delta );
//  fprintf(stderr, "axis=%d, delta=%d\n", axis, delta);
  if ( axis == kEventMouseWheelAxisX ) {
    Fl::e_dx = -delta;
    Fl::e_dy = 0;
    if ( Fl::e_dx) Fl::handle( FL_MOUSEWHEEL, window );
  } else if ( axis == kEventMouseWheelAxisY ) {
    Fl::e_dx = 0;
    Fl::e_dy = -delta;
    if ( Fl::e_dy) Fl::handle( FL_MOUSEWHEEL, window );
  } else {
    fl_unlock_function();

    return eventNotHandledErr;
  }

  fl_unlock_function();
  
  return noErr;
}


/**
 * convert the current mouse chord into the FLTK modifier state
 */
static void chord_to_e_state( UInt32 chord )
{
  static ulong state[] = 
  { 
    0, FL_BUTTON1, FL_BUTTON3, FL_BUTTON1|FL_BUTTON3, FL_BUTTON2,
    FL_BUTTON2|FL_BUTTON1, FL_BUTTON2|FL_BUTTON3, 
    FL_BUTTON2|FL_BUTTON1|FL_BUTTON3
  };
  Fl::e_state = ( Fl::e_state & 0xff0000 ) | state[ chord & 0x07 ];
}


/**
 * Carbon Mouse Button Handler
 */
static pascal OSStatus carbonMouseHandler( EventHandlerCallRef nextHandler, EventRef event, void *userData )
{
  static int keysym[] = { 0, FL_Button+1, FL_Button+3, FL_Button+2 };
  static int px, py;
  static char suppressed = 0;

  fl_lock_function();
  
  fl_os_event = event;
  Fl_Window *window = (Fl_Window*)userData;
  if ( !window->shown() )
  {
    fl_unlock_function();
    return noErr;
  }
  Fl::first_window(window);
  Point pos;
  GetEventParameter( event, kEventParamMouseLocation, typeQDPoint, NULL, sizeof(Point), NULL, &pos );
  EventMouseButton btn;
  GetEventParameter( event, kEventParamMouseButton, typeMouseButton, NULL, sizeof(EventMouseButton), NULL, &btn );
  UInt32 clickCount;
  GetEventParameter( event, kEventParamClickCount, typeUInt32, NULL, sizeof(UInt32), NULL, &clickCount );
  UInt32 chord;
  GetEventParameter( event, kEventParamMouseChord, typeUInt32, NULL, sizeof(UInt32), NULL, &chord );
  WindowRef xid = fl_xid(window), tempXid;
  int sendEvent = 0, part = 0;
  switch ( GetEventKind( event ) )
  {
  case kEventMouseDown:
    part = FindWindow( pos, &tempXid );
    if (!(Fl::grab() && window!=Fl::grab())) {
      if ( part == inGrow ) {
        fl_unlock_function();
        suppressed = 1;
        Fl_Tooltip::current(0L);
        return CallNextEventHandler( nextHandler, event ); // let the OS handle this for us
      }
      if ( part != inContent ) {
        fl_unlock_function();
        suppressed = 1;
        Fl_Tooltip::current(0L);
        // anything else to here?
        return CallNextEventHandler( nextHandler, event ); // let the OS handle this for us
      }
    }
    suppressed = 0;
    if (part==inContent && !IsWindowActive( xid ) ) {
      CallNextEventHandler( nextHandler, event ); // let the OS handle the activation, but continue to get a click-through effect
    }
    // normal handling of mouse-down follows
    fl_os_capture = xid;
    sendEvent = FL_PUSH;
    Fl::e_is_click = 1; px = pos.h; py = pos.v;
    if (clickCount>1) 
      Fl::e_clicks++;
    else
      Fl::e_clicks = 0;
    // fall through
  case kEventMouseUp:
    if (suppressed) {
      suppressed = 0;
      break;
    }
    if ( !window ) break;
    if ( !sendEvent ) {
      sendEvent = FL_RELEASE; 
    }
    Fl::e_keysym = keysym[ btn ];
    // fall through
  case kEventMouseMoved:
    suppressed = 0;
    if ( !sendEvent ) { 
      sendEvent = FL_MOVE; chord = 0; 
    }
    // fall through
  case kEventMouseDragged:
    if (suppressed) break;
    if ( !sendEvent ) {
      sendEvent = FL_MOVE; // Fl::handle will convert into FL_DRAG
      if (abs(pos.h-px)>5 || abs(pos.v-py)>5) 
        Fl::e_is_click = 0;
    }
    chord_to_e_state( chord );
    GrafPtr oldPort;
    GetPort( &oldPort );
    SetPort( GetWindowPort(xid) ); // \todo replace this! There must be some GlobalToLocal call that has a port as an argument
    SetOrigin(0, 0);
    Fl::e_x_root = pos.h;
    Fl::e_y_root = pos.v;
    GlobalToLocal( &pos );
    Fl::e_x = pos.h;
    Fl::e_y = pos.v;
    SetPort( oldPort );
    if (GetEventKind(event)==kEventMouseDown && part!=inContent) {
      int used = Fl::handle( sendEvent, window );
      CallNextEventHandler( nextHandler, event ); // let the OS handle this for us
      if (!used) 
        suppressed = 1;
    } else {
      Fl::handle( sendEvent, window );
    }
    break;
  }

  fl_unlock_function();
  
  return noErr;
}


/**
 * convert the keyboard return code into the symbol on the keycaps
 */
static unsigned short keycode_to_sym( UInt32 keyCode, UInt32 mods, unsigned short deflt )
{
  static Ptr map = 0;
  UInt32 state = 0;
  if (!map) {
    map = (Ptr)GetScriptManagerVariable(smKCHRCache);
    if (!map) {
      long kbID = GetScriptManagerVariable(smKeyScript);
      map = *GetResource('KCHR', kbID);
    }
  }
  if (map)
    return KeyTranslate(map, keyCode|mods, &state );
  return deflt;
}

/**
 * handle carbon keyboard events
 */
pascal OSStatus carbonKeyboardHandler( 
  EventHandlerCallRef nextHandler, EventRef event, void *userData )
{
  static char buffer[5];
  int sendEvent = 0;
  Fl_Window *window = (Fl_Window*)userData;
  Fl::first_window(window);
  UInt32 mods;
  static UInt32 prevMods = 0xffffffff;

  fl_lock_function();
  
  int kind = GetEventKind(event);
  
  // get the modifiers for any of the events
  GetEventParameter( event, kEventParamKeyModifiers, typeUInt32, 
                     NULL, sizeof(UInt32), NULL, &mods );
  if ( prevMods == 0xffffffff ) prevMods = mods;
  
  // get the key code only for key events
  UInt32 keyCode = 0;
  unsigned char key = 0;
  unsigned short sym = 0;
  if (kind!=kEventRawKeyModifiersChanged) {
    GetEventParameter( event, kEventParamKeyCode, typeUInt32, 
                       NULL, sizeof(UInt32), NULL, &keyCode );
    GetEventParameter( event, kEventParamKeyMacCharCodes, typeChar, 
                       NULL, sizeof(char), NULL, &key );
  }
  /* output a human readbale event identifier for debugging
  const char *ev = "";
  switch (kind) {
    case kEventRawKeyDown: ev = "kEventRawKeyDown"; break;
    case kEventRawKeyRepeat: ev = "kEventRawKeyRepeat"; break;
    case kEventRawKeyUp: ev = "kEventRawKeyUp"; break;
    case kEventRawKeyModifiersChanged: ev = "kEventRawKeyModifiersChanged"; break;
    default: ev = "unknown";
  }
  printf("%08x %08x %08x '%c' %s \n", mods, keyCode, key, key, ev);
  */
  switch (kind)
  {
  case kEventRawKeyDown:
  case kEventRawKeyRepeat:
    // When the user presses a "dead key", no information is send about
    // which dead key symbol was created. So we need to trick Carbon into
    // giving us the code by sending a "space" after the "dead key".
    if (key==0) {
      UInt32 ktState = 0;
      KeyboardLayoutRef klr;
      KLGetCurrentKeyboardLayout(&klr);
      const void *kchar = 0; KLGetKeyboardLayoutProperty(klr, kKLKCHRData, &kchar);
      KeyTranslate(kchar, (mods&0xff00) | keyCode, &ktState); // send the dead key
      key = KeyTranslate(kchar, 0x31, &ktState); // fake a space key press
      Fl::e_state |= 0x40000000; // mark this as a dead key
    } else {
      Fl::e_state &= 0xbfffffff; // clear the deadkey flag
    }
    sendEvent = FL_KEYBOARD;
    // fall through
  case kEventRawKeyUp:
    if ( !sendEvent ) {
      sendEvent = FL_KEYUP;
      Fl::e_state &= 0xbfffffff; // clear the deadkey flag
    }
    // if the user pressed alt/option, event_key should have the keycap, 
    // but event_text should generate the international symbol
    if ( isalpha(key) )
      sym = tolower(key);
    else if ( Fl::e_state&FL_CTRL && key<32 )
      sym = key+96;
    else if ( Fl::e_state&FL_ALT ) // find the keycap of this key
      sym = keycode_to_sym( keyCode & 0x7f, 0, macKeyLookUp[ keyCode & 0x7f ] );
    else
      sym = macKeyLookUp[ keyCode & 0x7f ];
    Fl::e_keysym = Fl::e_original_keysym = sym;
    // Handle FL_KP_Enter on regular keyboards and on Powerbooks
    if ( keyCode==0x4c || keyCode==0x34) key=0x0d;
    // Matt: the Mac has no concept of a NumLock key, or at least not visible
    // Matt: to Carbon. The kEventKeyModifierNumLockMask is only set when
    // Matt: a numeric keypad key is pressed and does not correspond with
    // Matt: the NumLock light in PowerBook keyboards.
    if ( (sym >= FL_KP && sym <= FL_KP_Last) || !(sym & 0xff00) ||
            sym == FL_Tab || sym == FL_Enter) {
      buffer[0] = key;
      Fl::e_length = 1;
    } else {
      buffer[0] = 0;
      Fl::e_length = 0;
    }
    Fl::e_text = buffer;
    // insert UnicodeHandling here!
    break;
  case kEventRawKeyModifiersChanged: {
    UInt32 tMods = prevMods ^ mods;
    if ( tMods )
    {
      mods_to_e_keysym( tMods );
      if ( Fl::e_keysym ) 
        sendEvent = ( prevMods<mods ) ? FL_KEYBOARD : FL_KEYUP;
      Fl::e_length = 0;
      buffer[0] = 0;
      prevMods = mods;
    }
    mods_to_e_state( mods );
    break; }
  }
  while (window->parent()) window = window->window();
  if (sendEvent && Fl::handle(sendEvent,window)) {
    fl_unlock_function();  
    return noErr; // return noErr if FLTK handled the event
  } else {
    fl_unlock_function();
    //return CallNextEventHandler( nextHandler, event );;
    // Matt: I had better results (no duplicate events) always returning
    // Matt: 'noErr'. System keyboard events still seem to work just fine.
    return noErr;
  }
}



/**
 * Open callback function to call...
 */

static void	(*open_cb)(const char *) = 0;


/**
 * Event handler for Apple-O key combination and also for file opens
 * via the finder...
 */

static OSErr OpenAppleEventHandler(const AppleEvent *appleEvt,
                                   AppleEvent *reply,
				   UInt32 refcon) {
  OSErr err;
  AEDescList documents;
  long i, n;
  FSSpec fileSpec;
  AEKeyword keyWd;
  DescType typeCd;
  Size actSz;
  char filename[1024];

  if (!open_cb) return noErr;

  // Initialize the document list...
  AECreateDesc(typeNull, NULL, 0, &documents);
 
  // Get the open parameter(s)...
  err = AEGetParamDesc(appleEvt, keyDirectObject, typeAEList, &documents);
  if (err != noErr) {
    AEDisposeDesc(&documents);
    return err;
  }

  // Lock access to FLTK in this thread...
  fl_lock_function();

  // Open the documents via the callback...
  if (AECountItems(&documents, &n) == noErr) {
    for (i = 1; i <= n; i ++) {
      // Get the next FSSpec record...
      AEGetNthPtr(&documents, i, typeFSS, &keyWd, &typeCd,
                  (Ptr)&fileSpec, sizeof(fileSpec),
		  (actSz = sizeof(fileSpec), &actSz));

      // Convert to a UNIX path...
      FSSpec2UnixPath(&fileSpec, filename);

      // Call the callback with the filename...
      (*open_cb)(filename);
    }
  }

  // Unlock access to FLTK for all threads...
  fl_unlock_function();

  // Get rid of the document list...
  AEDisposeDesc(&documents);

  return noErr;
}


/**
 * Install an open documents event handler...
 */

void fl_open_callback(void (*cb)(const char *)) {
  open_cb = cb;
  if (cb) {
    AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                          NewAEEventHandlerUPP((AEEventHandlerProcPtr)
			      OpenAppleEventHandler), 0, false);
  } else {
    AERemoveEventHandler(kCoreEventClass, kAEOpenDocuments,
                          NewAEEventHandlerUPP((AEEventHandlerProcPtr)
			      OpenAppleEventHandler), false);
  }
}


/**
 * initialize the Mac toolboxes, dock status, and set the default menubar
 */

extern "C" {
  extern OSErr CPSEnableForegroundOperation(ProcessSerialNumber *psn, UInt32 _arg2,
    UInt32 _arg3, UInt32 _arg4, UInt32 _arg5);
}

void fl_open_display() {
  static char beenHereDoneThat = 0;
  if ( !beenHereDoneThat )  {
    beenHereDoneThat = 1;
    
    FlushEvents(everyEvent,0);

    MoreMasters(); // \todo Carbon suggests MoreMasterPointers()
    AEInstallEventHandler( kCoreEventClass, kAEQuitApplication, NewAEEventHandlerUPP((AEEventHandlerProcPtr)QuitAppleEventHandler), 0, false );

    // create the Mac Handle for the default cursor (a pointer to a pointer)
    GetQDGlobalsArrow(&default_cursor);
    default_cursor_ptr = &default_cursor;
    fl_default_cursor  = &default_cursor_ptr;
    
    ClearMenuBar();
    AppendResMenu( GetMenuHandle( 1 ), 'DRVR' );
    DrawMenuBar();

    // bring the application into foreground without a 'CARB' resource
    Boolean same_psn;
    ProcessSerialNumber cur_psn, front_psn;
    if( !GetCurrentProcess( &cur_psn ) && !GetFrontProcess( &front_psn ) &&
        !SameProcess( &front_psn, &cur_psn, &same_psn ) && !same_psn )
    {
      // only transform the application type for unbundled apps
      CFBundleRef bundle = CFBundleGetMainBundle();
      if( bundle )
      {
      	FSRef execFs;
      	CFURLRef execUrl = CFBundleCopyExecutableURL( bundle );
      	CFURLGetFSRef( execUrl, &execFs );

      	FSRef bundleFs;
      	GetProcessBundleLocation( &cur_psn, &bundleFs );

      	if( !FSCompareFSRefs( &execFs, &bundleFs ) )
          bundle = NULL;

        CFRelease(execUrl);
      }

      if( !bundle )
      {
        // Earlier versions of this code tried to use weak linking, however it
	// appears that this does not work on 10.2.  Since 10.3 and higher provide
	// both TransformProcessType and CPSEnableForegroundOperation, the following
	// conditional code compiled on 10.2 will still work on newer releases...
        OSErr err;

#if MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_2
        if (TransformProcessType != NULL) {
	  err = TransformProcessType(&cur_psn, kProcessTransformToForegroundApplication);
	} else
#endif // MAC_OS_X_VERSION_MAX_ALLOWED > MAC_OS_X_VERSION_10_2
        err = CPSEnableForegroundOperation(&cur_psn, 0x03, 0x3C, 0x2C, 0x1103);

        if (err == noErr) {
	  SetFrontProcess( &cur_psn );
        }
      }
    }
  }
}


/**
 * get rid of allocated resources
 */
void fl_close_display()  {
}


/**
 * smallest x ccordinate in screen space
 */
int Fl::x() {
  BitMap r;
  GetQDGlobalsScreenBits(&r);
  return r.bounds.left;
}


/**
 * smallest y ccordinate in screen space
 */
int Fl::y() {
  BitMap r;
  GetQDGlobalsScreenBits(&r);
  return r.bounds.top + 20; // \todo 20 pixel menu bar?
}


/**
 * screen width (single monitor!?)
 */
int Fl::w() {
  BitMap r;
  GetQDGlobalsScreenBits(&r);
  return r.bounds.right - r.bounds.left;
}


/**
 * screen height (single monitor!?)
 */
int Fl::h() {
  BitMap r;
  GetQDGlobalsScreenBits(&r);
  return r.bounds.bottom - r.bounds.top - 20;
}


/**
 * get the current mouse pointer world coordinates
 */
void Fl::get_mouse(int &x, int &y) 
{
  fl_open_display();
  Point loc; 
  GetMouse( &loc );
  LocalToGlobal( &loc );
  x = loc.h;
  y = loc.v;
}


/**
 * convert Mac keystrokes to FLTK
 */
unsigned short mac2fltk(ulong macKey) 
{
  unsigned short cc = macKeyLookUp[(macKey>>8)&0x7f];
  if (cc) return cc;
  return macKey&0xff;
}


/**
 * Initialize the given port for redraw and call the windw's flush() to actually draw the content
 */ 
void Fl_X::flush()
{
  w->flush();
#ifdef __APPLE_QD__
  GrafPtr port; 
  GetPort( &port );
  if ( port )
    QDFlushPortBuffer( port, 0 );
#elif defined (__APPLE_QUARTZ__)
  if (fl_gc) 
    CGContextFlush(fl_gc);
#endif          
  SetOrigin( 0, 0 );
}


/**
 * Handle all clipping and redraw for the given port
 * There are two different callers for this event:
 * 1: the OS can request a redraw and provides all clipping itself
 * 2: Fl::flush() wants all redraws now
 */    
void handleUpdateEvent( WindowPtr xid ) 
{
  Fl_Window *window = fl_find( xid );
  if ( !window ) return;
  GrafPtr oldPort;
  GetPort( &oldPort );
  SetPort( GetWindowPort(xid) );
  Fl_X *i = Fl_X::i( window );
  i->wait_for_expose = 0;
  if ( window->damage() ) {
    if ( i->region ) {
      InvalWindowRgn( xid, i->region );
    }
  }
  if ( i->region ) { // no region, so the sytem will take the update region from the OS
    DisposeRgn( i->region );
    i->region = 0;
  }
  for ( Fl_X *cx = i->xidChildren; cx; cx = cx->xidNext )
  {
    cx->w->clear_damage(window->damage()|FL_DAMAGE_EXPOSE);
    cx->flush();
    cx->w->clear_damage();
  }
  window->clear_damage(window->damage()|FL_DAMAGE_EXPOSE);
  i->flush();
  window->clear_damage();
  SetPort( oldPort );
}     


/**
 * \todo this is a leftover from OS9 times. Please check how much applies to Carbon!
 */
int Fl_X::fake_X_wm(const Fl_Window* w,int &X,int &Y, int &bt,int &bx, int &by) {
  int W, H, xoff, yoff, dx, dy;
  int ret = bx = by = bt = 0;
  if (w->border() && !w->parent()) {
    if (w->maxw != w->minw || w->maxh != w->minh) {
      ret = 2;
      bx = 6; // \todo Mac : GetSystemMetrics(SM_CXSIZEFRAME);
      by = 6; // \todo Mac : get Mac window frame size GetSystemMetrics(SM_CYSIZEFRAME);
    } else {
      ret = 1;
      bx = 6; // \todo Mac : GetSystemMetrics(SM_CXFIXEDFRAME);
      by = 6; // \todo Mac : GetSystemMetrics(SM_CYFIXEDFRAME);
    }
    bt = 22; // \todo Mac : GetSystemMetrics(SM_CYCAPTION);
  }
  //The coordinates of the whole window, including non-client area
  xoff = bx;
  yoff = by + bt;
  dx = 2*bx;
  dy = 2*by + bt;
  X = w->x()-xoff;
  Y = w->y()-yoff;
  W = w->w()+dx;
  H = w->h()+dy;

  //Proceed to positioning the window fully inside the screen, if possible

  // let's get a little elaborate here. Mac OS X puts a lot of stuff on the desk
  // that we want to avoid when positioning our window, namely the Dock and the
  // top menu bar (and even more stuff in 10.4 Tiger). So we will go through the
  // list of all available screens and find the one that this window is most
  // likely to go to, and then reposition it to fit withing the 'good' area.
  Rect r;
  // find the screen, that the center of this window will fall into
  int R = X+W, B = Y+H; // right and bottom
  int cx = (X+R)/2, cy = (Y+B)/2; // center of window;
  GDHandle gd = 0L;
  for (gd = GetDeviceList(); gd; gd = GetNextDevice(gd)) {
  GDPtr gp = *gd;
  if (    cx >= gp->gdRect.left && cx <= gp->gdRect.right
       && cy >= gp->gdRect.top  && cy <= gp->gdRect.bottom)
    break;
  }
  // if the center doesn't fall on a screen, try the top left
  if (!gd) {
    for (gd = GetDeviceList(); gd; gd = GetNextDevice(gd)) {
      GDPtr gp = *gd;
      if (    X >= gp->gdRect.left && X <= gp->gdRect.right
           && Y >= gp->gdRect.top  && Y <= gp->gdRect.bottom)
        break;
    }
  }
  // if that doesn't fall on a screen, try the top right
  if (!gd) {
    for (gd = GetDeviceList(); gd; gd = GetNextDevice(gd)) {
      GDPtr gp = *gd;
      if (    R >= gp->gdRect.left && R <= gp->gdRect.right
           && Y >= gp->gdRect.top  && Y <= gp->gdRect.bottom)
        break;
    }
  }
  // if that doesn't fall on a screen, try the bottom left
  if (!gd) {
    for (gd = GetDeviceList(); gd; gd = GetNextDevice(gd)) {
      GDPtr gp = *gd;
      if (    X >= gp->gdRect.left && X <= gp->gdRect.right
           && B >= gp->gdRect.top  && B <= gp->gdRect.bottom)
        break;
    }
  }
  // last resort, try the bottom right
  if (!gd) {
    for (gd = GetDeviceList(); gd; gd = GetNextDevice(gd)) {
      GDPtr gp = *gd;
      if (    R >= gp->gdRect.left && R <= gp->gdRect.right
           && B >= gp->gdRect.top  && B <= gp->gdRect.bottom)
        break;
    }
  }
  // if we still have not found a screen, we will use the main
  // screen, the one that has the application menu bar.
  if (!gd) gd = GetMainDevice();
  if (gd) {
    GetAvailableWindowPositioningBounds(gd, &r);
    if ( R > r.right )  X -= R - r.right;
    if ( B > r.bottom ) Y -= B - r.bottom;
    if ( X < r.left )   X = r.left;
    if ( Y < r.top )    Y = r.top;
  }

  //Return the client area's top left corner in (X,Y)
  X+=xoff;
  Y+=yoff;

  return ret;
}

/**
 * convert a Mac FSSpec structure into a Unix filename 
 */
static int FSSpec2UnixPath( FSSpec *fs, char *dst )
{
  FSRef fsRef;
  FSpMakeFSRef( fs, &fsRef );
  FSRefMakePath( &fsRef, (UInt8*)dst, 1024 );
  return strlen(dst);
}

static DragReference currDragRef = 0;
static char *currDragData = 0L;
static int currDragSize = 0; 
static OSErr currDragErr = noErr;
Fl_Window *fl_dnd_target_window = 0;
#include <FL/fl_draw.H>

/**
 * Fill the currDrag* variables with the current DnD ASCII text.
 */
static OSErr fillCurrentDragData(DragReference dragRef)
{
  OSErr ret = noErr;
  char *dst = 0L;
  
  // shortcut through this whole procedure if this is still the same drag event
  if (dragRef==currDragRef)
    return currDragErr;
  
  // clear currDrag* for a new drag event
  currDragRef = dragRef;
  if (currDragData) free(currDragData);
  currDragData = 0;
  currDragSize = 0;
  
  // fill currDRag* with ASCII data, if available
  UInt16 i, nItem;
  ItemReference itemRef;
  FlavorFlags flags;
  Size itemSize, size = 0;
  CountDragItems( dragRef, &nItem );
  for ( i = 1; i <= nItem; i++ )
  {
    GetDragItemReferenceNumber( dragRef, i, &itemRef );
    ret = GetFlavorFlags( dragRef, itemRef, 'TEXT', &flags );
    if ( ret == noErr )
    {
      GetFlavorDataSize( dragRef, itemRef, 'TEXT', &itemSize );
      size += itemSize;
    }
    ret = GetFlavorFlags( dragRef, itemRef, 'hfs ', &flags );
    if ( ret == noErr )
    {
      size += 1024; //++ ouch! We should create the full pathname and figure out its length
    }
  }

  if ( !size )
  {
    currDragErr = userCanceledErr;
    return currDragErr;
  }

  currDragSize = size + nItem - 1;
  currDragData = dst = (char*)malloc( size+nItem );;

  for ( i = 1; i <= nItem; i++ )
  {
    GetDragItemReferenceNumber( dragRef, i, &itemRef );
    ret = GetFlavorFlags( dragRef, itemRef, 'TEXT', &flags );
    if ( ret == noErr )
    {
      GetFlavorDataSize( dragRef, itemRef, 'TEXT', &itemSize );
      GetFlavorData( dragRef, itemRef, 'TEXT', dst, &itemSize, 0L );
      dst += itemSize;
      *dst++ = '\n'; // add our element seperator
    }
    ret = GetFlavorFlags( dragRef, itemRef, 'hfs ', &flags );
    if ( ret == noErr )
    {
      HFSFlavor hfs; itemSize = sizeof( hfs );
      GetFlavorData( dragRef, itemRef, 'hfs ', &hfs, &itemSize, 0L );
      itemSize = FSSpec2UnixPath( &hfs.fileSpec, dst );
      dst += itemSize;
      if ( itemSize>1 && ( hfs.fileType=='fold' || hfs.fileType=='disk' ) ) 
        *dst++ = '/';
      *dst++ = '\n'; // add our element seperator
    }
  }

  dst[-1] = 0;
  currDragSize = dst - currDragData - 1;
  currDragErr = ret;
  return ret;
}

/**
 * Drag'n'drop tracking handler
 */
static pascal OSErr dndTrackingHandler( DragTrackingMessage msg, WindowPtr w, void *userData, DragReference dragRef )
{
  Fl_Window *target = (Fl_Window*)userData;
  Fl::first_window(target);
  Point mp;
  static int px, py;
  
  fillCurrentDragData(dragRef);
  Fl::e_length = currDragSize;
  Fl::e_text = currDragData;
  
  switch ( msg )
  {
  case kDragTrackingEnterWindow:
    // check if 'TEXT' is available
    GetDragMouse( dragRef, &mp, 0 );
    Fl::e_x_root = px = mp.h;
    Fl::e_y_root = py = mp.v;
    Fl::e_x = px - target->x();
    Fl::e_y = py - target->y();
    fl_dnd_target_window = target;
    if ( Fl::handle( FL_DND_ENTER, target ) )
      fl_cursor( FL_CURSOR_HAND ); //ShowDragHilite( ); // modify the mouse cursor?!
    else
      fl_cursor( FL_CURSOR_DEFAULT ); //HideDragHilite( dragRef );
    breakMacEventLoop();
    return noErr;
  case kDragTrackingInWindow:
    GetDragMouse( dragRef, &mp, 0 );
    if ( mp.h==px && mp.v==py )
      break;	//+ return previous condition for dnd hiliting
    Fl::e_x_root = px = mp.h;
    Fl::e_y_root = py = mp.v;
    Fl::e_x = px - target->x();
    Fl::e_y = py - target->y();
    fl_dnd_target_window = target;
    if ( Fl::handle( FL_DND_DRAG, target ) )
      fl_cursor( FL_CURSOR_HAND ); //ShowDragHilite( ); // modify the mouse cursor?!
    else
      fl_cursor( FL_CURSOR_DEFAULT ); //HideDragHilite( dragRef );
    breakMacEventLoop();
    return noErr;
    break;
  case kDragTrackingLeaveWindow:
    // HideDragHilite()
    fl_cursor( FL_CURSOR_DEFAULT ); //HideDragHilite( dragRef );
    if ( fl_dnd_target_window )
    {
      Fl::handle( FL_DND_LEAVE, fl_dnd_target_window );
      fl_dnd_target_window = 0;
    }
    breakMacEventLoop();
    return noErr;
  }
  return noErr;
}


/**
 * Drag'n'drop receive handler
 */
static pascal OSErr dndReceiveHandler( WindowPtr w, void *userData, DragReference dragRef )
{
  Point mp;
  OSErr ret;
  
  Fl_Window *target = fl_dnd_target_window = (Fl_Window*)userData;
  Fl::first_window(target);
  GetDragMouse( dragRef, &mp, 0 );
  Fl::e_x_root = mp.h;
  Fl::e_y_root = mp.v;
  Fl::e_x = Fl::e_x_root - target->x();
  Fl::e_y = Fl::e_y_root - target->y();
  if ( !Fl::handle( FL_DND_RELEASE, target ) )
    return userCanceledErr;

  ret = fillCurrentDragData(dragRef);
  if (ret==userCanceledErr)
    return userCanceledErr;
  
  Fl::e_length = currDragSize;
  Fl::e_text = currDragData;
//  printf("Sending following text to widget %p:\n%s\n", Fl::belowmouse(), Fl::e_text);
  int old_event = Fl::e_number;
  Fl::belowmouse()->handle(Fl::e_number = FL_PASTE);
  Fl::e_number = old_event;
  
  if (currDragData) {
    free(currDragData);
  }
  currDragData = 0L;
  currDragRef = 0;
  Fl::e_text = 0L;
  Fl::e_length = 0;
  fl_dnd_target_window = 0L;
  
  breakMacEventLoop();
  return noErr;
}


/**
 * go ahead, create that (sub)window
 * \todo we should make menu windows slightly transparent for the new Mac look
 */
void Fl_X::make(Fl_Window* w)
{
  static int xyPos = 100;
  if ( w->parent() ) // create a subwindow
  {
    Fl_Group::current(0);
    Rect wRect;
    wRect.top    = w->y();
    wRect.left   = w->x();
    wRect.bottom = w->y() + w->h(); if (wRect.bottom<=wRect.top) wRect.bottom = wRect.top+1;
    wRect.right  = w->x() + w->w(); if (wRect.right<=wRect.left) wRect.right = wRect.left+1;
    // our subwindow needs this structure to know about its clipping. 
    Fl_X* x = new Fl_X;
    x->other_xid = 0;
    x->region = 0;
    x->subRegion = 0;
    x->cursor = fl_default_cursor;
    x->gc = 0; // stay 0 for Quickdraw; fill with CGContext for Quartz
    Fl_Window *win = w->window();
    Fl_X *xo = Fl_X::i(win);
    if (xo) {
      x->xidNext = xo->xidChildren;
      x->xidChildren = 0L;
      xo->xidChildren = x;
      x->xid = fl_xid(win);
      x->w = w; w->i = x;
      x->wait_for_expose = 0;
      x->next = Fl_X::first; // must be in the list for ::flush()
      Fl_X::first = x;
      int old_event = Fl::e_number;
      w->handle(Fl::e_number = FL_SHOW);
      Fl::e_number = old_event;
      w->redraw(); // force draw to happen
    }
    fl_show_iconic = 0;
  }
  else // create a desktop window
  {
    Fl_Group::current(0);
    fl_open_display();
    int winclass = kDocumentWindowClass;
    int winattr = kWindowStandardHandlerAttribute | kWindowCloseBoxAttribute | kWindowCollapseBoxAttribute;
    int xp = w->x();
    int yp = w->y();
    int wp = w->w();
    int hp = w->h();
    if (w->size_range_set) {
      if ( w->minh != w->maxh || w->minw != w->maxw)
        winattr |= kWindowFullZoomAttribute | kWindowResizableAttribute | kWindowLiveResizeAttribute;
    } else {
      if (w->resizable()) {
        Fl_Widget *o = w->resizable();
        int minw = o->w(); if (minw > 100) minw = 100;
        int minh = o->h(); if (minh > 100) minh = 100;
        w->size_range(w->w() - o->w() + minw, w->h() - o->h() + minh, 0, 0);
        winattr |= kWindowFullZoomAttribute | kWindowResizableAttribute | kWindowLiveResizeAttribute;
      } else {
        w->size_range(w->w(), w->h(), w->w(), w->h());
      }
    }
    int xwm = xp, ywm = yp, bt, bx, by;

    if (!fake_X_wm(w, xwm, ywm, bt, bx, by)) {
      // menu windows and tooltips
      if (w->modal()||w->override()) {
        winclass = kHelpWindowClass;
        winattr  = 0;
      } else {
        winattr = 512; // kWindowNoTitleBarAttribute;
      }
    } else if (w->modal()) {
      winclass = kMovableModalWindowClass;
    }

    if (by+bt) {
      wp += 2*bx;
      hp += 2*by+bt;
    }
    if (!(w->flags() & Fl_Window::FL_FORCE_POSITION)) {
      // use the Carbon functions below for default window positioning
      w->x(xyPos+Fl::x());
      w->y(xyPos+Fl::y());
      xyPos += 25;
      if (xyPos>200) xyPos = 100;
    } else {
      if (!Fl::grab()) {
        xp = xwm; yp = ywm;
        w->x(xp);w->y(yp);
      }
      xp -= bx;
      yp -= by+bt;
    }

    if (w->non_modal() && Fl_X::first && !fl_disable_transient_for) {
      // find some other window to be "transient for":
      Fl_Window* w = Fl_X::first->w;
      while (w->parent()) w = w->window(); // todo: this code does not make any sense! (w!=w??)
    }

    Rect wRect;
    wRect.top    = w->y();
    wRect.left   = w->x();
    wRect.bottom = w->y() + w->h(); if (wRect.bottom<=wRect.top) wRect.bottom = wRect.top+1;
    wRect.right  = w->x() + w->w(); if (wRect.right<=wRect.left) wRect.right = wRect.left+1;

    const char *name = w->label();
    Str255 pTitle; 
    if (name) {
      if (strlen(name) > 255) pTitle[0] = 255;
      else pTitle[0] = strlen(name);

      memcpy(pTitle+1, name, pTitle[0]);
    } else pTitle[0] = 0;

    Fl_X* x = new Fl_X;
    x->other_xid = 0; // room for doublebuffering image map. On OS X this is only used by overlay windows
    x->region = 0;
    x->subRegion = 0;
    x->cursor = fl_default_cursor;
    x->xidChildren = 0;
    x->xidNext = 0;
    x->gc = 0;

    winattr &= GetAvailableWindowAttributes( winclass );	// make sure that the window will open
    CreateNewWindow( winclass, winattr, &wRect, &(x->xid) );
    SetWTitle(x->xid, pTitle);
    MoveWindow(x->xid, wRect.left, wRect.top, 1);	// avoid Carbon Bug on old OS
    if (w->non_modal() && !w->modal()) {
      // Major kludge: this is to have the regular look, but stay above the document windows
      SetWindowClass(x->xid, kFloatingWindowClass);
      SetWindowActivationScope(x->xid, kWindowActivationScopeAll);
    }
    if (!(w->flags() & Fl_Window::FL_FORCE_POSITION))
    {
      WindowRef pw = Fl_X::first ? Fl_X::first->xid : 0 ;
      if (w->modal()) {
        RepositionWindow(x->xid, pw, kWindowAlertPositionOnParentWindowScreen);
      } else if (w->non_modal()) {
        RepositionWindow(x->xid, pw, kWindowCenterOnParentWindowScreen);
      } else {
        RepositionWindow(x->xid, pw, kWindowCascadeOnParentWindowScreen);
      }
    }
    x->w = w; w->i = x;
    x->wait_for_expose = 1;
    x->next = Fl_X::first;
    Fl_X::first = x;
    { // Install Carbon Event handlers 
      OSStatus ret;
      EventHandlerUPP mousewheelHandler = NewEventHandlerUPP( carbonMousewheelHandler ); // will not be disposed by Carbon...
      static EventTypeSpec mousewheelEvents[] = {
        { kEventClassMouse, kEventMouseWheelMoved } };
      ret = InstallWindowEventHandler( x->xid, mousewheelHandler,
	        (int)(sizeof(mousewheelEvents)/sizeof(mousewheelEvents[0])),
		mousewheelEvents, w, 0L );
      EventHandlerUPP mouseHandler = NewEventHandlerUPP( carbonMouseHandler ); // will not be disposed by Carbon...
      static EventTypeSpec mouseEvents[] = {
        { kEventClassMouse, kEventMouseDown },
        { kEventClassMouse, kEventMouseUp },
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, kEventMouseDragged } };
      ret = InstallWindowEventHandler( x->xid, mouseHandler, 4, mouseEvents, w, 0L );
      EventHandlerUPP keyboardHandler = NewEventHandlerUPP( carbonKeyboardHandler ); // will not be disposed by Carbon...
      static EventTypeSpec keyboardEvents[] = {
        { kEventClassKeyboard, kEventRawKeyDown },
        { kEventClassKeyboard, kEventRawKeyRepeat },
        { kEventClassKeyboard, kEventRawKeyUp },
        { kEventClassKeyboard, kEventRawKeyModifiersChanged } };
      ret = InstallWindowEventHandler( x->xid, keyboardHandler, 4, keyboardEvents, w, 0L );
      EventHandlerUPP windowHandler = NewEventHandlerUPP( carbonWindowHandler ); // will not be disposed by Carbon...
      static EventTypeSpec windowEvents[] = {
        { kEventClassWindow, kEventWindowDrawContent },
        { kEventClassWindow, kEventWindowShown },
        { kEventClassWindow, kEventWindowHidden },
        { kEventClassWindow, kEventWindowActivated },
        { kEventClassWindow, kEventWindowDeactivated },
        { kEventClassWindow, kEventWindowClose },
        { kEventClassWindow, kEventWindowCollapsed },
        { kEventClassWindow, kEventWindowExpanded },
        { kEventClassWindow, kEventWindowBoundsChanging },
        { kEventClassWindow, kEventWindowBoundsChanged } };
      ret = InstallWindowEventHandler( x->xid, windowHandler, 10, windowEvents, w, 0L );
      ret = InstallTrackingHandler( dndTrackingHandler, x->xid, w );
      ret = InstallReceiveHandler( dndReceiveHandler, x->xid, w );
    }

    if ( ! Fl_X::first->next ) // if this is the first window, we need to bring the application to the front
    { 
      ProcessSerialNumber psn;
      OSErr err = GetCurrentProcess( &psn );
      if ( err==noErr ) SetFrontProcess( &psn );
    }
    
    if (w->size_range_set) w->size_range_();
    
    if (winclass != kHelpWindowClass) {
      Fl_Tooltip::enter(0);
    }
    if (w->size_range_set) w->size_range_();
    ShowWindow(x->xid);
    if (fl_show_iconic) { 
      fl_show_iconic = 0;
      CollapseWindow( x->xid, true ); // \todo Mac ; untested
    } else {
      w->set_visible();
    }

    Rect rect;
    GetWindowBounds(x->xid, kWindowContentRgn, &rect);
    w->x(rect.left); w->y(rect.top);
    w->w(rect.right-rect.left); w->h(rect.bottom-rect.top);

    int old_event = Fl::e_number;
    w->handle(Fl::e_number = FL_SHOW);
    Fl::e_number = old_event;
    w->redraw(); // force draw to happen
    
    if (w->modal()) { Fl::modal_ = w; fl_fix_focus(); }
  }
}


/**
 * Tell the OS what window sizes we want to allow
 */
void Fl_Window::size_range_() {
  size_range_set = 1;
  HISize minSize = { minw, minh };
  HISize maxSize = { maxw?maxw:32000, maxh?maxh:32000 };
  if (i && i->xid)
    SetWindowResizeLimits(i->xid, &minSize, &maxSize);
}


/**
 * returns pointer to the filename, or null if name ends with ':'
 */
const char *fl_filename_name( const char *name ) 
{
  const char *p, *q;
  if (!name) return (0);
  for ( p = q = name ; *p ; ) 
  {
    if ( ( p[0] == ':' ) && ( p[1] == ':' ) ) 
    {
      q = p+2;
      p++;
    }
    else if (p[0] == '/')
      q = p + 1;
    p++;
  }
  return q;
}


/**
 * set the window title bar
 * \todo make the titlebar icon work!
 */
void Fl_Window::label(const char *name,const char */*iname*/) {
  Fl_Widget::label(name);
  Str255 pTitle;

  if (name) { pTitle[0] = strlen(name); memcpy(pTitle+1, name, pTitle[0]); }
  else pTitle[0] = 0;

  if (shown() || i) SetWTitle(fl_xid(this), pTitle);
}


/**
 * make a window visible
 */
void Fl_Window::show() {
  image(Fl::scheme_bg_);
  if (Fl::scheme_bg_) {
    labeltype(FL_NORMAL_LABEL);
    align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  } else {
    labeltype(FL_NO_LABEL);
  }
  Fl_Tooltip::exit(this);
  if (!shown() || !i) {
    Fl_X::make(this);
  } else {
      if ( !parent() )
      {
        if ( IsWindowCollapsed( i->xid ) ) CollapseWindow( i->xid, false );
        if (!fl_capture) {
          BringToFront(i->xid);
          SelectWindow(i->xid);
        }
      }
  }
}


/**
 * resize a window
 */
void Fl_Window::resize(int X,int Y,int W,int H) {
  if (W<=0) W = 1; // OS X does not like zero width windows
  if (H<=0) H = 1;
  int is_a_resize = (W != w() || H != h());
//  printf("Fl_Winodw::resize(X=%d, Y=%d, W=%d, H=%d), is_a_resize=%d, resize_from_system=%p, this=%p\n",
//         X, Y, W, H, is_a_resize, resize_from_system, this);
  if (X != x() || Y != y()) set_flag(FL_FORCE_POSITION);
  else if (!is_a_resize) return;
  if ( (resize_from_system!=this) && (!parent()) && shown()) {
    if (is_a_resize) {
      if (resizable()) {
        if (W<minw) minw = W; // user request for resize takes priority
        if (W>maxw) maxw = W; // over a previously set size_range
        if (H<minh) minh = H;
        if (H>maxh) maxh = H;
        size_range(minw, minh, maxw, maxh);
      } else {
        size_range(W, H, W, H);
      }
      Rect dim; dim.left=X; dim.top=Y; dim.right=X+W; dim.bottom=Y+H;
      SetWindowBounds(i->xid, kWindowContentRgn, &dim);
      Rect all; all.top=-32000; all.bottom=32000; all.left=-32000; all.right=32000;
      InvalWindowRect( i->xid, &all );    
    } else {
      MoveWindow(i->xid, X, Y, 0);
    }
  }
  resize_from_system = 0;
  if (is_a_resize) {
    Fl_Group::resize(X,Y,W,H);
    if (shown()) { 
      redraw(); 
    }
  } else {
    x(X); y(Y); 
  }
}


/**
 * make all drawing go into this window (called by subclass flush() impl.)
 */
void Fl_Window::make_current() 
{
#ifdef __APPLE_QUARTZ__
  OSStatus err;
  Fl_X::q_release_context();
#endif
  if ( !fl_window_region )
    fl_window_region = NewRgn();
  fl_window = i->xid;
  current_ = this;

  SetPort( GetWindowPort(i->xid) ); // \todo check for the handling of doublebuffered windows

  int xp = 0, yp = 0;
  Fl_Window *win = this;
  while ( win ) 
  {
    if ( !win->window() )
      break;
    xp += win->x();
    yp += win->y();
    win = (Fl_Window*)win->window();
  }
  SetOrigin( -xp, -yp );
  
  SetRectRgn( fl_window_region, 0, 0, w(), h() );
  
  // \todo for performance reasons: we don't have to create this unless the child windows moved
  for ( Fl_X *cx = i->xidChildren; cx; cx = cx->xidNext )
  {
    Fl_Window *cw = cx->w;
    if (!cw->visible_r()) continue;
    Fl_Region r = NewRgn();
    SetRectRgn( r, cw->x() - xp, cw->y() - yp, 
                   cw->x() + cw->w() - xp, cw->y() + cw->h() - yp );
    DiffRgn( fl_window_region, r, fl_window_region );
    DisposeRgn( r );
  }
 
#ifdef __APPLE_QUARTZ__
  err = QDBeginCGContext(GetWindowPort(i->xid), &i->gc);
  if (err!=noErr) 
    fprintf(stderr, "Error %d in QDBeginCGContext\n", (int)err);
  fl_gc = i->gc;
  CGContextSaveGState(fl_gc);
  Fl_X::q_fill_context();
#endif
  fl_clip_region( 0 );
  SetPortClipRegion( GetWindowPort(i->xid), fl_window_region );
  return;
}

// helper function to manage the current CGContext fl_gc
#ifdef __APPLE_QUARTZ__
extern Fl_Color fl_color_;
extern class Fl_FontSize *fl_fontsize;
extern void fl_font(class Fl_FontSize*);
extern void fl_quartz_restore_line_style_();

// FLTK has only one global graphics state. This function copies the FLTK state into the
// current Quartz context
void Fl_X::q_fill_context() {
  if (!fl_gc) return;
  int hgt = 0;
  if (fl_window) {
    Rect portRect; 
    GetPortBounds(GetWindowPort( fl_window ), &portRect);
    hgt = portRect.bottom-portRect.top;
  } else {
    hgt = CGBitmapContextGetHeight(fl_gc);
  }
  CGContextTranslateCTM(fl_gc, 0.5, hgt-0.5f);
  CGContextScaleCTM(fl_gc, 1.0f, -1.0f);
  fl_font(fl_fontsize);
  fl_color(fl_color_);
  fl_quartz_restore_line_style_();
}

// The only way to reset clipping to its original state is to pop the current graphics
// state and restore the global state.
void Fl_X::q_clear_clipping() {
  if (!fl_gc) return;
  CGContextRestoreGState(fl_gc);
  CGContextSaveGState(fl_gc);
}

// Give the Quartz context back to the system
void Fl_X::q_release_context(Fl_X *x) {
  if (x && x->gc!=fl_gc) return;
  if (!fl_gc) return;
  CGContextRestoreGState(fl_gc);
  if (fl_window) {
    OSStatus err = QDEndCGContext(GetWindowPort(fl_window), &fl_gc);
    if (err!=noErr)
      fprintf(stderr, "Error %d in QDEndCGContext\n", (int)err);
  }
  fl_gc = 0;
}

void Fl_X::q_begin_image(CGRect &rect, int cx, int cy, int w, int h) {
  CGContextSaveGState(fl_gc);
  CGAffineTransform mx = CGContextGetCTM(fl_gc);
  CGRect r2 = rect;
  r2.origin.x -= 0.5f;
  r2.origin.y -= 0.5f;
  CGContextClipToRect(fl_gc, r2);
  mx.d = -1.0; mx.tx = -mx.tx;
  CGContextConcatCTM(fl_gc, mx);
  rect.origin.x = -(mx.tx+0.5f) + rect.origin.x     - cx;
  rect.origin.y =  (mx.ty+0.5f) - rect.origin.y - h + cy;
  rect.size.width = w;
  rect.size.height = h;
}

void Fl_X::q_end_image() {
  CGContextRestoreGState(fl_gc);
}

#endif

////////////////////////////////////////////////////////////////
// Cut & paste.

Fl_Widget *fl_selection_requestor = 0;
char *fl_selection_buffer[2];
int fl_selection_length[2];
int fl_selection_buffer_length[2];
static ScrapRef myScrap = 0;

/**
 * create a selection
 * owner: widget that created the selection
 * stuff: pointer to selected data
 * size of selected data
 */
void Fl::copy(const char *stuff, int len, int clipboard) {
  if (!stuff || len<0) return;
  if (len+1 > fl_selection_buffer_length[clipboard]) {
    delete[] fl_selection_buffer[clipboard];
    fl_selection_buffer[clipboard] = new char[len+100];
    fl_selection_buffer_length[clipboard] = len+100;
  }
  memcpy(fl_selection_buffer[clipboard], stuff, len);
  fl_selection_buffer[clipboard][len] = 0; // needed for direct paste
  fl_selection_length[clipboard] = len;
  if (clipboard) {
    ClearCurrentScrap();
    OSStatus ret = GetCurrentScrap( &myScrap );
    if ( ret != noErr ) {
      myScrap = 0;
      return;
    }
    // Previous version changed \n to \r before sending the text, but I would
    // prefer to leave the local buffer alone, so a copied buffer may be
    // needed. Check to see if this is necessary on OS/X.
    PutScrapFlavor( myScrap, kScrapFlavorTypeText, 0,
		    len, fl_selection_buffer[1] );
  }
}

// Call this when a "paste" operation happens:
void Fl::paste(Fl_Widget &receiver, int clipboard) {
  if (clipboard) {
    // see if we own the selection, if not go get it:
    ScrapRef scrap = 0;
    Size len = 0;
    if (GetCurrentScrap(&scrap) == noErr && scrap != myScrap &&
	GetScrapFlavorSize(scrap, kScrapFlavorTypeText, &len) == noErr) {
      if ( len >= fl_selection_buffer_length[1] ) {
	fl_selection_buffer_length[1] = len + 32;
	delete[] fl_selection_buffer[1];
	fl_selection_buffer[1] = new char[len + 32];
      }
      fl_selection_length[1] = len; len++;
      GetScrapFlavorData( scrap, kScrapFlavorTypeText, &len,
			  fl_selection_buffer[1] );
      fl_selection_buffer[1][fl_selection_length[1]] = 0;
      // turn all \r characters into \n:
      for (int x = 0; x < len; x++) {
	if (fl_selection_buffer[1][x] == '\r')
	  fl_selection_buffer[1][x] = '\n';
      }
    }
  }
  Fl::e_text = fl_selection_buffer[clipboard];
  Fl::e_length = fl_selection_length[clipboard];
  if (!Fl::e_text) Fl::e_text = (char *)"";
  receiver.handle(FL_PASTE);
  return;
}

void Fl::add_timeout(double time, Fl_Timeout_Handler cb, void* data)
{
   // check, if this timer slot exists already
   for (int i = 0;  i < mac_timer_used;  ++i) {
        MacTimeout& t = mac_timers[i];
        // if so, simply change the fire interval
        if (t.callback == cb  &&  t.data == data) {
            SetEventLoopTimerNextFireTime(t.timer, (EventTimerInterval)time);
            t.pending = 1;
            return;
        }
    }
    // no existing timer to use. Create a new one:
    int timer_id = -1;
    // find an empty slot in the timer array
    for (int i = 0;  i < mac_timer_used;  ++i) {
        if ( !mac_timers[i].timer ) {
            timer_id = i;
            break;
        }
    }
    // if there was no empty slot, append a new timer
    if (timer_id == -1) {
        // make space if needed
        if (mac_timer_used == mac_timer_alloc) {
            realloc_timers();
        }
        timer_id = mac_timer_used++;
    }
    // now install a brand new timer
    MacTimeout& t = mac_timers[timer_id];
    EventTimerInterval fireDelay = (EventTimerInterval)time;
    EventLoopTimerUPP  timerUPP = NewEventLoopTimerUPP(do_timer);
    EventLoopTimerRef  timerRef = 0;
    OSStatus err = InstallEventLoopTimer(GetMainEventLoop(), fireDelay, 0, timerUPP, data, &timerRef);
    if (err == noErr) {
        t.callback = cb;
        t.data     = data;
        t.timer    = timerRef;
        t.upp      = timerUPP;
        t.pending  = 1;
    } else {
        if (timerRef) 
            RemoveEventLoopTimer(timerRef);
        if (timerUPP)
            DisposeEventLoopTimerUPP(timerUPP);
    }
}

void Fl::repeat_timeout(double time, Fl_Timeout_Handler cb, void* data)
{
    // currently, repeat_timeout does not subtract the trigger time of the previous timer event as it should.
    add_timeout(time, cb, data);
}

int Fl::has_timeout(Fl_Timeout_Handler cb, void* data)
{
   for (int i = 0;  i < mac_timer_used;  ++i) {
        MacTimeout& t = mac_timers[i];
        if (t.callback == cb  &&  t.data == data && t.pending) {
            return 1;
        }
    }
    return 0;
}

void Fl::remove_timeout(Fl_Timeout_Handler cb, void* data)
{
   for (int i = 0;  i < mac_timer_used;  ++i) {
        MacTimeout& t = mac_timers[i];
        if (t.callback == cb  && ( t.data == data || data == NULL)) {
            delete_timer(t);
        }
    }
}

int MacUnlinkWindow(Fl_X *ip, Fl_X *start) {
  if (!ip) return 0;
  if (start) {
    Fl_X *pc = start;
    while (pc) {
      if (pc->xidNext == ip) {
        pc->xidNext = ip->xidNext;
        return 1;
      }
      if (pc->xidChildren) {
        if (pc->xidChildren == ip) {
          pc->xidChildren = ip->xidNext;
          return 1;
        }
        if (MacUnlinkWindow(ip, pc->xidChildren))
          return 1;
      }
      pc = pc->xidNext;
    }
  } else {
    for ( Fl_X *pc = Fl_X::first; pc; pc = pc->next ) {
      if (MacUnlinkWindow(ip, pc))
        return 1;
    }
  }  
  return 0;
}

static void MacRelinkWindow(Fl_X *x, Fl_X *p) {
  if (!x || !p) return;
  // first, check if 'x' is already registered as a child of 'p'
  for (Fl_X *i = p->xidChildren; i; i=i->xidNext) {
    if (i == x) return;
  }
  // now add 'x' as the first child of 'p'
  x->xidNext = p->xidChildren;
  p->xidChildren = x;
}

void MacDestroyWindow(Fl_Window *w, WindowPtr p) {
  MacUnmapWindow(w, p);
  if (w && !w->parent() && p)
    DisposeWindow(p);
}

void MacMapWindow(Fl_Window *w, WindowPtr p) {
  if (w && p)
    ShowWindow(p);
  //+ link to window list
  if (w && w->parent()) {
    MacRelinkWindow(Fl_X::i(w), Fl_X::i(w->window()));
    w->redraw();
  }
}

void MacUnmapWindow(Fl_Window *w, WindowPtr p) {
  if (w && !w->parent() && p) 
    HideWindow(p);
  if (w && Fl_X::i(w)) 
    MacUnlinkWindow(Fl_X::i(w));
}

//
// End of "$Id: Fl_mac.cxx 6033 2008-02-20 18:17:34Z matt $".
//
