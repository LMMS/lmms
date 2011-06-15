//
// "$Id: filename_absolute.cxx 8146 2010-12-31 22:13:07Z matt $"
//
// Filename expansion routines for the Fast Light Tool Kit (FLTK).
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

/* expand a file name by prepending current directory, deleting . and
   .. (not really correct for symbolic links) between the prepended
   current directory.  Use $PWD if it exists.
   Returns true if any changes were made.
*/

#include <FL/filename.H>
#include <FL/fl_utf8.h>
#include <stdlib.h>
#include "flstring.h"
#include <ctype.h>
#if defined(WIN32) && !defined(__CYGWIN__)
# include <direct.h>
#else
#  include <unistd.h>
#endif

#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
inline int isdirsep(char c) {return c=='/' || c=='\\';}
#else
#define isdirsep(c) ((c)=='/')
#endif

/** Makes a filename absolute from a relative filename.
    \code
    #include <FL/filename.H>
    [..]
    chdir("/var/tmp");
    fl_filename_absolute(out, sizeof(out), "foo.txt");         // out="/var/tmp/foo.txt"
    fl_filename_absolute(out, sizeof(out), "./foo.txt");       // out="/var/tmp/foo.txt"
    fl_filename_absolute(out, sizeof(out), "../log/messages"); // out="/var/log/messages"
    \endcode
    \param[out] to resulting absolute filename
    \param[in]  tolen size of the absolute filename buffer 
    \param[in]  from relative filename
    \return 0 if no change, non zero otherwise
 */
int fl_filename_absolute(char *to, int tolen, const char *from) {
  if (isdirsep(*from) || *from == '|'
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
      || from[1]==':'
#endif
      ) {
    strlcpy(to, from, tolen);
    return 0;
  }

  char *a;
  char *temp = new char[tolen];
  const char *start = from;

  a = fl_getcwd(temp, tolen);
  if (!a) {
    strlcpy(to, from, tolen);
    delete[] temp;
    return 0;
  }
#if defined(WIN32) || defined(__EMX__) && !defined(__CYGWIN__)
  for (a = temp; *a; a++) if (*a=='\\') *a = '/'; // ha ha
#else
  a = temp+strlen(temp);
#endif
  if (isdirsep(*(a-1))) a--;
  /* remove intermediate . and .. names: */
  while (*start == '.') {
    if (start[1]=='.' && isdirsep(start[2])) {
      char *b;
      for (b = a-1; b >= temp && !isdirsep(*b); b--);
      if (b < temp) break;
      a = b;
      start += 3;
    } else if (isdirsep(start[1])) {
      start += 2;
    } else if (!start[1]) {
      start ++; // Skip lone "."
      break;
    } else
      break;
  }

  *a++ = '/';
  strlcpy(a,start,tolen - (a - temp));

  strlcpy(to, temp, tolen);

  delete[] temp;

  return 1;
}

/** Makes a filename relative to the current working directory.
    \code
    #include <FL/filename.H>
    [..]
    chdir("/var/tmp/somedir");       // set cwd to /var/tmp/somedir
    [..]
    char out[FL_PATH_MAX];
    fl_filename_relative(out, sizeof(out), "/var/tmp/somedir/foo.txt");  // out="foo.txt",    return=1
    fl_filename_relative(out, sizeof(out), "/var/tmp/foo.txt");          // out="../foo.txt", return=1
    fl_filename_relative(out, sizeof(out), "foo.txt");                   // out="foo.txt",    return=0 (no change)
    fl_filename_relative(out, sizeof(out), "./foo.txt");                 // out="./foo.txt",  return=0 (no change)
    fl_filename_relative(out, sizeof(out), "../foo.txt");                // out="../foo.txt", return=0 (no change)
    \endcode
    \param[out] to resulting relative filename
    \param[in]  tolen size of the relative filename buffer 
    \param[in]  from absolute filename
    \return 0 if no change, non zero otherwise
 */
int					// O - 0 if no change, 1 if changed
fl_filename_relative(char       *to,	// O - Relative filename
                     int        tolen,	// I - Size of "to" buffer
                     const char *from)  // I - Absolute filename
{
  char cwd_buf[FL_PATH_MAX];	// Current directory
  // get the current directory and return if we can't
  if (!fl_getcwd(cwd_buf, sizeof(cwd_buf))) {
    strlcpy(to, from, tolen);
    return 0;
  }
  return fl_filename_relative(to, tolen, from, cwd_buf);
}


/** Makes a filename relative to any other directory.
 \param[out] to resulting relative filename
 \param[in]  tolen size of the relative filename buffer 
 \param[in]  from absolute filename
 \param[in]  base relative to this absolute path
 \return 0 if no change, non zero otherwise
 */
int					// O - 0 if no change, 1 if changed
fl_filename_relative(char       *to,	// O - Relative filename
                     int        tolen,	// I - Size of "to" buffer
                     const char *from,  // I - Absolute filename
                     const char *base) { // I - Find path relative to this path
  
  char          *newslash;		// Directory separator
  const char	*slash;			// Directory separator
  char          *cwd = 0L, *cwd_buf = 0L;
  if (base) cwd = cwd_buf = strdup(base);
  
  // return if "from" is not an absolute path
#if defined(WIN32) || defined(__EMX__)
  if (from[0] == '\0' ||
      (!isdirsep(*from) && !isalpha(*from) && from[1] != ':' &&
       !isdirsep(from[2]))) {
#else
  if (from[0] == '\0' || !isdirsep(*from)) {
#endif // WIN32 || __EMX__
    strlcpy(to, from, tolen);
    if (cwd_buf) free(cwd_buf);
    return 0;
  }
        
  // return if "cwd" is not an absolute path
#if defined(WIN32) || defined(__EMX__)
  if (!cwd || cwd[0] == '\0' ||
      (!isdirsep(*cwd) && !isalpha(*cwd) && cwd[1] != ':' &&
       !isdirsep(cwd[2]))) {
#else
  if (!cwd || cwd[0] == '\0' || !isdirsep(*cwd)) {
#endif // WIN32 || __EMX__
    strlcpy(to, from, tolen);
    if (cwd_buf) free(cwd_buf);
    return 0;
  }
              
#if defined(WIN32) || defined(__EMX__)
  // convert all backslashes into forward slashes
  for (newslash = strchr(cwd, '\\'); newslash; newslash = strchr(newslash + 1, '\\'))
    *newslash = '/';

  // test for the exact same string and return "." if so
  if (!strcasecmp(from, cwd)) {
    strlcpy(to, ".", tolen);
    free(cwd_buf);
    return (1);
  }

  // test for the same drive. Return the absolute path if not
  if (tolower(*from & 255) != tolower(*cwd & 255)) {
    // Not the same drive...
    strlcpy(to, from, tolen);
    free(cwd_buf);
    return 0;
  }

  // compare the path name without the drive prefix
  from += 2; cwd += 2;
#else
  // test for the exact same string and return "." if so
  if (!strcmp(from, cwd)) {
    strlcpy(to, ".", tolen);
    free(cwd_buf);
    return (1);
  }
#endif // WIN32 || __EMX__

  // compare both path names until we find a difference
  for (slash = from, newslash = cwd;
      *slash != '\0' && *newslash != '\0';
       slash ++, newslash ++)
    if (isdirsep(*slash) && isdirsep(*newslash)) continue;
#if defined(WIN32) || defined(__EMX__) || defined(__APPLE__)
    else if (tolower(*slash & 255) != tolower(*newslash & 255)) break;
#else
    else if (*slash != *newslash) break;
#endif // WIN32 || __EMX__ || __APPLE__

  // skip over trailing slashes
  if ( *newslash == '\0' && *slash != '\0' && !isdirsep(*slash)
     &&(newslash==cwd || !isdirsep(newslash[-1])) )
    newslash--;

  // now go back to the first character of the first differing paths segment
  while (!isdirsep(*slash) && slash > from) slash --;
  if (isdirsep(*slash)) slash ++;

  // do the same for the current dir
  if (isdirsep(*newslash)) newslash --;
  if (*newslash != '\0')
    while (!isdirsep(*newslash) && newslash > cwd) newslash --;

  // prepare the destination buffer
  to[0]         = '\0';
  to[tolen - 1] = '\0';

  // now add a "previous dir" sequence for every following slash in the cwd
  while (*newslash != '\0') {
    if (isdirsep(*newslash)) strlcat(to, "../", tolen);

    newslash ++;
  }

  // finally add the differing path from "from"
  strlcat(to, slash, tolen);

  free(cwd_buf);
  return 1;
}


//
// End of "$Id: filename_absolute.cxx 8146 2010-12-31 22:13:07Z matt $".
//
