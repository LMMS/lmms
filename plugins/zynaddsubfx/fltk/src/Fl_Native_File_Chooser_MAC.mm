// "$Id: Fl_Native_File_Chooser_MAC.mm 8784 2011-06-06 12:11:04Z manolo $"
//
// FLTK native OS file chooser widget
//
// Copyright 1998-2010 by Bill Spitzak and others.
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
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//

// TODO:
//	o When doing 'open file', only dir is preset, not filename.
//        Possibly 'preset_file' could be used to select the filename.
//

#ifdef __APPLE__

#include "Fl_Native_File_Chooser_common.cxx"		// strnew/strfree/strapp/chrcat
#include <libgen.h>		// dirname(3)
#include <sys/types.h>		// stat(2)
#include <sys/stat.h>		// stat(2)


#include <FL/Fl.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>

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

// CONSTRUCTOR
Fl_Native_File_Chooser::Fl_Native_File_Chooser(int val) {
  _btype          = val;
  _panel = NULL;
  _options        = NO_OPTIONS;
  _pathnames      = NULL;
  _tpathnames     = 0;
  _title          = NULL;
  _filter         = NULL;
  _filt_names     = NULL;
  memset(_filt_patt, 0, sizeof(char*) * MAXFILTERS);
  _filt_total     = 0;
  _filt_value     = 0;
  _directory      = NULL;
  _preset_file    = NULL;
  _errmsg         = NULL;
}

// DESTRUCTOR
Fl_Native_File_Chooser::~Fl_Native_File_Chooser() {
  // _opts		// nothing to manage
  // _options		// nothing to manage
  // _keepstate		// nothing to manage
  // _tempitem		// nothing to manage
  clear_pathnames();
  _directory   = strfree(_directory);
  _title       = strfree(_title);
  _preset_file = strfree(_preset_file);
  _filter      = strfree(_filter);
  //_filt_names		// managed by clear_filters()
  //_filt_patt[i]	// managed by clear_filters()
  //_filt_total		// managed by clear_filters()
  clear_filters();
  //_filt_value		// nothing to manage
  _errmsg = strfree(_errmsg);
}

// GET TYPE OF BROWSER
int Fl_Native_File_Chooser::type() const {
  return(_btype);
}

// SET OPTIONS
void Fl_Native_File_Chooser::options(int val) {
  _options = val;
}

// GET OPTIONS
int Fl_Native_File_Chooser::options() const {
  return(_options);
}

// SHOW THE BROWSER WINDOW
//     Returns:
//         0 - user picked a file
//         1 - user cancelled
//        -1 - failed; errmsg() has reason
//
int Fl_Native_File_Chooser::show() {

  // Make sure fltk interface updates before posting our dialog
  Fl::flush();
  
  // POST BROWSER
  int err = post();

  _filt_total = 0;

  return(err);
}

// SET ERROR MESSAGE
//     Internal use only.
//
void Fl_Native_File_Chooser::errmsg(const char *msg) {
  _errmsg = strfree(_errmsg);
  _errmsg = strnew(msg);
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
//     Value can be NULL for none.
//
void Fl_Native_File_Chooser::directory(const char *val) {
  _directory = strfree(_directory);
  _directory = strnew(val);
}

// GET PRESET PATHNAME
//     Returned value can be NULL if none set.
//
const char* Fl_Native_File_Chooser::directory() const {
  return(_directory);
}

// SET TITLE
//     Value can be NULL if no title desired.
//
void Fl_Native_File_Chooser::title(const char *val) {
  _title = strfree(_title);
  _title = strnew(val);
}

// GET TITLE
//     Returned value can be NULL if none set.
//
const char *Fl_Native_File_Chooser::title() const {
  return(_title);
}

// SET FILTER
//     Can be NULL if no filter needed
//
void Fl_Native_File_Chooser::filter(const char *val) {
  _filter = strfree(_filter);
  _filter = strnew(val);

  // Parse filter user specified
  //     IN: _filter = "C Files\t*.{cxx,h}\nText Files\t*.txt"
  //    OUT: _filt_names   = "C Files\tText Files"
  //         _filt_patt[0] = "*.{cxx,h}"
  //         _filt_patt[1] = "*.txt"
  //         _filt_total   = 2
  //
  parse_filter(_filter);
}

// GET FILTER
//     Returned value can be NULL if none set.
//
const char *Fl_Native_File_Chooser::filter() const {
  return(_filter);
}

// CLEAR ALL FILTERS
//    Internal use only.
//
void Fl_Native_File_Chooser::clear_filters() {
  _filt_names = strfree(_filt_names);
  for (int i=0; i<_filt_total; i++) {
    _filt_patt[i] = strfree(_filt_patt[i]);
  }
  _filt_total = 0;
}

// PARSE USER'S FILTER SPEC
//    Parses user specified filter ('in'),
//    breaks out into _filt_patt[], _filt_names, and _filt_total.
//
//    Handles:
//    IN:                                   OUT:_filt_names    OUT: _filt_patt
//    ------------------------------------  ------------------ ---------------
//    "*.{ma,mb}"                           "*.{ma,mb} Files"  "*.{ma,mb}"
//    "*.[abc]"                             "*.[abc] Files"    "*.[abc]"
//    "*.txt"                               "*.txt Files"      "*.c"
//    "C Files\t*.[ch]"                     "C Files"          "*.[ch]"
//    "C Files\t*.[ch]\nText Files\t*.cxx"  "C Files"          "*.[ch]"
//
//    Parsing Mode:
//         IN:"C Files\t*.{cxx,h}"
//             |||||||  |||||||||
//       mode: nnnnnnn  wwwwwwwww
//             \_____/  \_______/
//              Name     Wildcard
//
void Fl_Native_File_Chooser::parse_filter(const char *in) {
  clear_filters();
  if ( ! in ) return;
  int has_name = strchr(in, '\t') ? 1 : 0;

  char mode = has_name ? 'n' : 'w';	// parse mode: n=title, w=wildcard
  char wildcard[1024] = "";		// parsed wildcard
  char name[1024] = "";

  // Parse filter user specified
  for ( ; 1; in++ ) {

    //// DEBUG
    //// printf("WORKING ON '%c': mode=<%c> name=<%s> wildcard=<%s>\n",
    ////                    *in,  mode,     name,     wildcard);
    
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
	// TITLE
	//     If user didn't specify a name, make one
	//
	if ( name[0] == '\0' ) {
	  sprintf(name, "%.*s Files", (int)sizeof(name)-10, wildcard);
	}
	// APPEND NEW FILTER TO LIST
	if ( wildcard[0] ) {
	  // Add to filtername list
	  //     Tab delimit if more than one. We later break
	  //     tab delimited string into CFArray with 
	  //     CFStringCreateArrayBySeparatingStrings()
	  //
	  if ( _filt_total ) {
	      _filt_names = strapp(_filt_names, "\t");
	  }
	  _filt_names = strapp(_filt_names, name);

	  // Add filter to the pattern array
	  _filt_patt[_filt_total++] = strnew(wildcard);
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

// SET PRESET FILE
//     Value can be NULL for none.
//
void Fl_Native_File_Chooser::preset_file(const char* val) {
  _preset_file = strfree(_preset_file);
  _preset_file = strnew(val);
}

// PRESET FILE
//     Returned value can be NULL if none set.
//
const char* Fl_Native_File_Chooser::preset_file() const {
  return(_preset_file);
}

void Fl_Native_File_Chooser::filter_value(int val) {
  _filt_value = val;
}

int Fl_Native_File_Chooser::filter_value() const {
  return(_filt_value);
}

int Fl_Native_File_Chooser::filters() const {
  return(_filt_total);
}

#import <Cocoa/Cocoa.h>
#define UNLIKELYPREFIX "___fl_very_unlikely_prefix_"
#ifndef MAC_OS_X_VERSION_10_6
#define MAC_OS_X_VERSION_10_6 1060
#endif

int Fl_Native_File_Chooser::get_saveas_basename(void) {
  char *q = strdup( [[(NSSavePanel*)_panel filename] fileSystemRepresentation] );
  id delegate = [(NSSavePanel*)_panel delegate];
  if (delegate != nil) {
    const char *d = [[(NSSavePanel*)_panel directory] fileSystemRepresentation];
    int l = strlen(d) + 1;
    int lu = strlen(UNLIKELYPREFIX);
    // Remove UNLIKELYPREFIX between directory and filename parts
    memmove(q + l, q + l + lu, strlen(q + l + lu) + 1);
  }
  set_single_pathname( q );
  free(q);
  return 0;
}

// SET THE TYPE OF BROWSER
void Fl_Native_File_Chooser::type(int val) {
  _btype = val;
}

/* Input
 filter=  "C files\t*.{c,h}\nText files\t*.txt\n"
 patterns[0] = "*.{c,h}"
 patterns[1] = "*.txt"
 count = 2
 Return:
 "C files (*.{c,h})\nText files (*.txt)\n"
 */
static char *prepareMacFilter(int count, const char *filter, char **patterns) {
  int rank = 0, l = 0;
  for (int i = 0; i < count; i++) {
    l += strlen(patterns[i]) + 3;
    }
  const char *p = filter;
  char *q; q = new char[strlen(p) + l + 1];
  const char *r, *s;
  char *t;
  t = q;
  do {	// copy to t what is in filter removing what is between \t and \n, if any
    r = strchr(p, '\n');
    if (!r) r = p + strlen(p);
    s = strchr(p, '\t');
    if (s && s < r) { 
      memcpy(q, p, s - p); 
      q += s - p; 
      if (rank < count) { sprintf(q, " (%s)", patterns[rank]); q += strlen(q); }
    }
    else { 
      memcpy(q, p, r - p); 
      q += r - p; 
    }
    rank++;
    *(q++) = '\n'; 
    if (*p) p = r + 1;
  } while(*p);
  *q = 0;
  return t;
}
  
@interface FLopenDelegate : NSObject 
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
<NSOpenSavePanelDelegate>
#endif
{
  NSPopUpButton *nspopup;
  char **filter_pattern;
}
- (FLopenDelegate*)setPopup:(NSPopUpButton*)popup filter_pattern:(char**)pattern;
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename;
@end
@implementation FLopenDelegate
- (FLopenDelegate*)setPopup:(NSPopUpButton*)popup filter_pattern:(char**)pattern
{
  nspopup = popup;
  filter_pattern = pattern;
  return self;
}
- (BOOL)panel:(id)sender shouldShowFilename:(NSString *)filename
{
  if ( [nspopup indexOfSelectedItem] == [nspopup numberOfItems] - 1) return YES;
  const char *pathname = [filename fileSystemRepresentation];
  if ( fl_filename_isdir(pathname) ) return YES;
  if ( fl_filename_match(pathname, filter_pattern[ [nspopup indexOfSelectedItem] ]) ) return YES;
  return NO;
}
@end

@interface FLsaveDelegate : NSObject 
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
<NSOpenSavePanelDelegate>
#endif
{
}
- (NSString *)panel:(id)sender userEnteredFilename:(NSString *)filename confirmed:(BOOL)okFlag;
@end
@implementation FLsaveDelegate
- (NSString *)panel:(id)sender userEnteredFilename:(NSString *)filename confirmed:(BOOL)okFlag
{
  if (! okFlag) return filename;
  // User has clicked save, and no overwrite confirmation should occur.
  // To get the latter, we need to change the name we return (hence the prefix):
  return [@ UNLIKELYPREFIX stringByAppendingString:filename];
}
@end
  
static NSPopUpButton *createPopupAccessory(NSSavePanel *panel, const char *filter, const char *title, int rank)
{
  NSPopUpButton *popup;
  NSRect rectview = NSMakeRect(5, 5, 350, 30 );
  NSView *view = [[[NSView alloc] initWithFrame:rectview] autorelease];
  NSRect rectbox = NSMakeRect(0, 3, 140, 20 );
  NSBox *box = [[[NSBox alloc] initWithFrame:rectbox] autorelease];
  NSRect rectpop = NSMakeRect(105, 0, 246, 30 );
  popup = [[[NSPopUpButton alloc ] initWithFrame:rectpop pullsDown:NO] autorelease];
  [view addSubview:box];
  [view addSubview:popup];
  [box setBorderType:NSNoBorder];
  NSString *nstitle = [[NSString alloc] initWithUTF8String:title];
  [box setTitle:nstitle];
  [nstitle release];
  NSFont *font = [NSFont controlContentFontOfSize:NSRegularControlSize];
  [box setTitleFont:font];
  [box sizeToFit];
  // horizontally move box to fit the locale-dependent width of its title
  NSRect r=[box frame];
  NSPoint o = r.origin;
  o.x = rectpop.origin.x - r.size.width + 15;
  [box setFrameOrigin:o];
  CFStringRef tab = CFSTR("\n");
  CFStringRef tmp_cfs;
  tmp_cfs = CFStringCreateWithCString(NULL, filter, kCFStringEncodingUTF8);
  CFArrayRef array = CFStringCreateArrayBySeparatingStrings(NULL, tmp_cfs, tab);
  CFRelease(tmp_cfs);
  CFRelease(tab);
  [popup addItemsWithTitles:(NSArray*)array];
  NSMenuItem *item = [popup itemWithTitle:@""];
  if (item) [popup removeItemWithTitle:@""];
  CFRelease(array);
  [popup selectItemAtIndex:rank];
  [panel setAccessoryView:view];
  return popup;
}
  
// POST BROWSER
//     Internal use only.
//     Assumes '_opts' has been initialized.
//
//     Returns:
//         0 - user picked a file
//         1 - user cancelled
//        -1 - failed; errmsg() has reason
//     
int Fl_Native_File_Chooser::post() {
  // INITIALIZE BROWSER
  if ( _filt_total == 0 ) {	// Make sure they match
    _filt_value = 0;		// TBD: move to someplace more logical?
  }
  NSAutoreleasePool *localPool;
  localPool = [[NSAutoreleasePool alloc] init];
  switch (_btype) {
    case BROWSE_FILE:
    case BROWSE_MULTI_FILE:
    case BROWSE_DIRECTORY:
    case BROWSE_MULTI_DIRECTORY:
      _panel =  [NSOpenPanel openPanel];
      break;	  
    case BROWSE_SAVE_DIRECTORY:
    case BROWSE_SAVE_FILE:
      _panel =  [NSSavePanel savePanel];
      break;
  }
  int retval;
  NSString *nstitle = [NSString stringWithUTF8String: (_title ? _title : "No Title")];
  [(NSSavePanel*)_panel setTitle:nstitle];
  switch (_btype) {
    case BROWSE_MULTI_FILE:
      [(NSOpenPanel*)_panel setAllowsMultipleSelection:YES];
      break;
    case BROWSE_MULTI_DIRECTORY:
      [(NSOpenPanel*)_panel setAllowsMultipleSelection:YES];
      /* FALLTHROUGH */
    case BROWSE_DIRECTORY:
      [(NSOpenPanel*)_panel setCanChooseDirectories:YES];
      break;
    case BROWSE_SAVE_DIRECTORY:
      [(NSSavePanel*)_panel setCanCreateDirectories:YES];
      break;
  }
  
  // SHOW THE DIALOG
  if ( [(NSSavePanel*)_panel isKindOfClass:[NSOpenPanel class]] ) {
    NSPopUpButton *popup = nil;
    if (_filt_total) {
      char *t = prepareMacFilter(_filt_total, _filter, _filt_patt);
      popup = createPopupAccessory((NSSavePanel*)_panel, t, Fl_File_Chooser::show_label, 0);
      delete[] t;
      [[popup menu] addItem:[NSMenuItem separatorItem]];
      [popup addItemWithTitle:[[NSString alloc] initWithUTF8String:Fl_File_Chooser::all_files_label]];
      [popup setAction:@selector(validateVisibleColumns)];
      [popup setTarget:(NSObject*)_panel];
      static FLopenDelegate *openDelegate = nil;
      if (openDelegate == nil) {
	// not to be ever freed
	openDelegate = [[FLopenDelegate alloc] init];
      }
      [openDelegate setPopup:popup filter_pattern:_filt_patt];
      [(NSOpenPanel*)_panel setDelegate:openDelegate];
    }
    NSString *dir = nil;
    NSString *fname = nil;
    NSString *preset = nil;
    if (_preset_file) {
      preset = [[NSString alloc] initWithUTF8String:_preset_file];
      if (strchr(_preset_file, '/') != NULL) 
	dir = [[NSString alloc] initWithString:[preset stringByDeletingLastPathComponent]];
      fname = [preset lastPathComponent];
    }
    if (_directory && !dir) dir = [[NSString alloc] initWithUTF8String:_directory];
    retval = [(NSOpenPanel*)_panel runModalForDirectory:dir file:fname types:nil];	
    [dir release];
    [preset release];
    if (_filt_total) {
      _filt_value = [popup indexOfSelectedItem];
    }
    if ( retval == NSOKButton ) {
      clear_pathnames();
      NSArray *array = [(NSOpenPanel*)_panel filenames];
      _tpathnames = [array count];
      _pathnames = new char*[_tpathnames];
      for(int i = 0; i < _tpathnames; i++) {
	_pathnames[i] = strnew([(NSString*)[array objectAtIndex:i] fileSystemRepresentation]);
      }
    }
  }
  else {
    NSString *dir = nil;
    NSString *fname = nil;
    NSString *preset = nil;
    NSPopUpButton *popup = nil;
    [(NSSavePanel*)_panel setAllowsOtherFileTypes:YES];
    if ( !(_options & SAVEAS_CONFIRM) ) {
      static FLsaveDelegate *saveDelegate = nil;
      if (saveDelegate == nil)saveDelegate = [[FLsaveDelegate alloc] init]; // not to be ever freed
      [(NSSavePanel*)_panel setDelegate:saveDelegate];
    }
    if (_preset_file) {
      preset = [[NSString alloc] initWithUTF8String:_preset_file];
      if (strchr(_preset_file, '/') != NULL) {
	dir = [[NSString alloc] initWithString:[preset stringByDeletingLastPathComponent]];
      }
      fname = [preset lastPathComponent];
    }
    if (_directory && !dir) dir = [[NSString alloc] initWithUTF8String:_directory];
    if (_filt_total) {
      char *t = prepareMacFilter(_filt_total, _filter, _filt_patt);
      popup = createPopupAccessory((NSSavePanel*)_panel, t, [[(NSSavePanel*)_panel nameFieldLabel] UTF8String], _filt_value);
      delete[] t;
    }
    retval = [(NSSavePanel*)_panel runModalForDirectory:dir file:fname];
    if (_filt_total) {
      _filt_value = [popup indexOfSelectedItem];
    }
    [dir release];
    [preset release];
    if ( retval == NSOKButton ) get_saveas_basename();
  }
  [localPool release];
  return (retval == NSOKButton ? 0 : 1);
}

#endif // __APPLE__

//
// End of "$Id: Fl_Native_File_Chooser_MAC.mm 8784 2011-06-06 12:11:04Z manolo $".
//
