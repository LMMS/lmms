//
// "$Id: fl_open_uri.cxx 5636 2007-01-23 20:45:28Z mike $"
//
// fl_open_uri() code for FLTK.
//
// Test with:
//
//    gcc -I/fltk/dir -I/fltk/dir/src -DTEST -o fl_open_uri fl_open_uri.cxx -lfltk
//
// Copyright 2003-2007 by Michael R Sweet
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

//
// Include necessary headers...
//

#include <FL/filename.H>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "flstring.h"
#ifdef WIN32
#  include <windows.h>
#  include <shellapi.h>
#else
#  include <sys/wait.h>
#  include <signal.h>
#  include <fcntl.h>
#  include <unistd.h>
#endif // WIN32


//
// Local functions...
//

#if !defined(WIN32) && !defined(__APPLE__)
static char	*path_find(const char *program, char *filename, int filesize);
#endif // !WIN32 && !__APPLE__
#ifndef WIN32
static int	run_program(const char *program, char **argv, char *msg, int msglen);
#endif // !WIN32


/**
 * Open the specified URI.
 *
 * fl_open_uri() opens the specified Uniform Resource Identifier (URI)
 * using an operating-system dependent program or interface. For URIs
 * using the "ftp", "http", or "https" schemes, the system default web
 * browser is used to open the URI, while "mailto" and "news" URIs are
 * typically opened using the system default mail reader and "file" URIs
 * are opened using the file system navigator.
 *
 * On success, the (optional) msg buffer is filled with the command that
 * was run to open the URI; on Windows, this will always be "open uri".
 *
 * On failure, the msg buffer is filled with an English error message.
 *
 * @param uri The URI to open
 * @param msg Optional buffer which contains the command or error message
 * @param msglen Length of optional buffer
 * @return 1 on success, 0 on failure
 */

int
fl_open_uri(const char *uri, char *msg, int msglen) {
  // Supported URI schemes...
  static const char * const schemes[] = {
    "file://",
    "ftp://",
    "http://",
    "https://",
    "mailto:",
    "news://",
    NULL
  };

  // Validate the URI scheme...
  int i;
  for (i = 0; schemes[i]; i ++)
    if (!strncmp(uri, schemes[i], strlen(schemes[i])))
      break;

  if (!schemes[i]) {
    if (msg) {
      char scheme[255];
      if (sscanf(uri, "%254[^:]", scheme) == 1) {
        snprintf(msg, msglen, "URI scheme \"%s\" not supported.", scheme);
      } else {
        snprintf(msg, msglen, "Bad URI \"%s\"", uri);
      }
    }

    return 0;
  }

#ifdef WIN32
  if (msg) snprintf(msg, msglen, "open %s", uri);

  return (int)ShellExecute(HWND_DESKTOP, "open", uri, NULL, NULL, SW_SHOW) > 32;

#elif defined(__APPLE__)
  char	*argv[3];			// Command-line arguments

  argv[0] = "open";
  argv[1] = (char *)uri;
  argv[2] = 0;

  if (msg) snprintf(msg, msglen, "open %s", uri);

  return run_program("/usr/bin/open", argv, msg, msglen) != 0;

#else // !WIN32 && !__APPLE__
  // Run any of several well-known commands to open the URI.
  //
  // We give preference to the Portland group's xdg-utils
  // programs which run the user's preferred web browser, etc.
  // based on the current desktop environment in use.  We fall
  // back on older standards and then finally test popular programs
  // until we find one we can use.
  //
  // Note that we specifically do not support the MAILER and
  // BROWSER environment variables because we have no idea whether
  // we need to run the listed commands in a terminal program.

  char	command[1024],			// Command to run...
	*argv[4],			// Command-line arguments
	remote[1024];			// Remote-mode command...
  const char * const *commands;		// Array of commands to check...
  static const char * const browsers[] = {
    "xdg-open", // Portland
    "htmlview", // Freedesktop.org
    "firefox",
    "mozilla",
    "netscape",
    "konqueror", // KDE
    "opera",
    "hotjava", // Solaris
    "mosaic",
    NULL
  };
  static const char * const readers[] = {
    "xdg-email", // Portland
    "thunderbird",
    "mozilla",
    "netscape",
    "evolution", // GNOME
    "kmailservice", // KDE
    NULL
  };
  static const char * const managers[] = {
    "xdg-open", // Portland
    "fm", // IRIX
    "dtaction", // CDE
    "nautilus", // GNOME
    "konqueror", // KDE
    NULL
  };

  // Figure out which commands to check for...
  if (!strncmp(uri, "file://", 7)) commands = managers;
  else if (!strncmp(uri, "mailto:", 7) ||
           !strncmp(uri, "news:", 5)) commands = readers;
  else commands = browsers;

  // Find the command to run...
  for (i = 0; commands[i]; i ++)
    if (path_find(commands[i], command, sizeof(command))) break;

  if (!commands[i]) {
    if (msg) {
      snprintf(msg, msglen, "No helper application found for \"%s\"", uri);
    }

    return 0;
  }

  // Handle command-specific arguments...
  argv[0] = (char *)commands[i];

  if (!strcmp(commands[i], "firefox") ||
      !strcmp(commands[i], "mozilla") ||
      !strcmp(commands[i], "netscape") ||
      !strcmp(commands[i], "thunderbird")) {
    // program -remote openURL(uri)
    snprintf(remote, sizeof(remote), "openURL(%s)", uri);

    argv[1] = (char *)"-remote";
    argv[2] = remote;
    argv[3] = 0;
  } else if (!strcmp(commands[i], "dtaction")) {
    // dtaction open uri
    argv[1] = (char *)"open";
    argv[2] = (char *)uri;
    argv[3] = 0;
  } else {
    // program uri
    argv[1] = (char *)uri;
    argv[2] = 0;
  }

  if (msg) {
    strlcpy(msg, argv[0], msglen);

    for (i = 1; argv[i]; i ++) {
      strlcat(msg, " ", msglen);
      strlcat(msg, argv[i], msglen);
    }
  }

  return run_program(command, argv, msg, msglen) != 0;
#endif // WIN32
}


#if !defined(WIN32) && !defined(__APPLE__)
// Find a program in the path...
static char *path_find(const char *program, char *filename, int filesize) {
  const char	*path;			// Search path
  char		*ptr,			// Pointer into filename
		*end;			// End of filename buffer


  if ((path = getenv("PATH")) == NULL) path = "/bin:/usr/bin";

  for (ptr = filename, end = filename + filesize - 1; *path; path ++) {
    if (*path == ':') {
      if (ptr > filename && ptr[-1] != '/' && ptr < end) *ptr++ = '/';

      strlcpy(ptr, program, end - ptr + 1);

      if (!access(filename, X_OK)) return filename;

      ptr = filename;
    } else if (ptr < end) *ptr++ = *path;
  }

  if (ptr > filename) {
    if (ptr[-1] != '/' && ptr < end) *ptr++ = '/';

    strlcpy(ptr, program, end - ptr + 1);

    if (!access(filename, X_OK)) return filename;
  }

  return 0;
}
#endif // !WIN32 && !__APPLE__


#ifndef WIN32
// Run the specified program, returning 1 on success and 0 on failure
static int
run_program(const char *program, char **argv, char *msg, int msglen) {
  pid_t	pid;				// Process ID of first child
  int status;				// Exit status from first child
  sigset_t set, oldset;			// Signal masks


  // Block SIGCHLD while we run the program...
  //
  // Note that I only use the POSIX signal APIs, however older operating
  // systems may either not support POSIX signals or have side effects.
  // IRIX, for example, provides three separate and incompatible signal
  // APIs, so it is possible that an application setting a signal handler
  // via signal() or sigset() will not have its SIGCHLD signals blocked...

  sigemptyset(&set);
  sigaddset(&set, SIGCHLD);
  sigprocmask(SIG_BLOCK, &set, &oldset);

  // Create child processes that actually run the program for us...
  if ((pid = fork()) == 0) {
    // First child comes here, fork a second child and exit...
    if (!fork()) {
      // Second child comes here, redirect stdin/out/err to /dev/null...
      close(0);
      open("/dev/null", O_RDONLY);

      close(1);
      open("/dev/null", O_WRONLY);

      close(2);
      open("/dev/null", O_WRONLY);

      // Detach from the current process group...
      setsid();

      // Run the program...
      execv(program, argv);
      _exit(0);
    } else {
      // First child gets here, exit immediately...
      _exit(0);
    }
  } else if (pid < 0) {
    // Restore signal handling...
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    // Return indicating failure...
    return 0;
  }

  // Wait for the first child to exit...
  while (waitpid(pid, &status, 0) < 0) {
    if (errno != EINTR) {
      // Someone else grabbed the child status...
      if (msg) snprintf(msg, msglen, "waitpid(%ld) failed: %s", (long)pid,
                        strerror(errno));

      // Restore signal handling...
      sigprocmask(SIG_SETMASK, &oldset, NULL);

      // Return indicating failure...
      return 0;
    }
  }

  // Restore signal handling...
  sigprocmask(SIG_SETMASK, &oldset, NULL);

  // Return indicating success...
  return 1;
}
#endif // !WIN32


#ifdef TEST
//
// Test code...
//

// Open the URI on the command-line...
int main(int argc, char **argv) {
  char msg[1024];


  if (argc != 2) {
    puts("Usage: fl_open_uri URI");
    return 1;
  }

  if (!fl_open_uri(argv[1], msg, sizeof(msg))) {
    puts(msg);
    return 1;
  } else return 0;
}
#endif // TEST


//
// End of "$Id: fl_open_uri.cxx 5636 2007-01-23 20:45:28Z mike $".
//
