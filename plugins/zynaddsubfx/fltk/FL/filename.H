/*
 * "$Id: filename.H 8073 2010-12-20 13:35:43Z ianmacarthur $"
 *
 * Filename header file for the Fast Light Tool Kit (FLTK).
 *
 * Copyright 1998-2010 by Bill Spitzak and others.
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

/* Xcode on OS X includes files by recursing down into directories.
 * This code catches the cycle and directly includes the required file.
 */
#ifdef fl_dirent_h_cyclic_include
#  include "/usr/include/dirent.h"
#endif

#ifndef FL_FILENAME_H
#  define FL_FILENAME_H

#  include "Fl_Export.H"

/** \addtogroup filenames File names and URI utility functions
 File names and URI functions defined in <FL/filename.H>
    @{ */

#  define FL_PATH_MAX 2048 /**< all path buffers should use this length */
/** Gets the file name from a path.
    Similar to basename(3), exceptions shown below.
    \code
    #include <FL/filename.H>
    [..]
    const char *out;
    out = fl_filename_name("/usr/lib");     // out="lib"
    out = fl_filename_name("/usr/");        // out=""      (basename(3) returns "usr" instead)
    out = fl_filename_name("/usr");         // out="usr"
    out = fl_filename_name("/");            // out=""      (basename(3) returns "/" instead)
    out = fl_filename_name(".");            // out="."
    out = fl_filename_name("..");           // out=".."
    \endcode
    \return a pointer to the char after the last slash, or to \p filename if there is none.
 */
FL_EXPORT const char *fl_filename_name(const char * filename);
FL_EXPORT const char *fl_filename_ext(const char *buf);
FL_EXPORT char *fl_filename_setext(char *to, int tolen, const char *ext);
FL_EXPORT int fl_filename_expand(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_absolute(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_relative(char *to, int tolen, const char *from);
FL_EXPORT int fl_filename_match(const char *name, const char *pattern);
FL_EXPORT int fl_filename_isdir(const char *name);

#  if defined(__cplusplus) && !defined(FL_DOXYGEN)
/*
 * Under WIN32, we include filename.H from numericsort.c; this should probably change...
 */

inline char *fl_filename_setext(char *to, const char *ext) { return fl_filename_setext(to, FL_PATH_MAX, ext); }
inline int fl_filename_expand(char *to, const char *from) { return fl_filename_expand(to, FL_PATH_MAX, from); }
inline int fl_filename_absolute(char *to, const char *from) { return fl_filename_absolute(to, FL_PATH_MAX, from); }
FL_EXPORT int fl_filename_relative(char *to, int tolen, const char *from, const char *cwd);
inline int fl_filename_relative(char *to, const char *from) { return fl_filename_relative(to, FL_PATH_MAX, from); }
#  endif /* __cplusplus */


#  if defined(WIN32) && !defined(__CYGWIN__) && !defined(__WATCOMC__)

struct dirent {char d_name[1];};

#  elif defined(__WATCOMC__)
#    include <sys/types.h>
#    include <direct.h>

#  else
/*
 * WARNING: on some systems (very few nowadays?) <dirent.h> may not exist.
 * The correct information is in one of these files:
 *
 *     #include <sys/ndir.h>
 *     #include <sys/dir.h>
 *     #include <ndir.h>
 *
 * plus you must do the following #define:
 *
 *     #define dirent direct
 *
 * It would be best to create a <dirent.h> file that does this...
 */
#    include <sys/types.h>
#    define fl_dirent_h_cyclic_include
#    include <dirent.h>
#    undef fl_dirent_h_cyclic_include
#  endif

#  if defined (__cplusplus)
extern "C" {
#  endif /* __cplusplus */

#  if !defined(FL_DOXYGEN)
FL_EXPORT int fl_alphasort(struct dirent **, struct dirent **);
FL_EXPORT int fl_casealphasort(struct dirent **, struct dirent **);
FL_EXPORT int fl_casenumericsort(struct dirent **, struct dirent **);
FL_EXPORT int fl_numericsort(struct dirent **, struct dirent **);
#  endif

  typedef int (Fl_File_Sort_F)(struct dirent **, struct dirent **); /**< File sorting function. \see fl_filename_list() */

#  if defined(__cplusplus)
}

/*
 * Portable "scandir" function.  Ugly but necessary...
 */

FL_EXPORT int fl_filename_list(const char *d, struct dirent ***l,
                               Fl_File_Sort_F *s = fl_numericsort);
FL_EXPORT void fl_filename_free_list(struct dirent ***l, int n);

/*
 * Generic function to open a Uniform Resource Identifier (URI) using a
 * system-defined program (added in FLTK 1.1.8)
 */

FL_EXPORT int	fl_open_uri(const char *uri, char *msg = (char *)0,
		            int msglen = 0);

#    ifndef FL_DOXYGEN
/*
 * _fl_filename_isdir_quick() is a private function that checks for a
 * trailing slash and assumes that the passed name is a directory if
 * it finds one.  This function is used by Fl_File_Browser and
 * Fl_File_Chooser to avoid extra stat() calls, but is not supported
 * outside of FLTK...
 */
int _fl_filename_isdir_quick(const char *name);
#    endif

#  endif /* __cplusplus */

/*
 * FLTK 1.0.x compatibility definitions...
 */

#  ifdef FLTK_1_0_COMPAT
#    define filename_absolute	fl_filename_absolute
#    define filename_expand	fl_filename_expand
#    define filename_ext	fl_filename_ext
#    define filename_isdir	fl_filename_isdir
#    define filename_list	fl_filename_list
#    define filename_match	fl_filename_match
#    define filename_name	fl_filename_name
#    define filename_relative	fl_filename_relative
#    define filename_setext	fl_filename_setext
#    define numericsort		fl_numericsort
#  endif /* FLTK_1_0_COMPAT */


#endif /* FL_FILENAME_H */

/** @} */

/*
 * End of "$Id: filename.H 8073 2010-12-20 13:35:43Z ianmacarthur $".
 */
