//
// "$Id: Fl_lock.cxx 5950 2007-10-07 10:44:28Z matt $"
//
// Multi-threading support code for the Fast Light Tool Kit (FLTK).
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


#include <FL/Fl.H>
#include <config.h>

#include <stdlib.h>

/*
   From Bill:

   I would prefer that FLTK contain the minimal amount of extra
   stuff for doing threads.  There are other portable thread
   wrapper libraries out there and FLTK should not be providing
   another.  This file is an attempt to make minimal additions
   and make them self-contained in this source file.

   From Mike:

   Starting with 1.1.8, we now have a callback so that you can
   process awake() messages as they come in.


   The API:

   Fl::lock() - recursive lock.  You must call this before the
   first call to Fl::wait()/run() to initialize the thread
   system. The lock is locked all the time except when
   Fl::wait() is waiting for events.

   Fl::unlock() - release the recursive lock.

   Fl::awake(void*) - Causes Fl::wait() to return (with the lock
   locked) even if there are no events ready.

   Fl::awake(void (*cb)(void *), void*) - Call a function
   in the main thread from within another thread of execution.

   Fl::thread_message() - returns an argument sent to an
   Fl::awake() call, or returns NULL if none.  WARNING: the
   current implementation only has a one-entry queue and only
   returns the most recent value!
*/

Fl_Awake_Handler *Fl::awake_ring_;
void **Fl::awake_data_;
int Fl::awake_ring_size_;
int Fl::awake_ring_head_;
int Fl::awake_ring_tail_;
static const int AWAKE_RING_SIZE = 1024;

static void lock_ring();
static void unlock_ring();


int Fl::add_awake_handler_(Fl_Awake_Handler func, void *data)
{
  int ret = 0;
  lock_ring();
  if (!awake_ring_) {
    awake_ring_size_ = AWAKE_RING_SIZE;
    awake_ring_ = (Fl_Awake_Handler*)malloc(awake_ring_size_*sizeof(Fl_Awake_Handler));
    awake_data_ = (void**)malloc(awake_ring_size_*sizeof(void*));
  }
  if (awake_ring_head_==awake_ring_tail_-1 || awake_ring_head_+1==awake_ring_tail_) {
    // ring is full. Return -1 as an error indicator.
    ret = -1;
  } else {
    awake_ring_[awake_ring_head_] = func;
    awake_data_[awake_ring_head_] = data;
    ++awake_ring_head_;
    if (awake_ring_head_ == awake_ring_size_)
      awake_ring_head_ = 0;
  }
  unlock_ring();
  return ret;
}

int Fl::get_awake_handler_(Fl_Awake_Handler &func, void *&data)
{
  int ret = 0;
  lock_ring();
  if (!awake_ring_ || awake_ring_head_ == awake_ring_tail_) {
    ret = -1;
  } else {
    func = awake_ring_[awake_ring_tail_];
    data = awake_data_[awake_ring_tail_];
    ++awake_ring_tail_;
    if (awake_ring_tail_ == awake_ring_size_)
      awake_ring_tail_ = 0;
  }
  unlock_ring();
  return ret;
}

//
// 'Fl::awake()' - Let the main thread know an update is pending
//                 and have it cal a specific function
//
int Fl::awake(Fl_Awake_Handler func, void *data) {
  int ret = add_awake_handler_(func, data);
  Fl::awake();
  return ret;
}

////////////////////////////////////////////////////////////////
// Windows threading...
#ifdef WIN32
#  include <windows.h>
#  include <process.h>
#  include <FL/x.H>

// These pointers are in Fl_win32.cxx:
extern void (*fl_lock_function)();
extern void (*fl_unlock_function)();

// The main thread's ID
static DWORD main_thread;

// Microsoft's version of a MUTEX...
CRITICAL_SECTION cs;
CRITICAL_SECTION *cs_ring;

void unlock_ring() {
  LeaveCriticalSection(cs_ring);
}

void lock_ring() {
  if (!cs_ring) {
    cs_ring = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    InitializeCriticalSection(cs_ring);
  }
  EnterCriticalSection(cs_ring);
}

//
// 'unlock_function()' - Release the lock.
//

static void unlock_function() {
  LeaveCriticalSection(&cs);
}

//
// 'lock_function()' - Get the lock.
//

static void lock_function() {
  EnterCriticalSection(&cs);
}

//
// 'Fl::lock()' - Lock access to FLTK data structures...
//

void Fl::lock() {
  if (!main_thread) InitializeCriticalSection(&cs);

  lock_function();

  if (!main_thread) {
    fl_lock_function   = lock_function;
    fl_unlock_function = unlock_function;
    main_thread        = GetCurrentThreadId();
  }
}

//
// 'Fl::unlock()' - Unlock access to FLTK data structures...
//

void Fl::unlock() {
  unlock_function();
}


//
// 'Fl::awake()' - Let the main thread know an update is pending.
//
// When called from a thread, it causes FLTK to awake from Fl::wait()...
//

void Fl::awake(void* msg) {
  PostThreadMessage( main_thread, fl_wake_msg, (WPARAM)msg, 0);
}

////////////////////////////////////////////////////////////////
// POSIX threading...
#elif HAVE_PTHREAD
#  include <unistd.h>
#  include <fcntl.h>
#  include <pthread.h>

// Pipe for thread messaging via Fl::awake()...
static int thread_filedes[2];

// Mutex and state information for Fl::lock() and Fl::unlock()...
static pthread_mutex_t fltk_mutex;
static pthread_t owner;
static int counter;

static void lock_function_init_std() {
  pthread_mutex_init(&fltk_mutex, NULL);
}

static void lock_function_std() {
  if (!counter || owner != pthread_self()) {
    pthread_mutex_lock(&fltk_mutex);
    owner = pthread_self();
  }
  counter++;
}

static void unlock_function_std() {
  if (!--counter) pthread_mutex_unlock(&fltk_mutex);
}

#  ifdef PTHREAD_MUTEX_RECURSIVE
static bool lock_function_init_rec() {
  pthread_mutexattr_t attrib;
  pthread_mutexattr_init(&attrib);
  if (pthread_mutexattr_settype(&attrib, PTHREAD_MUTEX_RECURSIVE)) {
    pthread_mutexattr_destroy(&attrib);
    return true;
  }

  pthread_mutex_init(&fltk_mutex, &attrib);
  return false;
}

static void lock_function_rec() {
  pthread_mutex_lock(&fltk_mutex);
}

static void unlock_function_rec() {
  pthread_mutex_unlock(&fltk_mutex);
}
#  endif // PTHREAD_MUTEX_RECURSIVE

void Fl::awake(void* msg) {
  write(thread_filedes[1], &msg, sizeof(void*));
}

static void* thread_message_;
void* Fl::thread_message() {
  void* r = thread_message_;
  thread_message_ = 0;
  return r;
}

static void thread_awake_cb(int fd, void*) {
  read(fd, &thread_message_, sizeof(void*));
  Fl_Awake_Handler func;
  void *data;
  while (Fl::get_awake_handler_(func, data)==0) {
    (*func)(data);
  }
}

// These pointers are in Fl_x.cxx:
extern void (*fl_lock_function)();
extern void (*fl_unlock_function)();

void Fl::lock() {
  if (!thread_filedes[1]) {
    // Initialize thread communication pipe to let threads awake FLTK
    // from Fl::wait()
    pipe(thread_filedes);

    // Make the write side of the pipe non-blocking to avoid deadlock
    // conditions (STR #1537)
    fcntl(thread_filedes[1], F_SETFL,
          fcntl(thread_filedes[1], F_GETFL) | O_NONBLOCK);

    // Monitor the read side of the pipe so that messages sent via
    // Fl::awake() from a thread will "wake up" the main thread in
    // Fl::wait().
    Fl::add_fd(thread_filedes[0], FL_READ, thread_awake_cb);

    // Set lock/unlock functions for this system, using a system-supplied
    // recursive mutex if supported...
#  ifdef PTHREAD_MUTEX_RECURSIVE
    if (!lock_function_init_rec()) {
      fl_lock_function   = lock_function_rec;
      fl_unlock_function = unlock_function_rec;
    } else {
#  endif // PTHREAD_MUTEX_RECURSIVE
      lock_function_init_std();
      fl_lock_function   = lock_function_std;
      fl_unlock_function = unlock_function_std;
#  ifdef PTHREAD_MUTEX_RECURSIVE
    }
#  endif // PTHREAD_MUTEX_RECURSIVE
  }

  fl_lock_function();
}

void Fl::unlock() {
  fl_unlock_function();
}

// Mutex code for the awake ring buffer
static pthread_mutex_t *ring_mutex;

void unlock_ring() {
  pthread_mutex_unlock(ring_mutex);
}

void lock_ring() {
  if (!ring_mutex) {
    ring_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(ring_mutex, NULL);
  }
  pthread_mutex_lock(ring_mutex);
}

#else

void unlock_ring() {
}

void lock_ring() {
}

void Fl::awake(void*) {
}

#endif // WIN32

//
// End of "$Id: Fl_lock.cxx 5950 2007-10-07 10:44:28Z matt $".
//
