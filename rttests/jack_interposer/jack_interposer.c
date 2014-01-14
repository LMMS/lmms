// needed to use RTLD_NEXT from dlfcn.h
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dlfcn.h>
#include <poll.h>
#include <jack/jack.h>
#include <stdarg.h>

#define ABORT_ON_VIOLATION 1

// Define THREAD_LOCAL as the keyword for thread-local storage if the compiler
// supports it.
#if ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)) || (__GNUC__ > 3)
#define THREAD_LOCAL __thread
#else
#define THREAD_LOCAL
#endif

// is set to 'true' when entering the process-callback and to 'false' when 
// leaving it. When set to 'true', calls to non-realtime functions will 
// cause warnings/errors.
// 
// If this library is compiled with GCC 3.3 or later in_rt will have
// thread-local storage, which means that it should work on SMP machines and
// with multiple clients in the same process.
//
// If this library is built using another compiler it will NOT have
// thread-local storage, thus introducing the limitation that jack_interposer
// is only usable on single-CPU machines (or machines configured to run the
// application under test on only 1 CPU).
bool THREAD_LOCAL in_rt = false;

#include "checkers.c"
#include "manual.c"

JackProcessCallback real_process_callback;

int interposed_process_callback(jack_nframes_t nframes, void* arg)
{
  int result;

  in_rt = true;

  result = real_process_callback(nframes, arg);

  in_rt = false;

  return result;
}

int jack_set_process_callback(jack_client_t* client,
	JackProcessCallback process_callback, void* arg)
{
  static int (*func)() = NULL;
  int result;

  if(!func)
    func = (int(*)()) dlsym(RTLD_NEXT, "jack_set_process_callback");
  if(!func)
  {
    fprintf(stderr, "Error dlsym'ing jack_set_process_callback\n");
    abort();
  }
 
  real_process_callback = process_callback;

  result = func(client, interposed_process_callback, arg);

  return result;
}
