//
// "$Id: Fl_Native_File_Chooser_WIN32.H 7003 2010-01-14 20:47:59Z greg.ercolano $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2005 by Bill Spitzak and others.
// Copyright 2004 by Greg Ercolano.
// April 2005 - API changes, improved filter processing by Nathan Vander Wilt
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
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//

#ifndef FL_DOXYGEN		// PREVENT DOXYGEN'S USE OF THIS FILE

// #define _WIN32_WINNT	0x0501	// needed for OPENFILENAME's 'FlagsEx'
#include <stdio.h>
#include <stdlib.h>		// malloc
#include <windows.h>
#include <commdlg.h>		// OPENFILENAME, GetOpenFileName()
#include <shlobj.h>		// BROWSEINFO, SHBrowseForFolder()

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
    NO_OPTIONS     = 0x0000,	// no options enabled
    SAVEAS_CONFIRM = 0x0001,	// Show native 'Save As' overwrite confirm dialog (if supported)
    NEW_FOLDER     = 0x0002,	// Show 'New Folder' icon (if supported)
    PREVIEW        = 0x0004,	// enable preview mode
  };
private:
  int  _btype;			// kind-of browser to show()
  int  _options;		// general options
  OPENFILENAMEW _ofn;		// GetOpenFileName() & GetSaveFileName() struct
  BROWSEINFO   _binf;		// SHBrowseForFolder() struct
  char  **_pathnames;		// array of pathnames
  int     _tpathnames;		// total pathnames
  char   *_directory;		// default pathname to use
  char   *_title;		// title for window
  char   *_filter;		// user-side search filter
  char   *_parsedfilt;		// filter parsed for Windows dialog
  int     _nfilters;		// number of filters parse_filter counted
  char   *_preset_file;		// the file to preselect
  char   *_errmsg;		// error message

  // Private methods
  void errmsg(const char *msg);

  void clear_pathnames();
  void set_single_pathname(const char *s);
  void add_pathname(const char *s);

  void FreePIDL(ITEMIDLIST *pidl);
  void ClearOFN();
  void ClearBINF();
  void Win2Unix(char *s);
  void Unix2Win(char *s);
  int showfile();
  static int CALLBACK Dir_CB(HWND win, UINT msg, LPARAM param, LPARAM data);
  int showdir();

  void parse_filter(const char *);
  void clear_filters();
  void add_filter(const char *, const char *);

public:
  Fl_Native_File_Chooser(int val = BROWSE_FILE);
  ~Fl_Native_File_Chooser();

  // Public methods
  void type(int val);
  int type() const;
  void options(int);
  int options() const;
  int count() const;
  const char *filename() const;
  const char *filename(int i) const;
  void directory(const char *val);
  const char *directory() const;
  void title(const char *val);
  const char *title() const;
  const char *filter() const;
  void filter(const char *val);
  int filters() const { return _nfilters; }
  void filter_value(int i);
  int filter_value() const;
  void preset_file(const char *);
  const char *preset_file() const;
  const char *errmsg() const;
  int show();
};
#endif /*!FL_DOXYGEN*/

//
// End of "$Id: Fl_Native_File_Chooser_WIN32.H 7003 2010-01-14 20:47:59Z greg.ercolano $".
//
