/*
 * "$Id: fl_call_main.c 5848 2007-05-20 16:18:31Z mike $"
 *
 * Copyright 1998-2005 by Bill Spitzak and others.
 *
 * fl_call_main() calls main() for you Windows people.  Needs to be done in C
 * because Borland C++ won't let you call main() from C++.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

/*
 * This WinMain() function can be overridden by an application and
 * is provided for compatibility with programs written for other
 * operating systems that conform to the ANSI standard entry point
 * "main()".  This will allow you to build a WIN32 Application
 * without any special settings.
 *
 * Because of problems with the Microsoft Visual C++ header files
 * and/or compiler, you cannot have a WinMain function in a DLL.
 * I don't know why.  Thus, this nifty feature is only available
 * if you link to the static library.
 *
 * Currently the debug version of this library will create a
 * console window for your application so you can put printf()
 * statements for debugging or informational purposes.  Ultimately
 * we want to update this to always use the parent's console,
 * but at present we have not identified a function or API in
 * Microsoft(r) Windows(r) that allows for it.
 */

#if defined(WIN32) && !defined(FL_DLL) && !defined (__GNUC__)

#  include <windows.h>
#  include <stdio.h>
#  include <stdlib.h>

#  ifdef __MWERKS__
#   include <crtl.h>
#  endif

extern int main(int, char *[]);

#  ifdef BORLAND5
#    define __argc _argc
#    define __argv _argv
#  endif /* BORLAND5 */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             LPSTR lpCmdLine, int nCmdShow) {
  int rc;

#  ifdef _DEBUG
 /*
  * If we are using compiling in debug mode, open a console window so
  * we can see any printf's, etc...
  *
  * While we can detect if the program was run from the command-line -
  * look at the CMDLINE environment variable, it will be "WIN" for
  * programs started from the GUI - the shell seems to run all WIN32
  * applications in the background anyways...
  */

  AllocConsole();
  freopen("conin$", "r", stdin);
  freopen("conout$", "w", stdout);
  freopen("conout$", "w", stderr);
#  endif /* _DEBUG */

  /* Run the standard main entry point function... */
  rc = main(__argc, __argv);

#  ifdef _DEBUG
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
#  endif /* _DEBUG */

  return rc;
}

#elif defined(__hpux)
/* This code to prevent "empty translation unit" or similar warnings... */
static void dummy(void) {}
#endif // WIN32 && !FL_DLL && !__GNUC__

/*
 * End of "$Id: fl_call_main.c 5848 2007-05-20 16:18:31Z mike $".
 */

