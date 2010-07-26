//
// "$Id: Fl_Native_File_Chooser_MAC.H 7354 2010-03-29 11:07:29Z matt $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2009 by Bill Spitzak and others.
// Copyright 2004 Greg Ercolano.
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

#ifndef FL_DOXYGEN		// PREVENT DOXYGEN'S USE OF THIS FILE

// OSX-SPECIFIC NATIVE BROWSER
#include <Carbon/Carbon.h>
#include <config.h>

#undef check // necessary for use of Fl::check()

#include <FL/filename.H>
#define MAXFILTERS	80

class Fl_Native_File_Chooser {
public:
  enum Type {
    BROWSE_FILE = 0,
    BROWSE_DIRECTORY,
    BROWSE_MULTI_FILE,
    BROWSE_MULTI_DIRECTORY,
    BROWSE_SAVE_FILE,
    BROWSE_SAVE_DIRECTORY
  };
  enum Option {
    NO_OPTIONS     = 0x0000,		// no options enabled
    SAVEAS_CONFIRM = 0x0001,		// Show native 'Save As' overwrite confirm dialog (if supported)
    NEW_FOLDER     = 0x0002,		// Show 'New Folder' icon (if supported)
    PREVIEW        = 0x0004,		// enable preview mode
  };
private:
  int             _btype;		// kind-of browser to show()
  int             _options;		// general options
  void 	         *_panel;
  char          **_pathnames;		// array of pathnames
  int             _tpathnames;	        // total pathnames
  char           *_directory;		// default pathname to use
  char           *_title;		// title for window
  char           *_preset_file;	        // the 'save as' filename

  char           *_filter;		// user-side search filter, eg:
					// C Files\t*.[ch]\nText Files\t*.txt"

  char           *_filt_names;		// filter names (tab delimited)
					// eg. "C Files\tText Files"

  char           *_filt_patt[MAXFILTERS];
					// array of filter patterns, eg:
					//     _filt_patt[0]="*.{cxx,h}"
					//     _filt_patt[1]="*.txt"

  int             _filt_total;		// parse_filter() # of filters loaded
  int             _filt_value;		// index of the selected filter
  char           *_errmsg;		// error message

  // Private methods
  void errmsg(const char *msg);
  void clear_pathnames();
  void set_single_pathname(const char *s);
  int get_saveas_basename(void);
  void clear_filters();
  void add_filter(const char *, const char *);
  void parse_filter(const char *from);
  int post();

public:
  Fl_Native_File_Chooser(int val = BROWSE_FILE);
  ~Fl_Native_File_Chooser();

  // Public methods
  void type(int);
  int type() const;
  void options(int);
  int options() const;
  int count() const;
  const char *filename() const;
  const char *filename(int i) const;
  void directory(const char *);
  const char *directory() const;
  void title(const char *);
  const char *title() const;
  const char *filter() const;
  void filter(const char *);
  void filter_value(int i) { _filt_value = i; }
  int filter_value() { return(_filt_value); }
  int filters() { return(_filt_total); }
  void preset_file(const char *);
  const char *preset_file();
  const char *errmsg() const;
  int show();
};

#endif /*!FL_DOXYGEN*/

//
// End of "$Id: Fl_Native_File_Chooser_MAC.H 7354 2010-03-29 11:07:29Z matt $".
//
