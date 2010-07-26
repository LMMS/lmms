//
// "$Id: Fl_Native_File_Chooser_FLTK.H 7007 2010-01-14 23:08:06Z greg.ercolano $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2009 by Bill Spitzak and others.
// Copyright 2005 by Nathan Vander Wilt.
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

/* \file
   Fl_Native_File_Chooser widget. */

#include <FL/Fl_File_Chooser.H>
#include <unistd.h>		// _POSIX_NAME_MAX

/**
  This class lets an FLTK application easily and consistently access 
  the operating system's native file chooser. Some operating systems 
  have very complex and specific file choosers that many users want 
  access to specifically, instead of FLTK's default file chooser(s). 
  <P>
  In cases where there is no native file browser, FLTK's own file browser
  is used instead.
  <P>
  \code
  // Create and post the local native file chooser
  #include <FL/Fl_Native_File_Chooser.H>
  [..]
  Fl_Native_File_Chooser fnfc;
  fnfc.title("Pick a file");
  fnfc.type(Fl_Native_File_Chooser::BROWSE_FILE);
  fnfc.filter("Text\t*.txt\n"
              "C Files\t*.{cxx,h,c}");
  fnfc.directory("/var/tmp");
  // Show native chooser
  switch ( fnfc.show() ) {
    case -1: printf("ERROR: %s\n", fnfc.errmsg());    break;  // ERROR
    case  1: printf("CANCEL\n");                      break;  // CANCEL
    default: printf("PICKED: %s\n", fnfc.filename()); break;  // FILE CHOSEN
  }
  \endcode
  <P>
  <B>Platform Specific Caveats</B>
  <P>
  - Under X windows, it's best if you call Fl_File_Icon::load_system_icons()
    at the start of main(), to enable the nicer looking file browser widgets.
  - Some operating systems support certain OS specific options; see 
    Fl_Native_File_Chooser::options() for a list.
*/
class Fl_Native_File_Chooser {
public:
  enum Type {
    BROWSE_FILE = 0,			///< browse files (lets user choose one file)
    BROWSE_DIRECTORY,			///< browse directories (lets user choose one directory)
    BROWSE_MULTI_FILE,			///< browse files (lets user choose multiple files)
    BROWSE_MULTI_DIRECTORY,		///< browse directories (lets user choose multiple directories)
    BROWSE_SAVE_FILE,			///< browse to save a file
    BROWSE_SAVE_DIRECTORY		///< browse to save a directory
  };
  enum Option {
    NO_OPTIONS     = 0x0000,		///< no options enabled
    SAVEAS_CONFIRM = 0x0001,		///< Show native 'Save As' overwrite confirm dialog (if supported)
    NEW_FOLDER     = 0x0002,		///< Show 'New Folder' icon (if supported)
    PREVIEW        = 0x0004,		///< enable preview mode
  };
private:
  int   _btype;			// kind-of browser to show()
  int   _options;		// general options
  int   _nfilters;
  char *_filter;		// user supplied filter
  char *_parsedfilt;		// parsed filter
  int   _filtvalue;		// selected filter
  char *_preset_file;
  char *_prevvalue;		// Returned filename
  char *_directory;
  char *_errmsg;		// error message
  char *_old_dir;
  Fl_File_Chooser *_file_chooser;

  // added by MG
  Fl_File_Browser *my_fileList;
  Fl_Check_Button *show_hidden;
  int prev_filtervalue;
  static void show_hidden_cb(Fl_Check_Button *o, void *data);
  static void remove_hidden_files(Fl_File_Browser *my_fileList);

  // Private methods
  void errmsg(const char *msg);
  int type_fl_file(int);
  void parse_filter();
  void keeplocation();
  int exist_dialog();

public:
  Fl_Native_File_Chooser(int val=BROWSE_FILE);
  ~Fl_Native_File_Chooser();

  // Public methods
  void type(int);
  int type() const;
  void options(int);
  int options() const;
  int count() const;
  const char *filename() const;
  const char *filename(int i) const;
  void directory(const char *val);
  const char *directory() const;
  void title(const char *);
  const char* title() const;
  const char *filter() const;
  void filter(const char *);
  int filters() const;
  void filter_value(int i);
  int filter_value() const;
  void preset_file(const char*);
  const char* preset_file() const;
  const char *errmsg() const;
  int show();
};

//
// End of "$Id: Fl_Native_File_Chooser_FLTK.H 7007 2010-01-14 23:08:06Z greg.ercolano $".
//
