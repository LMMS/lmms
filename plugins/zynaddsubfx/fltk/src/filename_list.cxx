//
// "$Id: filename_list.cxx 6986 2010-01-01 18:30:49Z greg.ercolano $"
//
// Filename list routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2009 by Bill Spitzak and others.
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

// Wrapper for scandir with const-correct function prototypes.

#include <FL/filename.H>
#include "flstring.h"
#include <stdlib.h>


extern "C" {
#ifndef HAVE_SCANDIR
  int fl_scandir (const char *dir, dirent ***namelist,
	          int (*select)(dirent *),
	          int (*compar)(dirent **, dirent **));
#  define scandir	fl_scandir
#endif
}

int fl_alphasort(struct dirent **a, struct dirent **b) {
  return strcmp((*a)->d_name, (*b)->d_name);
}

int fl_casealphasort(struct dirent **a, struct dirent **b) {
  return strcasecmp((*a)->d_name, (*b)->d_name);
}


/**
   Portable and const-correct wrapper for the scandir() function. 
   For each file in that directory a "dirent" structure is created. 
   The only portable thing about a dirent is that dirent.d_name is the nul-terminated file name. 
   An pointers array to these dirent's is created and a pointer to the array is returned in *list.
   The number of entries is given as a return value. 
   If there is an error reading the directory a number less than zero is returned, 
   and errno has the reason; errno does not work under WIN32. 

   \b Include:
   \code
   #include <FL/filename.H>
   \endcode

   \param[in] d the name of the directory to list.  It does not matter if it has a trailing slash.
   \param[out] list table containing the resulting directory listing
   \param[in] sort sorting functor:
    - fl_alphasort: The files are sorted in ascending alphabetical order; 
        upper and lowercase letters are compared according to their ASCII ordering  uppercase before lowercase.
    - fl_casealphasort: The files are sorted in ascending alphabetical order; 
        upper and lowercase letters are compared equally case is not significant.
    - fl_casenumericsort: The files are sorted in ascending "alphanumeric" order, where an attempt is made 
        to put unpadded numbers in consecutive order; upper and lowercase letters 
        are compared equally case is not significant.
    - fl_numericsort: The files are sorted in ascending "alphanumeric" order, where an attempt is made 
        to put unpadded numbers in consecutive order; upper and lowercase letters are compared 
        according to their ASCII ordering - uppercase before lowercase. 
   \return the number of entries if no error, a negative value otherwise.
*/
int fl_filename_list(const char *d, dirent ***list,
                     Fl_File_Sort_F *sort) {
#ifndef HAVE_SCANDIR
  int n = scandir(d, list, 0, sort);
#elif defined(HAVE_SCANDIR_POSIX)
  // HP-UX, Cygwin and POSIX (2008) define the comparison function like this:
  int n = scandir(d, list, 0, (int(*)(const dirent **, const dirent **))sort);
#elif defined(__osf__)
  // OSF, DU 4.0x
  int n = scandir(d, list, 0, (int(*)(dirent **, dirent **))sort);
#elif defined(_AIX)
  // AIX is almost standard...
  int n = scandir(d, list, 0, (int(*)(void*, void*))sort);
#elif !defined(__sgi)
  // The vast majority of UNIX systems want the sort function to have this
  // prototype, most likely so that it can be passed to qsort without any
  // changes:
  int n = scandir(d, list, 0, (int(*)(const void*,const void*))sort);
#else
  // This version is when we define our own scandir (WIN32 and perhaps
  // some Unix systems) and apparently on IRIX:
  int n = scandir(d, list, 0, sort);
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
  // we did this already during fl_scandir/win32
#else
  // append a '/' to all filenames that are directories
  int i, dirlen = strlen(d);
  char *fullname = (char*)malloc(dirlen+FL_PATH_MAX+3); // Add enough extra for two /'s and a nul
  // Use memcpy for speed since we already know the length of the string...
  memcpy(fullname, d, dirlen+1);
  char *name = fullname + dirlen;
  if (name!=fullname && name[-1]!='/') *name++ = '/';
  for (i=0; i<n; i++) {
    dirent *de = (*list)[i];
    int len = strlen(de->d_name);
    if (de->d_name[len-1]=='/' || len>FL_PATH_MAX) continue;
    // Use memcpy for speed since we already know the length of the string...
    memcpy(name, de->d_name, len+1);
    if (fl_filename_isdir(fullname)) {
      (*list)[i] = de = (dirent*)realloc(de, de->d_name - (char*)de + len + 2);
      char *dst = de->d_name + len;
      *dst++ = '/';
      *dst = 0;
    }
  }
  free(fullname);
#endif
  return n;
}

//
// End of "$Id: filename_list.cxx 6986 2010-01-01 18:30:49Z greg.ercolano $".
//
