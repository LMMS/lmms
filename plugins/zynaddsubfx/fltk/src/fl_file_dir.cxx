//
// "$Id: fl_file_dir.cxx 5417 2006-09-05 09:57:41Z matt $"
//
// File chooser widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

#include "flstring.h"
#include <FL/filename.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/fl_ask.H>


static Fl_File_Chooser	*fc = (Fl_File_Chooser *)0;
static void		(*current_callback)(const char*) = 0;
static const char	*current_label = fl_ok;


// Do a file chooser callback...
static void callback(Fl_File_Chooser *, void*) {
  if (current_callback && fc->value())
    (*current_callback)(fc->value());
}


// Set the file chooser callback
void fl_file_chooser_callback(void (*cb)(const char*)) {
  current_callback = cb;
}


// Set the "OK" button label
void fl_file_chooser_ok_label(const char *l) {
  if (l) current_label = l;
  else current_label = fl_ok;
}


//
// 'fl_file_chooser()' - Show a file chooser dialog and get a filename.
//

char *					// O - Filename or NULL
fl_file_chooser(const char *message,	// I - Message in titlebar
                const char *pat,	// I - Filename pattern
		const char *fname,	// I - Initial filename selection
		int        relative) {	// I - 0 for absolute path
  static char	retname[1024];		// Returned filename

  if (!fc) {
    if (!fname || !*fname) fname = ".";

    fc = new Fl_File_Chooser(fname, pat, Fl_File_Chooser::CREATE, message);
    fc->callback(callback, 0);
  } else {
    fc->type(Fl_File_Chooser::CREATE);
    // see, if we use the same pattern between calls
    char same_pattern = 0;
    const char *fcf = fc->filter();
    if ( fcf && pat && strcmp(fcf, pat)==0)
      same_pattern = 1;
    else if ( (fcf==0L || *fcf==0) && (pat==0L || *pat==0) )
      same_pattern = 1;
    // now set the pattern to the new pattern (even if they are the same)
    fc->filter(pat);
    fc->label(message);

    if (!fname) { // null pointer reuses same filename if pattern didn't change
      if (!same_pattern && fc->value()) {
	// if pattern is different, remove name but leave old directory:
	strlcpy(retname, fc->value(), sizeof(retname));

	char *p = strrchr(retname, '/');

        if (p) {
	  // If the filename is "/foo", then the directory will be "/", not
	  // ""...
	  if (p == retname)
	    retname[1] = '\0';
	  else
	    *p = '\0';
	}
	// Set the directory...
	fc->value(retname);
      } else {
        // re-use the previously selected name
      }
    } else if (!*fname) { // empty filename reuses directory with empty name
      const char *fcv = fc->value();
      if (fcv) 
        strlcpy(retname, fc->value(), sizeof(retname));
      else 
        *retname = 0;
      const char *n = fl_filename_name(retname);
      if (n) *((char*)n) = 0;
      fc->value("");
      fc->directory(retname);
    } else {
       fc->value(fname);
    }
  }

  fc->ok_label(current_label);
  fc->show();

  while (fc->shown())
    Fl::wait();

  if (fc->value() && relative) {
    fl_filename_relative(retname, sizeof(retname), fc->value());

    return retname;
  } else if (fc->value()) return (char *)fc->value();
  else return 0;
}


//
// 'fl_dir_chooser()' - Show a file chooser dialog and get a directory.
//

char *					// O - Directory or NULL
fl_dir_chooser(const char *message,	// I - Message for titlebar
               const char *fname,	// I - Initial directory name
	       int        relative)	// I - 0 for absolute
{
  static char	retname[1024];		// Returned directory name

  if (!fc) {
    if (!fname || !*fname) fname = ".";

    fc = new Fl_File_Chooser(fname, "*", Fl_File_Chooser::CREATE |
                                         Fl_File_Chooser::DIRECTORY, message);
    fc->callback(callback, 0);
  } else {
    fc->type(Fl_File_Chooser::CREATE | Fl_File_Chooser::DIRECTORY);
    fc->filter("*");
    if (fname && *fname) fc->value(fname);
    fc->label(message);
  }

  fc->show();

  while (fc->shown())
    Fl::wait();

  if (fc->value() && relative) {
    fl_filename_relative(retname, sizeof(retname), fc->value());

    return retname;
  } else if (fc->value()) return (char *)fc->value();
  else return 0;
}


//
// End of "$Id: fl_file_dir.cxx 5417 2006-09-05 09:57:41Z matt $".
//
