// "$Id: Fl_Native_File_Chooser_WIN32.cxx 8454 2011-02-20 21:46:11Z manolo $"
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

// Any application to multi-folder implementation:
//     http://www.codeproject.com/dialog/selectfolder.asp
//

#ifndef FL_DOXYGEN		// PREVENT DOXYGEN'S USE OF THIS FILE

#include <stdio.h>		// debugging
#include <wchar.h>		//MG
#include "Fl_Native_File_Chooser_common.cxx"		// strnew/strfree/strapp/chrcat
typedef const wchar_t *LPCWSTR; //MG
LPCWSTR utf8towchar(const char *in); //MG
char *wchartoutf8(LPCWSTR in);  //MG

#include <FL/Fl_Native_File_Chooser.H>

#define LCURLY_CHR	'{'
#define RCURLY_CHR	'}'
#define LBRACKET_CHR	'['
#define RBRACKET_CHR	']'
#define MAXFILTERS	80

void fl_OleInitialize();	// in Fl.cxx (Windows only)

// STATIC: PRINT WINDOWS 'DOUBLE NULL' STRING (DEBUG)
#ifdef DEBUG
static void dnullprint(char *wp) {
  if ( ! wp ) return;
  for ( int t=0; true; t++ ) {
    if ( wp[t] == '\0' && wp[t+1] == '\0' ) {
      printf("\\0\\0");
      fflush(stdout);
      return;
    } else if ( wp[t] == '\0' ) {
      printf("\\0");
    } else { 
      printf("%c",wp[t]);
    }
  }
}
#endif

// RETURN LENGTH OF DOUBLENULL STRING
//    Includes single nulls in count, excludes trailing doublenull.
//
//         1234 567
//         |||/\|||
//    IN: "one\0two\0\0"
//   OUT: 7
//
static int dnulllen(const char *wp) {
  int len = 0;
  while ( ! ( *(wp+0) == 0 && *(wp+1) == 0 ) ) {
    ++wp;
    ++len;
  }
  return(len);
}

// STATIC: Append a string to another, leaving terminated with DOUBLE NULL.
//     Automatically handles extending length of string.
//     wp can be NULL (a new wp will be allocated and initialized).
//     string must be NULL terminated.
//     The pointer wp may be modified on return.
//
static void dnullcat(char*&wp, const char *string, int n = -1 ) {
  //DEBUG printf("DEBUG: dnullcat IN: <"); dnullprint(wp); printf(">\n");
  int inlen = ( n < 0 ) ? strlen(string) : n;
  if ( ! wp ) {
    wp = new char[inlen + 4];
    *(wp+0) = '\0';
    *(wp+1) = '\0';
  } else {
    int wplen = dnulllen(wp);
    // Make copy of wp into larger buffer
    char *tmp = new char[wplen + inlen + 4];
    memcpy(tmp, wp, wplen+2);	// copy of wp plus doublenull
    delete [] wp;			// delete old wp
    wp = tmp;			// use new copy
    //DEBUG printf("DEBUG: dnullcat COPY: <"); dnullprint(wp); printf("> (wplen=%d)\n", wplen);
  }

  // Find end of double null string
  //     *wp2 is left pointing at second null.
  //
  char *wp2 = wp;
  if ( *(wp2+0) != '\0' && *(wp2+1) != '\0' ) {
    for ( ; 1; wp2++ ) {
      if ( *(wp2+0) == '\0' && *(wp2+1) == '\0' ) {
        wp2++;
        break;
      }
    }
  }

  if ( n == -1 ) n = strlen(string);
  strncpy(wp2, string, n);

  // Leave string double-null terminated
  *(wp2+n+0) = '\0';
  *(wp2+n+1) = '\0';
  //DEBUG printf("DEBUG: dnullcat OUT: <"); dnullprint(wp); printf(">\n\n");
}

// CTOR
Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
  _btype           = val;
  _options         = NO_OPTIONS;
  memset((void*)&_ofn, 0, sizeof(OPENFILENAMEW));
  _ofn.lStructSize = sizeof(OPENFILENAMEW);
  _ofn.hwndOwner   = NULL;
  memset((void*)&_binf, 0, sizeof(BROWSEINFO));
  _pathnames       = NULL;
  _tpathnames      = 0;
  _directory       = NULL;
  _title           = NULL;
  _filter          = NULL;
  _parsedfilt      = NULL;
  _nfilters        = 0;
  _preset_file     = NULL;
  _errmsg          = NULL;
}

// DTOR
Fl_Native_File_Chooser::~Fl_Native_File_Chooser() {
  //_pathnames                // managed by clear_pathnames()
  //_tpathnames               // managed by clear_pathnames()
  _directory   = strfree(_directory);
  _title       = strfree(_title);
  _filter      = strfree(_filter);
  //_parsedfilt               // managed by clear_filters()
  //_nfilters                 // managed by clear_filters()
  _preset_file = strfree(_preset_file);
  _errmsg      = strfree(_errmsg);
  clear_filters();
  clear_pathnames();
  ClearOFN();
  ClearBINF();
}

// SET TYPE OF BROWSER
void Fl_Native_File_Chooser::type(int val) {
  _btype = val;
}

// GET TYPE OF BROWSER
int Fl_Native_File_Chooser::type() const {
  return( _btype );
}

// SET OPTIONS
void Fl_Native_File_Chooser::options(int val) {
  _options = val;
}

// GET OPTIONS
int Fl_Native_File_Chooser::options() const {
  return(_options);
}

// PRIVATE: SET ERROR MESSAGE
void Fl_Native_File_Chooser::errmsg(const char *val) {
  _errmsg = strfree(_errmsg);
  _errmsg = strnew(val);
}

// FREE PATHNAMES ARRAY, IF IT HAS ANY CONTENTS
void Fl_Native_File_Chooser::clear_pathnames() {
  if ( _pathnames ) {
    while ( --_tpathnames >= 0 ) {
      _pathnames[_tpathnames] = strfree(_pathnames[_tpathnames]);
    }
    delete [] _pathnames;
    _pathnames = NULL;
  }
  _tpathnames = 0;
}

// SET A SINGLE PATHNAME
void Fl_Native_File_Chooser::set_single_pathname(const char *s) {
  clear_pathnames();
  _pathnames = new char*[1];
  _pathnames[0] = strnew(s);
  _tpathnames = 1;
}

// ADD PATHNAME TO EXISTING ARRAY
void Fl_Native_File_Chooser::add_pathname(const char *s) {
  if ( ! _pathnames ) {
    // Create first element in array
    ++_tpathnames;
    _pathnames = new char*[_tpathnames];
  } else {
    // Grow array by 1
    char **tmp = new char*[_tpathnames+1];		// create new buffer
    memcpy((void*)tmp, (void*)_pathnames, 
		       sizeof(char*)*_tpathnames);	// copy old
    delete [] _pathnames;				// delete old
    _pathnames = tmp;					// use new
    ++_tpathnames;
  }
  _pathnames[_tpathnames-1] = strnew(s);
}

// FREE A PIDL (Pointer to IDentity List)
void Fl_Native_File_Chooser::FreePIDL(ITEMIDLIST *pidl) {
  IMalloc *imalloc = NULL;
  if ( SUCCEEDED(SHGetMalloc(&imalloc)) ) {
    imalloc->Free(pidl);
    imalloc->Release();
    imalloc = NULL;
  }
}

// CLEAR MICROSOFT OFN (OPEN FILE NAME) CLASS
void Fl_Native_File_Chooser::ClearOFN() {
  // Free any previously allocated lpstrFile before zeroing out _ofn
  if ( _ofn.lpstrFile ) {
    delete [] _ofn.lpstrFile;
    _ofn.lpstrFile = NULL;
  }
  if ( _ofn.lpstrInitialDir ) {
    delete [] (TCHAR*) _ofn.lpstrInitialDir; //msvc6 compilation fix
    _ofn.lpstrInitialDir = NULL;
  }
  _ofn.lpstrFilter = NULL;		// (deleted elsewhere)
  int temp = _ofn.nFilterIndex;		// keep the filter_value
  memset((void*)&_ofn, 0, sizeof(_ofn));
  _ofn.lStructSize  = sizeof(OPENFILENAMEW);
  _ofn.nFilterIndex = temp;
}

// CLEAR MICROSOFT BINF (BROWSER INFO) CLASS
void Fl_Native_File_Chooser::ClearBINF() {
  if ( _binf.pidlRoot ) {
    FreePIDL((ITEMIDLIST*)_binf.pidlRoot);
    _binf.pidlRoot = NULL;
  }
  memset((void*)&_binf, 0, sizeof(_binf));
}

// CONVERT WINDOWS BACKSLASHES TO UNIX FRONTSLASHES
void Fl_Native_File_Chooser::Win2Unix(char *s) {
  for ( ; *s; s++ )
    if ( *s == '\\' ) *s = '/';
}

// CONVERT UNIX FRONTSLASHES TO WINDOWS BACKSLASHES
void Fl_Native_File_Chooser::Unix2Win(char *s) {
  for ( ; *s; s++ )
    if ( *s == '/' ) *s = '\\';
}

// SHOW FILE BROWSER
int Fl_Native_File_Chooser::showfile() {
  ClearOFN();
  clear_pathnames();
  size_t fsize = MAX_PATH;
  _ofn.Flags |= OFN_NOVALIDATE;		// prevent disabling of front slashes
  _ofn.Flags |= OFN_HIDEREADONLY;	// hide goofy readonly flag
  // USE NEW BROWSER
  _ofn.Flags |= OFN_EXPLORER;		// use newer explorer windows
  _ofn.Flags |= OFN_ENABLESIZING;	// allow window to be resized (hey, why not?)

  // XXX: The docs for OFN_NOCHANGEDIR says the flag is 'ineffective' on XP/2K/NT!
  //      But let's set it anyway..
  //
  _ofn.Flags |= OFN_NOCHANGEDIR;	// prevent dialog for messing up the cwd

  switch ( _btype ) {
    case BROWSE_DIRECTORY:
    case BROWSE_MULTI_DIRECTORY:
    case BROWSE_SAVE_DIRECTORY:
      abort();				// never happens: handled by showdir()
    case BROWSE_FILE:
      fsize = 65536;			// XXX: there must be a better way
      break;
    case BROWSE_MULTI_FILE:
      _ofn.Flags |= OFN_ALLOWMULTISELECT;
      fsize = 65536;			// XXX: there must be a better way
      break;
    case BROWSE_SAVE_FILE:
      if ( options() & SAVEAS_CONFIRM && type() == BROWSE_SAVE_FILE ) {
	  _ofn.Flags |= OFN_OVERWRITEPROMPT;
      }
      break;
  }
  // SPACE FOR RETURNED FILENAME
  _ofn.lpstrFile    = new WCHAR[fsize];
  _ofn.nMaxFile     = fsize-1;
  _ofn.lpstrFile[0] = 0;
  _ofn.lpstrFile[1] = 0;		// dnull
  // PARENT WINDOW
  _ofn.hwndOwner = GetForegroundWindow();
  // DIALOG TITLE
  if (_title) {
    static WCHAR wtitle[200];
    wcscpy(wtitle, utf8towchar(_title));
    _ofn.lpstrTitle =  wtitle;
  } else {
    _ofn.lpstrTitle = NULL;
  }
  // FILTER
  if (_parsedfilt != NULL) {	// to convert a null-containing char string into a widechar string
    static WCHAR wpattern[MAX_PATH];
    const char *p = _parsedfilt;
    while(*(p + strlen(p) + 1) != 0) p += strlen(p) + 1;
    p += strlen(p) + 2;
    MultiByteToWideChar(CP_UTF8, 0, _parsedfilt, p - _parsedfilt, wpattern, MAX_PATH);
    _ofn.lpstrFilter = wpattern;
  } else {
    _ofn.lpstrFilter = NULL;
  }
  // PRESET FILE
  //     If set, supercedes _directory. See KB Q86920 for details
  //
  if ( _preset_file ) {
    size_t len = strlen(_preset_file);
    if ( len >= _ofn.nMaxFile ) {
      char msg[80];
      sprintf(msg, "preset_file() filename is too long: %ld is >=%ld", (long)len, (long)fsize);
      return(-1);
    }
    wcscpy(_ofn.lpstrFile, utf8towchar(_preset_file));
//  Unix2Win(_ofn.lpstrFile);
    len = wcslen(_ofn.lpstrFile);
    _ofn.lpstrFile[len+0] = 0;	// multiselect needs dnull
    _ofn.lpstrFile[len+1] = 0;
  }
  if ( _directory ) {
    // PRESET DIR
    //     XXX: See KB Q86920 for doc bug:
    //     http://support.microsoft.com/default.aspx?scid=kb;en-us;86920
    //
    _ofn.lpstrInitialDir    = new WCHAR[MAX_PATH];
    wcscpy((WCHAR *)_ofn.lpstrInitialDir, utf8towchar(_directory));
    // Unix2Win((char*)_ofn.lpstrInitialDir);
  }
  // SAVE THE CURRENT DIRECTORY
  //     XXX: Save the cwd because GetOpenFileName() is probably going to
  //     change it, in spite of the OFN_NOCHANGEDIR flag, due to its docs
  //     saying the flag is 'ineffective'. %^(
  //
  char oldcwd[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, oldcwd);
  oldcwd[MAX_PATH-1] = '\0';
  // OPEN THE DIALOG WINDOW
  int err;
  if ( _btype == BROWSE_SAVE_FILE ) {
    err = GetSaveFileNameW(&_ofn);
  } else {
    err = GetOpenFileNameW(&_ofn);
  }
  if ( err == 0 ) {
    // EXTENDED ERROR CHECK
    int err = CommDlgExtendedError();
    // CANCEL?
    if ( err == 0 ) return(1);	// user hit 'cancel'
    // AN ERROR OCCURRED
    char msg[80];
    sprintf(msg, "CommDlgExtendedError() code=%d", err);
    errmsg(msg);
    // XXX: RESTORE CWD
    if ( oldcwd[0] ) SetCurrentDirectory(oldcwd);
    return(-1);
  }
  // XXX: RESTORE CWD
  if ( oldcwd[0] ) {
    SetCurrentDirectory(oldcwd);
  }
  // PREPARE PATHNAMES FOR RETURN
  switch ( _btype ) {
    case BROWSE_FILE: 
    case BROWSE_SAVE_FILE:
      set_single_pathname(wchartoutf8(_ofn.lpstrFile));
      // Win2Unix(_pathnames[_tpathnames-1]);
      break;
    case BROWSE_MULTI_FILE: {
      // EXTRACT MULTIPLE FILENAMES
      const WCHAR *dirname = _ofn.lpstrFile;
      int dirlen = wcslen(dirname);
      if ( dirlen > 0 ) {
	// WALK STRING SEARCHING FOR 'DOUBLE-NULL'
	//     eg. "/dir/name\0foo1\0foo2\0foo3\0\0"
	//
	char pathname[MAX_PATH]; 
	for ( const WCHAR *s = dirname + dirlen + 1; 
		 *s; s+= (wcslen(s)+1)) {
		strcpy(pathname, wchartoutf8(dirname));
		strcat(pathname, "\\");
		strcat(pathname, wchartoutf8(s));
		add_pathname(pathname);
	}
      }
      // XXX
      //    Work around problem where pasted forward-slash pathname
      //    into the file browser causes new "Explorer" interface
      //    not to grok forward slashes, passing back as a 'filename'..!
      //
      if ( _tpathnames == 0 ) {
	add_pathname(wchartoutf8(dirname)); 
	// Win2Unix(_pathnames[_tpathnames-1]);
      }
      break;
    }
    case BROWSE_DIRECTORY:
    case BROWSE_MULTI_DIRECTORY:
    case BROWSE_SAVE_DIRECTORY:
      abort();			// never happens: handled by showdir()
  }
  return(0);
}

// Used by SHBrowseForFolder(), sets initial selected dir.
// Ref: Usenet: microsoft.public.vc.mfc, Dec 8 2000, 1:38p David Lowndes
//              Subject: How to specify to select an initial folder .."
//
int CALLBACK Fl_Native_File_Chooser::Dir_CB(HWND win, UINT msg, LPARAM param, LPARAM data) {
  switch (msg) {
    case BFFM_INITIALIZED:
      if (data) ::SendMessage(win, BFFM_SETSELECTION, TRUE, data);
      break;
    case BFFM_SELCHANGED:
      TCHAR path[MAX_PATH];
      if ( SHGetPathFromIDList((ITEMIDLIST*)param, path) ) {
	::SendMessage(win, BFFM_ENABLEOK, 0, 1);
      } else {
	// disable ok button if not a path
	::SendMessage(win, BFFM_ENABLEOK, 0, 0);
      }
      break;
    case BFFM_VALIDATEFAILED:
      // we could pop up an annoying message here. 
      // also needs set ulFlags |= BIF_VALIDATE
      break;
    default:
      break;
  }
  return(0);
}

// SHOW DIRECTORY BROWSER
int Fl_Native_File_Chooser::showdir() {
  // initialize OLE only once
  fl_OleInitialize();		// init needed by BIF_USENEWUI
  ClearBINF();
  clear_pathnames();
  // PARENT WINDOW
  _binf.hwndOwner = GetForegroundWindow();
  // DIALOG TITLE
  _binf.lpszTitle = _title ? _title : NULL;
  // FLAGS
  _binf.ulFlags = 0; 		// initialize

  // TBD: make sure matches to runtime system, if need be.
  //(what if _WIN32_IE doesn't match system? does the program not run?)
  //
  // TBD: match all 3 types of directories
  //
  // NOTE: *Don't* use BIF_SHAREABLE. It /disables/ mapped network shares
  //       from being visible in BROWSE_DIRECTORY mode. 
  //       See Walter Garm's comments in ./TODO.

#if defined(BIF_NONEWFOLDERBUTTON)				// Version 6.0
  if ( _btype == BROWSE_DIRECTORY ) _binf.ulFlags |= BIF_NONEWFOLDERBUTTON;
  _binf.ulFlags |= BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
#elif defined(BIF_USENEWUI)					// Version 5.0
  if ( _btype == BROWSE_DIRECTORY ) _binf.ulFlags |= BIF_EDITBOX;
  else if ( _btype == BROWSE_SAVE_DIRECTORY ) _binf.ulFlags |= BIF_USENEWUI;
  _binf.ulFlags |= BIF_RETURNONLYFSDIRS;
#elif defined(BIF_EDITBOX)					// Version 4.71
  _binf.ulFlags |= BIF_RETURNONLYFSDIRS | BIF_EDITBOX;
#else								// Version Old
  _binf.ulFlags |= BIF_RETURNONLYFSDIRS;
#endif

  // BUFFER
  char displayname[MAX_PATH];
  _binf.pszDisplayName = displayname;
  // PRESET DIR
  char presetname[MAX_PATH];
  if ( _directory ) {
    strcpy(presetname, _directory);
    // Unix2Win(presetname);
    _binf.lParam = (LPARAM)presetname;
  }
  else _binf.lParam = 0;
  _binf.lpfn = Dir_CB;
  // OPEN BROWSER
  ITEMIDLIST *pidl = SHBrowseForFolder(&_binf);
  // CANCEL?
  if ( pidl == NULL ) return(1);

  // GET THE PATHNAME(S) THE USER SELECTED
  // TBD: expand NetHood shortcuts from this PIDL??
  // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/shell/reference/functions/shbrowseforfolder.asp

  TCHAR path[MAX_PATH];
  if ( SHGetPathFromIDList(pidl, path) ) {
    // Win2Unix(path);
    add_pathname(path);
  }
  FreePIDL(pidl);
  if ( !strlen(path) ) return(1);             // don't return empty pathnames
  return(0);
}

// RETURNS:
//    0 - user picked a file
//    1 - user cancelled
//   -1 - failed; errmsg() has reason
//
int Fl_Native_File_Chooser::show() {
  if ( _btype == BROWSE_DIRECTORY || 
       _btype == BROWSE_MULTI_DIRECTORY || 
       _btype == BROWSE_SAVE_DIRECTORY ) {
    return(showdir());
  } else {
    return(showfile());
  }
}

// RETURN ERROR MESSAGE
const char *Fl_Native_File_Chooser::errmsg() const {
  return(_errmsg ? _errmsg : "No error");
}

// GET FILENAME
const char* Fl_Native_File_Chooser::filename() const {
  if ( _pathnames && _tpathnames > 0 ) return(_pathnames[0]);
  return("");
}

// GET FILENAME FROM LIST OF FILENAMES
const char* Fl_Native_File_Chooser::filename(int i) const {
  if ( _pathnames && i < _tpathnames ) return(_pathnames[i]);
  return("");
}

// GET TOTAL FILENAMES CHOSEN
int Fl_Native_File_Chooser::count() const {
  return(_tpathnames);
}

// PRESET PATHNAME
//     Can be NULL if no preset is desired.
//
void Fl_Native_File_Chooser::directory(const char *val) {
  _directory = strfree(_directory);
  _directory = strnew(val);
}

// GET PRESET PATHNAME
//    Can return NULL if none set.
//
const char *Fl_Native_File_Chooser::directory() const {
  return(_directory);
}

// SET TITLE
//     Can be NULL if no title desired.
//
void Fl_Native_File_Chooser::title(const char *val) {
  _title = strfree(_title);
  _title = strnew(val);
}

// GET TITLE
//    Can return NULL if none set.
//
const char *Fl_Native_File_Chooser::title() const {
  return(_title);
}

// SET FILTER
//     Can be NULL if no filter needed
//
void Fl_Native_File_Chooser::filter(const char *val) {
  _filter = strfree(_filter);
  clear_filters();
  if ( val ) {
    _filter = strnew(val);
    parse_filter(_filter);
  }
  add_filter("All Files", "*.*");	// always include 'all files' option

#ifdef DEBUG
  nullprint(_parsedfilt);
#endif /*DEBUG*/
}

// GET FILTER
//    Can return NULL if none set.
//
const char *Fl_Native_File_Chooser::filter() const {
  return(_filter);
}

// CLEAR FILTERS
void Fl_Native_File_Chooser::clear_filters() {
  _nfilters = 0;
  _parsedfilt = strfree(_parsedfilt);
}

// ADD A FILTER
void Fl_Native_File_Chooser::add_filter(const char *name_in,	// name of filter (optional: can be null)
	                    const char *winfilter) {    	// windows style filter (eg. "*.cxx;*.h")
  // No name? Make one..
  char name[1024];
  if ( !name_in || name_in[0] == '\0' ) {
    sprintf(name, "%.*s Files", int(sizeof(name)-10), winfilter);
  } else {
    sprintf(name, "%.*s", int(sizeof(name)-10), name_in);
  }
  dnullcat(_parsedfilt, name);
  dnullcat(_parsedfilt, winfilter);
  _nfilters++;
  //DEBUG printf("DEBUG: ADD FILTER name=<%s> winfilter=<%s>\n", name, winfilter);
}

// CONVERT FLTK STYLE PATTERN MATCHES TO WINDOWS 'DOUBLENULL' PATTERN
//    Handles:
//        IN              OUT
//        -----------     -----------------------------
//        *.{ma,mb}       "*.{ma,mb} Files\0*.ma;*.mb\0\0"
//        *.[abc]         "*.[abc] Files\0*.a;*.b;*.c\0\0"
//        *.txt           "*.txt Files\0*.txt\0\0"
//        C Files\t*.[ch] "C Files\0*.c;*.h\0\0"
//
//    Example:
//         IN: "*.{ma,mb}"
//        OUT: "*.ma;*.mb Files\0*.ma;*.mb\0All Files\0*.*\0\0"
//              ---------------  ---------  ---------  ---
//                     |             |          |       |
//                   Title       Wildcards    Title    Wildcards
//
// Parsing Mode:
//         IN:"C Files\t*.{cxx,h}"
//             |||||||  |||||||||
//       mode: nnnnnnn  ww{{{{{{{
//             \_____/  \_______/
//              Name     Wildcard
//
void Fl_Native_File_Chooser::parse_filter(const char *in) {
  clear_filters();
  if ( ! in ) return;

  int has_name = strchr(in, '\t') ? 1 : 0;

  char mode = has_name ? 'n' : 'w';	// parse mode: n=name, w=wildcard
  int nwildcards = 0;
  char wildcards[MAXFILTERS][1024];	// parsed wildcards (can be several)
  char wildprefix[512] = "";
  char name[512] = "";

  // Init
  int t;
  for ( t=0; t<MAXFILTERS; t++ ) {
    wildcards[t][0] = '\0';
  }

  // Parse
  for ( ; 1; in++ ) {

    //// DEBUG
    //// printf("WORKING ON '%c': mode=<%c> name=<%s> wildprefix=<%s> nwildcards=%d wildcards[n]=<%s>\n",
    ////        *in, mode, name, wildprefix, nwildcards, wildcards[nwildcards]);

    switch (*in) {
      case ',':
      case '|':
	if ( mode == LCURLY_CHR ) {
	  // create new wildcard, copy in prefix
	  strcat(wildcards[nwildcards++], wildprefix);
	  continue;
	} else {
	  goto regchar;
	}
	continue;

      // FINISHED PARSING A NAME?
      case '\t':
	if ( mode != 'n' ) goto regchar;
	// finish parsing name? switch to wildcard mode
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
      {
	if ( mode == 'w' ) {		// finished parsing wildcard?
	  if ( nwildcards == 0 ) {
	    strcpy(wildcards[nwildcards++], wildprefix);
	  }
	  // Append wildcards in Microsoft's "*.one;*.two" format
	  char comp[4096] = "";
	  for ( t=0; t<nwildcards; t++ ) {
	    if ( t != 0 ) strcat(comp, ";");
	    strcat(comp, wildcards[t]);
	  }
	  // Add if not empty
	  if ( comp[0] ) {
	    add_filter(name, comp);
	  }
	}
	// RESET
	for ( t=0; t<MAXFILTERS; t++ ) {
	  wildcards[t][0] = '\0';
	}
	nwildcards = 0;
	wildprefix[0] = name[0] = '\0';
	mode = strchr(in,'\t') ? 'n' : 'w';
	// DONE?
	if ( *in == '\0' ) return;	// done
	continue;			// not done yet, more filters
      }

      // STARTING A WILDCARD?
      case LBRACKET_CHR:
      case LCURLY_CHR:
	mode = *in;
	if ( *in == LCURLY_CHR ) {
	  // create new wildcard
	  strcat(wildcards[nwildcards++], wildprefix);
	}
	continue;

      // ENDING A WILDCARD?
      case RBRACKET_CHR:
      case RCURLY_CHR:
	mode = 'w';	// back to wildcard mode
	continue;

      // ALL OTHER NON-SPECIAL CHARACTERS
      default:
      regchar:		// handle regular char
	switch ( mode ) {
	  case LBRACKET_CHR: 
	    // create new wildcard
	    ++nwildcards;
	    // copy in prefix
	    strcpy(wildcards[nwildcards-1], wildprefix);
	    // append search char
	    chrcat(wildcards[nwildcards-1], *in);
	    continue;

	  case LCURLY_CHR:
	    if ( nwildcards > 0 ) {
	      chrcat(wildcards[nwildcards-1], *in);
	    }
	    continue;

	  case 'n':
	    chrcat(name, *in);
	    continue;

	  case 'w':
	    chrcat(wildprefix, *in);
	    for ( t=0; t<nwildcards; t++ ) {
	      chrcat(wildcards[t], *in);
	    }
	    continue;
	}
	break;
    }
  }
}

// SET 'CURRENTLY SELECTED FILTER'
void Fl_Native_File_Chooser::filter_value(int i) {
  _ofn.nFilterIndex = i + 1;
}

// RETURN VALUE OF 'CURRENTLY SELECTED FILTER'
int Fl_Native_File_Chooser::filter_value() const {
  return(_ofn.nFilterIndex ? _ofn.nFilterIndex-1 : _nfilters+1);
}

// PRESET FILENAME FOR 'SAVE AS' CHOOSER
void Fl_Native_File_Chooser::preset_file(const char* val) {
  _preset_file = strfree(_preset_file);
  _preset_file = strnew(val);
}

// GET PRESET FILENAME FOR 'SAVE AS' CHOOSER
const char* Fl_Native_File_Chooser::preset_file() const {
  return(_preset_file);
}

int Fl_Native_File_Chooser::filters() const {
  return(_nfilters);
}

char *wchartoutf8(LPCWSTR in)
{
  static char *out = NULL;
  static int lchar = 0;
  if (in == NULL)return NULL;
  int utf8len  = WideCharToMultiByte(CP_UTF8, 0, in, -1, NULL, 0, NULL, NULL);
  if (utf8len > lchar) {
    lchar = utf8len;
    out = (char *)realloc(out, lchar * sizeof(char));
  }
  WideCharToMultiByte(CP_UTF8, 0, in, -1, out, utf8len, NULL, NULL);
  return out;
}

LPCWSTR utf8towchar(const char *in)
{
  static WCHAR *wout = NULL;
  static int lwout = 0;
  if (in == NULL)return NULL;
  int wlen = MultiByteToWideChar(CP_UTF8, 0, in, -1, NULL, 0);
  if (wlen > lwout) {
    lwout = wlen;
    wout = (WCHAR *)realloc(wout, lwout * sizeof(WCHAR));
  }
  MultiByteToWideChar(CP_UTF8, 0, in, -1, wout, wlen);
  return wout;
}

#endif /*!FL_DOXYGEN*/

//
// End of "$Id: Fl_Native_File_Chooser_WIN32.cxx 8454 2011-02-20 21:46:11Z manolo $".
//
