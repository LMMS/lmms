// "$Id: Fl_Native_File_Chooser_FLTK.cxx 8282 2011-01-16 18:26:51Z manolo $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2010 by Bill Spitzak and others.
// Copyright 2004 Greg Ercolano.
// API changes + filter improvements by Nathan Vander Wilt 2005
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

#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_File_Icon.H>
#define FLTK_CHOOSER_SINGLE    Fl_File_Chooser::SINGLE
#define FLTK_CHOOSER_DIRECTORY Fl_File_Chooser::DIRECTORY
#define FLTK_CHOOSER_MULTI     Fl_File_Chooser::MULTI
#define FLTK_CHOOSER_CREATE    Fl_File_Chooser::CREATE

#include "Fl_Native_File_Chooser_common.cxx"
#include <sys/stat.h>
#include <string.h>

/**
  The constructor. Internally allocates the native widgets.
  Optional \p val presets the type of browser this will be, 
  which can also be changed with type().
*/
Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
  //// CANT USE THIS -- MESSES UP LINKING/CREATES DEPENDENCY ON fltk_images.
  //// Have app call this from main() instead.
  ////
  ////  static int init = 0;		// 'first time' initialize flag
  ////  if ( init == 0 ) {
  ////    // Initialize when instanced for first time
  ////    Fl_File_Icon::load_system_icons();
  ////    init = 1;
  ////  }
  _btype       = val;
  _options     = NO_OPTIONS;
  _filter      = NULL;
  _filtvalue   = 0;
  _parsedfilt  = NULL;
  _preset_file = NULL;
  _prevvalue   = NULL;
  _directory   = NULL;
  _errmsg      = NULL;
  _file_chooser = new Fl_File_Chooser(NULL, NULL, 0, NULL);
  type(val);			// do this after _file_chooser created
  _nfilters    = 0;
} 

/**
  Destructor. 
  Deallocates any resources allocated to this widget.
*/
Fl_Native_File_Chooser::~Fl_Native_File_Chooser() {
  delete _file_chooser;
  _filter      = strfree(_filter);
  _parsedfilt  = strfree(_parsedfilt);
  _preset_file = strfree(_preset_file);
  _prevvalue   = strfree(_prevvalue);
  _directory   = strfree(_directory);
  _errmsg      = strfree(_errmsg);
}

// PRIVATE: SET ERROR MESSAGE
void Fl_Native_File_Chooser::errmsg(const char *msg) {
  _errmsg = strfree(_errmsg);
  _errmsg = strnew(msg);
}

// PRIVATE: translate Native types to Fl_File_Chooser types
int Fl_Native_File_Chooser::type_fl_file(int val) {
  switch (val) {
    case BROWSE_FILE:
      return(FLTK_CHOOSER_SINGLE);
    case BROWSE_DIRECTORY:
      return(FLTK_CHOOSER_SINGLE | FLTK_CHOOSER_DIRECTORY);
    case BROWSE_MULTI_FILE:
      return(FLTK_CHOOSER_MULTI);
    case BROWSE_MULTI_DIRECTORY:
      return(FLTK_CHOOSER_DIRECTORY | FLTK_CHOOSER_MULTI);
    case BROWSE_SAVE_FILE:
      return(FLTK_CHOOSER_SINGLE | FLTK_CHOOSER_CREATE);
    case BROWSE_SAVE_DIRECTORY:
      return(FLTK_CHOOSER_DIRECTORY | FLTK_CHOOSER_MULTI | FLTK_CHOOSER_CREATE);
    default:
      return(FLTK_CHOOSER_SINGLE);
  }
}

/**
  Sets the current Fl_Native_File_Chooser::Type of browser.
 */
void Fl_Native_File_Chooser::type(int val) {
  _btype = val;
  _file_chooser->type(type_fl_file(val));
}

/**
  Gets the current Fl_Native_File_Chooser::Type of browser.
 */
int Fl_Native_File_Chooser::type() const {
  return(_btype);
}

/**
  Sets the platform specific chooser options to \p val.
  \p val is expected to be one or more Fl_Native_File_Chooser::Option flags ORed together.
  Some platforms have OS-specific functions that can be enabled/disabled via this method.
  <P>
  \code
  Flag              Description                                       Win       Mac       Other
  --------------    -----------------------------------------------   -------   -------   -------
  NEW_FOLDER        Shows the 'New Folder' button.                    Ignored   Used      Used
  PREVIEW           Enables the 'Preview' mode by default.            Ignored   Ignored   Used
  SAVEAS_CONFIRM    Confirm dialog if BROWSE_SAVE_FILE file exists.   Ignored   Used      Used
  \endcode
*/
void Fl_Native_File_Chooser::options(int val) {
  _options = val;
}

/**
  Gets the platform specific Fl_Native_File_Chooser::Option flags.
*/
int Fl_Native_File_Chooser::options() const {
  return(_options);
}

/**
  Post the chooser's dialog. Blocks until dialog has been completed or cancelled.
  \returns
     - 0  -- user picked a file
     - 1  -- user cancelled
     - -1 -- failed; errmsg() has reason
*/
int Fl_Native_File_Chooser::show() {
  // FILTER
  if ( _parsedfilt ) {
    _file_chooser->filter(_parsedfilt);
  }

  // FILTER VALUE
  //     Set this /after/ setting the filter
  //
  _file_chooser->filter_value(_filtvalue);

  // DIRECTORY
  if ( _directory && _directory[0] ) {
    _file_chooser->directory(_directory);
  } else {
    _file_chooser->directory(_prevvalue);
  }

  // PRESET FILE
  if ( _preset_file ) {
    _file_chooser->value(_preset_file);
  }

  // OPTIONS: PREVIEW
  _file_chooser->preview( (options() & PREVIEW) ? 1 : 0);

  // OPTIONS: NEW FOLDER
  if ( options() & NEW_FOLDER )
    _file_chooser->type(_file_chooser->type() | FLTK_CHOOSER_CREATE);	// on

  // SHOW
  _file_chooser->show();

  // BLOCK WHILE BROWSER SHOWN
  while ( _file_chooser->shown() ) {
    Fl::wait();
  }

  if ( _file_chooser->value() && _file_chooser->value()[0] ) {
    _prevvalue = strfree(_prevvalue);
    _prevvalue = strnew(_file_chooser->value());
    _filtvalue = _file_chooser->filter_value();	// update filter value

    // HANDLE SHOWING 'SaveAs' CONFIRM
    if ( options() & SAVEAS_CONFIRM && type() == BROWSE_SAVE_FILE ) {
      struct stat buf;
      if ( stat(_file_chooser->value(), &buf) != -1 ) {
	if ( buf.st_mode & S_IFREG ) {		// Regular file + exists?
	  if ( exist_dialog() == 0 ) {
	    return(1);
	  }
	}
      }
    }
  }

  if ( _file_chooser->count() ) return(0);
  else return(1);
}

/**
  Returns a system dependent error message for the last method that failed. 
  This message should at least be flagged to the user in a dialog box, or to some kind of error log. 
  Contents will be valid only for methods that document errmsg() will have info on failures.
 */
const char *Fl_Native_File_Chooser::errmsg() const {
  return(_errmsg ? _errmsg : "No error");
}

/**
  Return the filename the user choose.
  Use this if only expecting a single filename.
  If more than one filename is expected, use filename(int) instead.
  Return value may be "" if no filename was chosen (eg. user cancelled).
 */
const char* Fl_Native_File_Chooser::filename() const {
  if ( _file_chooser->count() > 0 ) return(_file_chooser->value());
  return("");
}

/**
  Return one of the filenames the user selected.
  Use count() to determine how many filenames the user selected.
  <P>
  \b Example:
  \code
  if ( fnfc->show() == 0 ) {
    // Print all filenames user selected
    for (int n=0; n<fnfc->count(); n++ ) {
      printf("%d) '%s'\n", n, fnfc->filename(n));
    }
  }
  \endcode
 */
const char* Fl_Native_File_Chooser::filename(int i) const {
  if ( i < _file_chooser->count() )
    return(_file_chooser->value(i+1));	// convert fltk 1 based to our 0 based
  return("");
}

/**
  Set the title of the file chooser's dialog window.
  Can be NULL if no title desired.
  The default title varies according to the platform, so you are advised to set the title explicitly.
*/
void Fl_Native_File_Chooser::title(const char *val) {
  _file_chooser->label(val);
}

/**
  Get the title of the file chooser's dialog window.
  Return value may be NULL if no title was set.
*/
const char *Fl_Native_File_Chooser::title() const {
  return(_file_chooser->label());
}

/**
  Sets the filename filters used for browsing. 
  The default is NULL, which browses all files.
  <P>
  The filter string can be any of:
  <P>
    - A single wildcard (eg. "*.txt")
    - Multiple wildcards (eg. "*.{cxx,h,H}")
    - A descriptive name followed by a "\t" and a wildcard (eg. "Text Files\t*.txt")
    - A list of separate wildcards with a "\n" between each (eg. "*.{cxx,H}\n*.txt")
    - A list of descriptive names and wildcards (eg. "C++ Files\t*.{cxx,H}\nTxt Files\t*.txt")
  <P>
  The format of each filter is a wildcard, or an optional user description 
  followed by '\\t' and the wildcard.
  <P>
  On most platforms, each filter is available to the user via a pulldown menu 
  in the file chooser. The 'All Files' option is always available to the user. 
*/
void Fl_Native_File_Chooser::filter(const char *val) {
  _filter = strfree(_filter);
  _filter = strnew(val);
  parse_filter();
}

/**
  Returns the filter string last set.
  Can be NULL if no filter was set.
 */
const char *Fl_Native_File_Chooser::filter() const {
  return(_filter);
}

/**
Gets how many filters were available, not including "All Files" 
*/
int Fl_Native_File_Chooser::filters() const {
  return(_nfilters);
}

/**
  Sets which filter will be initially selected.

  The first filter is indexed as 0. 
  If filter_value()==filters(), then "All Files" was chosen. 
  If filter_value() > filters(), then a custom filter was set.
 */
void Fl_Native_File_Chooser::filter_value(int val) {
  _filtvalue = val;
}

/**
  Returns which filter value was last selected by the user.
  This is only valid if the chooser returns success.
 */
int Fl_Native_File_Chooser::filter_value() const {
  return(_filtvalue);
}

/**
  Returns the number of filenames (or directory names) the user selected.
  <P>
  \b Example:
  \code
  if ( fnfc->show() == 0 ) {
    // Print all filenames user selected
    for (int n=0; n<fnfc->count(); n++ ) {
      printf("%d) '%s'\n", n, fnfc->filename(n));
    }
  }
  \endcode
*/
int Fl_Native_File_Chooser::count() const {
  return(_file_chooser->count());
}

/**
  Preset the directory the browser will show when opened.
  If \p val is NULL, or no directory is specified, the chooser will attempt
  to use the last non-cancelled folder.
*/
void Fl_Native_File_Chooser::directory(const char *val) {
  _directory = strfree(_directory);
  _directory = strnew(val);
}

/**
  Returns the current preset directory() value.
*/
const char *Fl_Native_File_Chooser::directory() const {
  return(_directory);
}

// PRIVATE: Convert our filter format to fltk's chooser format
//     FROM                                     TO (FLTK)
//     -------------------------                --------------------------
//     "*.cxx"                                  "*.cxx Files(*.cxx)"
//     "C Files\t*.{cxx,h}"                     "C Files(*.{cxx,h})"
//     "C Files\t*.{cxx,h}\nText Files\t*.txt"  "C Files(*.{cxx,h})\tText Files(*.txt)"
//
//     Returns a modified version of the filter that the caller is responsible
//     for freeing with strfree().
//
void Fl_Native_File_Chooser::parse_filter() {
  _parsedfilt = strfree(_parsedfilt);	// clear previous parsed filter (if any)
  _nfilters = 0;
  char *in = _filter;
  if ( !in ) return;

  int has_name = strchr(in, '\t') ? 1 : 0;

  char mode = has_name ? 'n' : 'w';	// parse mode: n=title, w=wildcard
  char wildcard[1024] = "";		// parsed wildcard
  char name[1024] = "";

  // Parse filter user specified
  for ( ; 1; in++ ) {
    /*** DEBUG
    printf("WORKING ON '%c': mode=<%c> name=<%s> wildcard=<%s>\n",
			*in, mode,     name,     wildcard);
    ***/

    switch (*in) {
      // FINISHED PARSING NAME?
      case '\t':
        if ( mode != 'n' ) goto regchar;
        mode = 'w';
        break; 
      // ESCAPE NEXT CHAR
      case '\\':
	++in;
	goto regchar; 
      // FINISHED PARSING ONE OF POSSIBLY SEVERAL FILTERS?
      case '\r':
      case '\n':
      case '\0':
	// APPEND NEW FILTER TO LIST
	if ( wildcard[0] ) {
	  // OUT: "name(wild)\tname(wild)"
	  char comp[2048];
	  sprintf(comp, "%s%.511s(%.511s)", ((_parsedfilt)?"\t":""),
					    name, wildcard);
	  _parsedfilt = strapp(_parsedfilt, comp);
	  _nfilters++;
	  //DEBUG printf("DEBUG: PARSED FILT NOW <%s>\n", _parsedfilt);
	}
	// RESET
	wildcard[0] = name[0] = '\0';
	mode = strchr(in, '\t') ? 'n' : 'w';
	// DONE?
	if ( *in == '\0' ) return;	// done
	else continue;			// not done yet, more filters

      // Parse all other chars
      default:				// handle all non-special chars
      regchar:				// handle regular char
	switch ( mode ) {
	  case 'n': chrcat(name, *in);     continue;
	  case 'w': chrcat(wildcard, *in); continue;
	}
	break;
    }
  }
  //NOTREACHED
}

/**
  Sets the default filename for the chooser.
  Use directory() to set the default directory.
  Mainly used to preset the filename for save dialogs, 
  and on most platforms can be used for opening files as well. 
 */
void Fl_Native_File_Chooser::preset_file(const char* val) {
  _preset_file = strfree(_preset_file);
  _preset_file = strnew(val);
}

/**
  Get the preset filename.
  */
const char* Fl_Native_File_Chooser::preset_file() const {
  return(_preset_file);
}


int Fl_Native_File_Chooser::exist_dialog() {
  return(fl_choice("%s", fl_cancel, fl_ok, NULL, file_exists_message));
}

//
// End of "$Id: Fl_Native_File_Chooser_FLTK.cxx 8282 2011-01-16 18:26:51Z manolo $".
//
