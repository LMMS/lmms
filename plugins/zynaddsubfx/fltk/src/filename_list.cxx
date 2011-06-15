//
// "$Id: filename_list.cxx 8192 2011-01-05 16:50:10Z manolo $"
//
// Filename list routines for the Fast Light Tool Kit (FLTK).
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

// Wrapper for scandir with const-correct function prototypes.

#include <FL/filename.H>
#include <FL/fl_utf8.h>
#include "flstring.h"
#include <stdlib.h>


extern "C" {
#ifndef HAVE_SCANDIR
  int fl_scandir (const char *dir, dirent ***namelist,
	          int (*select)(dirent *),
	          int (*compar)(dirent **, dirent **));
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
#if defined(WIN32) && !defined(__CYGWIN__) && !defined(HAVE_SCANDIR)
  // For Windows we have a special scandir implementation that uses
  // the Win32 "wide" functions for lookup, avoiding the code page mess
  // entirely. It also fixes up the trailing '/'.
  return fl_scandir(d, list, 0, sort);

#else // WIN32

  int dirlen;
  char *dirloc;

  // Assume that locale encoding is no less dense than UTF-8
  dirlen = strlen(d);
#ifdef __APPLE__
  dirloc = (char *)d;
#else
  dirloc = (char *)malloc(dirlen + 1);
  fl_utf8to_mb(d, dirlen, dirloc, dirlen + 1);
#endif

#ifndef HAVE_SCANDIR
  // This version is when we define our own scandir
  int n = fl_scandir(dirloc, list, 0, sort);
#elif defined(HAVE_SCANDIR_POSIX) && !defined(__APPLE__)
  // POSIX (2008) defines the comparison function like this:
  int n = scandir(dirloc, list, 0, (int(*)(const dirent **, const dirent **))sort);
#elif defined(__osf__)
  // OSF, DU 4.0x
  int n = scandir(dirloc, list, 0, (int(*)(dirent **, dirent **))sort);
#elif defined(_AIX)
  // AIX is almost standard...
  int n = scandir(dirloc, list, 0, (int(*)(void*, void*))sort);
#elif defined(__sgi)
  int n = scandir(dirloc, list, 0, sort);
#else
  // The vast majority of UNIX systems want the sort function to have this
  // prototype, most likely so that it can be passed to qsort without any
  // changes:
  int n = scandir(dirloc, list, 0, (int(*)(const void*,const void*))sort);
#endif

#ifndef __APPLE__
  free(dirloc);
#endif

  // convert every filename to utf-8, and append a '/' to all
  // filenames that are directories
  int i;
  char *fullname = (char*)malloc(dirlen+FL_PATH_MAX+3); // Add enough extra for two /'s and a nul
  // Use memcpy for speed since we already know the length of the string...
  memcpy(fullname, d, dirlen+1);

  char *name = fullname + dirlen;
  if (name!=fullname && name[-1]!='/')
    *name++ = '/';

  for (i=0; i<n; i++) {
    int newlen;
    dirent *de = (*list)[i];
    int len = strlen(de->d_name);
#ifdef __APPLE__
    newlen = len;
#else
    newlen = fl_utf8from_mb(NULL, 0, de->d_name, len);
#endif
    dirent *newde = (dirent*)malloc(de->d_name - (char*)de + newlen + 2); // Add space for a / and a nul

    // Conversion to UTF-8
    memcpy(newde, de, de->d_name - (char*)de);
#ifdef __APPLE__
    strcpy(newde->d_name, de->d_name);
#else
    fl_utf8from_mb(newde->d_name, newlen + 1, de->d_name, len);
#endif

    // Check if dir (checks done on "old" name as we need to interact with
    // the underlying OS)
    if (de->d_name[len-1]!='/' && len<=FL_PATH_MAX) {
      // Use memcpy for speed since we already know the length of the string...
      memcpy(name, de->d_name, len+1);
      if (fl_filename_isdir(fullname)) {
        char *dst = newde->d_name + newlen;
        *dst++ = '/';
        *dst = 0;
      }
    }

    free(de);
    (*list)[i] = newde;
  }
  free(fullname);

  return n;

#endif // WIN32
}

/**
 \brief Free the list of filenames that is generated by fl_filename_list().
 
 Free everything that was allocated by a previous call to fl_filename_list().
 Use the return values as parameters for this function.
 
 \param[in,out] list table containing the resulting directory listing
 \param[in] n number of entries in the list
 */
void fl_filename_free_list(struct dirent ***list, int n)
{
  if (n<0) return;
  
  int i;
  for (i = 0; i < n; i ++) {
    if ((*list)[i])
      free((*list)[i]);
  }  
  free(*list);
  *list = 0;
}


//
// End of "$Id: filename_list.cxx 8192 2011-01-05 16:50:10Z manolo $".
//
