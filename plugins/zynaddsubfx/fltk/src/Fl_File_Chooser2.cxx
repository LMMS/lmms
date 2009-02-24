//
// "$Id: Fl_File_Chooser2.cxx 6092 2008-04-11 12:57:37Z matt $"
//
// More Fl_File_Chooser routines.
//
// Copyright 1999-2005 by Michael Sweet.
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
// Contents:
//
//   Fl_File_Chooser::count()             - Return the number of selected files.
//   Fl_File_Chooser::directory()         - Set the directory in the file chooser.
//   Fl_File_Chooser::filter()            - Set the filter(s) for the chooser.
//   Fl_File_Chooser::newdir()            - Make a new directory.
//   Fl_File_Chooser::value()             - Return a selected filename.
//   Fl_File_Chooser::rescan()            - Rescan the current directory.
//   Fl_File_Chooser::favoritesButtonCB() - Handle favorites selections.
//   Fl_File_Chooser::fileListCB()        - Handle clicks (and double-clicks)
//                                          in the Fl_File_Browser.
//   Fl_File_Chooser::fileNameCB()        - Handle text entry in the FileBrowser.
//   Fl_File_Chooser::showChoiceCB()      - Handle show selections.
//   compare_dirnames()                   - Compare two directory names.
//   quote_pathname()                     - Quote a pathname for a menu.
//   unquote_pathname()                   - Unquote a pathname from a menu.
//

//
// Include necessary headers.
//

#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <FL/x.H>
#include <FL/Fl_Shared_Image.H>

#include <stdio.h>
#include <stdlib.h>
#include "flstring.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN32) && ! defined (__CYGWIN__)
#  include <direct.h>
#  include <io.h>
// Visual C++ 2005 incorrectly displays a warning about the use of POSIX APIs
// on Windows, which is supposed to be POSIX compliant...
#  define access _access
#  define mkdir _mkdir
// Apparently Borland C++ defines DIRECTORY in <direct.h>, which
// interfers with the Fl_File_Icon enumeration of the same name.
#  ifdef DIRECTORY
#    undef DIRECTORY
#  endif // DIRECTORY
#else
#  include <unistd.h>
#  include <pwd.h>
#endif /* WIN32 */


//
// File chooser label strings and sort function...
//

Fl_Preferences	Fl_File_Chooser::prefs_(Fl_Preferences::USER, "fltk.org", "filechooser");

const char	*Fl_File_Chooser::add_favorites_label = "Add to Favorites";
const char	*Fl_File_Chooser::all_files_label = "All Files (*)";
const char	*Fl_File_Chooser::custom_filter_label = "Custom Filter";
const char	*Fl_File_Chooser::existing_file_label = "Please choose an existing file!";
const char	*Fl_File_Chooser::favorites_label = "Favorites";
const char	*Fl_File_Chooser::filename_label = "Filename:";
#ifdef WIN32
const char	*Fl_File_Chooser::filesystems_label = "My Computer";
#else
const char	*Fl_File_Chooser::filesystems_label = "File Systems";
#endif // WIN32
const char	*Fl_File_Chooser::manage_favorites_label = "Manage Favorites";
const char	*Fl_File_Chooser::new_directory_label = "New Directory?";
const char	*Fl_File_Chooser::new_directory_tooltip = "Create a new directory.";
const char	*Fl_File_Chooser::preview_label = "Preview";
const char	*Fl_File_Chooser::save_label = "Save";
const char	*Fl_File_Chooser::show_label = "Show:";
Fl_File_Sort_F	*Fl_File_Chooser::sort = fl_numericsort;


//
// Local functions...
//

static int	compare_dirnames(const char *a, const char *b);
static void	quote_pathname(char *, const char *, int);
static void	unquote_pathname(char *, const char *, int);


//
// 'Fl_File_Chooser::count()' - Return the number of selected files.
//

int				// O - Number of selected files
Fl_File_Chooser::count() {
  int		i;		// Looping var
  int		fcount;		// Number of selected files
  const char	*filename;	// Filename in input field or list


  filename = fileName->value();

  if (!(type_ & MULTI)) {
    // Check to see if the file name input field is blank...
    if (!filename || !filename[0]) return 0;
    else return 1;
  }

  for (i = 1, fcount = 0; i <= fileList->size(); i ++)
    if (fileList->selected(i)) {
      // See if this file is a directory...
      // matt: why would we do that? It is perfectly legal to select multiple
      // directories in a DIR chooser. They are visually selected and value(i)
      // returns all of them as expected
      //filename = (char *)fileList->text(i);

      //if (filename[strlen(filename) - 1] != '/')
	fcount ++;
    }

  if (fcount) return fcount;
  else if (!filename || !filename[0]) return 0;
  else return 1;
}


//
// 'Fl_File_Chooser::directory()' - Set the directory in the file chooser.
//

void
Fl_File_Chooser::directory(const char *d)// I - Directory to change to
{
  char	*dirptr;			// Pointer into directory


//  printf("Fl_File_Chooser::directory(\"%s\")\n", d == NULL ? "(null)" : d);

  // NULL == current directory
  if (d == NULL)
    d = ".";

#ifdef WIN32
  // See if the filename contains backslashes...
  char	*slash;				// Pointer to slashes
  char	fixpath[1024];			// Path with slashes converted
  if (strchr(d, '\\')) {
    // Convert backslashes to slashes...
    strlcpy(fixpath, d, sizeof(fixpath));

    for (slash = strchr(fixpath, '\\'); slash; slash = strchr(slash + 1, '\\'))
      *slash = '/';

    d = fixpath;
  }
#endif // WIN32

  if (d[0] != '\0')
  {
    // Make the directory absolute...
#if (defined(WIN32) && ! defined(__CYGWIN__))|| defined(__EMX__)
    if (d[0] != '/' && d[0] != '\\' && d[1] != ':')
#else
    if (d[0] != '/' && d[0] != '\\')
#endif /* WIN32 || __EMX__ */
      fl_filename_absolute(directory_, d);
    else
      strlcpy(directory_, d, sizeof(directory_));

    // Strip any trailing slash...
    dirptr = directory_ + strlen(directory_) - 1;
    if ((*dirptr == '/' || *dirptr == '\\') && dirptr > directory_)
      *dirptr = '\0';

    // See if we have a trailing .. or . in the filename...
    dirptr = directory_ + strlen(directory_) - 3;
    if (dirptr >= directory_ && strcmp(dirptr, "/..") == 0) {
      // Yes, we have "..", so strip the trailing path...
      *dirptr = '\0';
      while (dirptr > directory_) {
        if (*dirptr == '/') break;
	dirptr --;
      }

      if (dirptr >= directory_ && *dirptr == '/')
        *dirptr = '\0';
    } else if ((dirptr + 1) >= directory_ && strcmp(dirptr + 1, "/.") == 0) {
      // Strip trailing "."...
      dirptr[1] = '\0';
    }
  }
  else
    directory_[0] = '\0';

  if (shown()) {
    // Rescan the directory...
    rescan();
  }
}


//
// 'Fl_File_Chooser::favoritesButtonCB()' - Handle favorites selections.
//

void
Fl_File_Chooser::favoritesButtonCB()
{
  int		v;			// Current selection
  char		pathname[1024],		// Pathname
		menuname[2048];		// Menu name


  v = favoritesButton->value();

  if (!v) {
    // Add current directory to favorites...
    if (getenv("HOME")) v = favoritesButton->size() - 5;
    else v = favoritesButton->size() - 4;

    sprintf(menuname, "favorite%02d", v);

    prefs_.set(menuname, directory_);
    prefs_.flush();

    quote_pathname(menuname, directory_, sizeof(menuname));
    favoritesButton->add(menuname);

    if (favoritesButton->size() > 104) {
      ((Fl_Menu_Item *)favoritesButton->menu())[0].deactivate();
    }
  } else if (v == 1) {
    // Manage favorites...
    favoritesCB(0);
  } else if (v == 2) {
    // Filesystems/My Computer
    directory("");
  } else {
    unquote_pathname(pathname, favoritesButton->text(v), sizeof(pathname));
    directory(pathname);
  }
}


//
// 'Fl_File_Chooser::favoritesCB()' - Handle favorites dialog.
//

void
Fl_File_Chooser::favoritesCB(Fl_Widget *w)
					// I - Widget
{
  int		i;			// Looping var
  char		name[32],		// Preference name
		pathname[1024];		// Directory in list


  if (!w) {
    // Load the favorites list...
    favList->clear();
    favList->deselect();

    for (i = 0; i < 100; i ++) {
      // Get favorite directory 0 to 99...
      sprintf(name, "favorite%02d", i);

      prefs_.get(name, pathname, "", sizeof(pathname));

      // Stop on the first empty favorite...
      if (!pathname[0]) break;

      // Add the favorite to the list...
      favList->add(pathname,
                   Fl_File_Icon::find(pathname, Fl_File_Icon::DIRECTORY));
    }

    favUpButton->deactivate();
    favDeleteButton->deactivate();
    favDownButton->deactivate();
    favOkButton->deactivate();

    favWindow->hotspot(favList);
    favWindow->show();
  } else if (w == favList) {
    i = favList->value();
    if (i) {
      if (i > 1) favUpButton->activate();
      else favUpButton->deactivate();

      favDeleteButton->activate();

      if (i < favList->size()) favDownButton->activate();
      else favDownButton->deactivate();
    } else {
      favUpButton->deactivate();
      favDeleteButton->deactivate();
      favDownButton->deactivate();
    }
  } else if (w == favUpButton) {
    i = favList->value();

    favList->insert(i - 1, favList->text(i), favList->data(i));
    favList->remove(i + 1);
    favList->select(i - 1);

    if (i == 2) favUpButton->deactivate();

    favDownButton->activate();

    favOkButton->activate();
  } else if (w == favDeleteButton) {
    i = favList->value();

    favList->remove(i);

    if (i > favList->size()) i --;
    favList->select(i);

    if (i < favList->size()) favDownButton->activate();
    else favDownButton->deactivate();

    if (i > 1) favUpButton->activate();
    else favUpButton->deactivate();

    if (!i) favDeleteButton->deactivate();

    favOkButton->activate();
  } else if (w == favDownButton) {
    i = favList->value();

    favList->insert(i + 2, favList->text(i), favList->data(i));
    favList->remove(i);
    favList->select(i + 1);

    if ((i + 1) == favList->size()) favDownButton->deactivate();

    favUpButton->activate();

    favOkButton->activate();
  } else if (w == favOkButton) {
    // Copy the new list over...
    for (i = 0; i < favList->size(); i ++) {
      // Set favorite directory 0 to 99...
      sprintf(name, "favorite%02d", i);

      prefs_.set(name, favList->text(i + 1));
    }

    // Clear old entries as necessary...
    for (; i < 100; i ++) {
      // Clear favorite directory 0 to 99...
      sprintf(name, "favorite%02d", i);

      prefs_.get(name, pathname, "", sizeof(pathname));

      if (pathname[0]) prefs_.set(name, "");
      else break;
    }

    update_favorites();
    prefs_.flush();

    favWindow->hide();
  }
}


//
// 'Fl_File_Chooser::fileListCB()' - Handle clicks (and double-clicks) in the
//                                   Fl_File_Browser.
//

void
Fl_File_Chooser::fileListCB()
{
  char	*filename,			// New filename
	pathname[1024];			// Full pathname to file


  filename = (char *)fileList->text(fileList->value());
  if (!filename)
    return;

  if (!directory_[0]) {
    strlcpy(pathname, filename, sizeof(pathname));
  } else if (strcmp(directory_, "/") == 0) {
    snprintf(pathname, sizeof(pathname), "/%s", filename);
  } else {
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, filename);
  }

  if (Fl::event_clicks()) {
#if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
    if ((strlen(pathname) == 2 && pathname[1] == ':') ||
        _fl_filename_isdir_quick(pathname))
#else
    if (_fl_filename_isdir_quick(pathname))
#endif /* WIN32 || __EMX__ */
    {
      // Change directories...
      directory(pathname);

      // Reset the click count so that a click in the same spot won't
      // be treated as a triple-click.  We use a value of -1 because
      // the next click will increment click count to 0, which is what
      // we really want...
      Fl::event_clicks(-1);
    }
    else
    {
      // Hide the window - picked the file...
      window->hide();
    }
  }
  else
  {
    // Check if the user clicks on a directory when picking files;
    // if so, make sure only that item is selected...
    filename = pathname + strlen(pathname) - 1;

    if ((type_ & MULTI) && !(type_ & DIRECTORY)) {
      if (*filename == '/') {
	// Clicked on a directory, deselect everything else...
	int i = fileList->value();
	fileList->deselect();
	fileList->select(i);
      } else {
        // Clicked on a file - see if there are other directories selected...
        int i;
	const char *temp;
	for (i = 1; i <= fileList->size(); i ++) {
	  if (i != fileList->value() && fileList->selected(i)) {
	    temp = fileList->text(i);
	    temp += strlen(temp) - 1;
	    if (*temp == '/') break;	// Yes, selected directory
	  }
	}

        if (i <= fileList->size()) {
	  i = fileList->value();
	  fileList->deselect();
	  fileList->select(i);
	}
      }
    }
    // Strip any trailing slash from the directory name...
    if (*filename == '/') *filename = '\0';

//    puts("Setting fileName from fileListCB...");
    fileName->value(pathname);

    // Update the preview box...
    Fl::remove_timeout((Fl_Timeout_Handler)previewCB, this);
    Fl::add_timeout(1.0, (Fl_Timeout_Handler)previewCB, this);

    // Do any callback that is registered...
    if (callback_) (*callback_)(this, data_);

    // Activate the OK button as needed...
    if (!_fl_filename_isdir_quick(pathname) || (type_ & DIRECTORY))
      okButton->activate();
    else
      okButton->deactivate();
  }
}


//
// 'Fl_File_Chooser::fileNameCB()' - Handle text entry in the FileBrowser.
//

void
Fl_File_Chooser::fileNameCB()
{
  char		*filename,	// New filename
		*slash,		// Pointer to trailing slash
		pathname[1024],	// Full pathname to file
		matchname[256];	// Matching filename
  int		i,		// Looping var
		min_match,	// Minimum number of matching chars
		max_match,	// Maximum number of matching chars
		num_files,	// Number of files in directory
		first_line;	// First matching line
  const char	*file;		// File from directory

//  puts("fileNameCB()");
//  printf("Event: %s\n", fl_eventnames[Fl::event()]);

  // Get the filename from the text field...
  filename = (char *)fileName->value();

  if (!filename || !filename[0]) {
    okButton->deactivate();
    return;
  }

  // Expand ~ and $ variables as needed...
  if (strchr(filename, '~') || strchr(filename, '$')) {
    fl_filename_expand(pathname, sizeof(pathname), filename);
    filename = pathname;
    value(pathname);
  }

  // Make sure we have an absolute path...
#if (defined(WIN32) && !defined(__CYGWIN__)) || defined(__EMX__)
  if (directory_[0] != '\0' && filename[0] != '/' &&
      filename[0] != '\\' &&
      !(isalpha(filename[0] & 255) && (!filename[1] || filename[1] == ':'))) {
#else
  if (directory_[0] != '\0' && filename[0] != '/') {
#endif /* WIN32 || __EMX__ */
    fl_filename_absolute(pathname, sizeof(pathname), filename);
    value(pathname);
    fileName->mark(fileName->position()); // no selection after expansion
  } else if (filename != pathname) {
    // Finally, make sure that we have a writable copy...
    strlcpy(pathname, filename, sizeof(pathname));
  }

  filename = pathname;

  // Now process things according to the key pressed...
  if (Fl::event_key() == FL_Enter || Fl::event_key() == FL_KP_Enter) {
    // Enter pressed - select or change directory...
#if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
    if ((isalpha(pathname[0] & 255) && pathname[1] == ':' && !pathname[2]) ||
        _fl_filename_isdir_quick(pathname) &&
	compare_dirnames(pathname, directory_)) {
#else
    if (_fl_filename_isdir_quick(pathname) &&
	compare_dirnames(pathname, directory_)) {
#endif /* WIN32 || __EMX__ */
      directory(pathname);
    } else if ((type_ & CREATE) || access(pathname, 0) == 0) {
      if (!_fl_filename_isdir_quick(pathname) || (type_ & DIRECTORY)) {
	// Update the preview box...
	update_preview();

	// Do any callback that is registered...
	if (callback_) (*callback_)(this, data_);

	// Hide the window to signal things are done...
	window->hide();
      }
    } else {
      // File doesn't exist, so beep at and alert the user...
      fl_alert(existing_file_label);
    }
  }
  else if (Fl::event_key() != FL_Delete &&
           Fl::event_key() != FL_BackSpace) {
    // Check to see if the user has entered a directory...
    if ((slash = strrchr(pathname, '/')) == NULL)
      slash = strrchr(pathname, '\\');

    if (!slash) return;

    // Yes, change directories if necessary...
    *slash++ = '\0';
    filename = slash;

#if defined(WIN32) || defined(__EMX__)
    if (strcasecmp(pathname, directory_) &&
        (pathname[0] || strcasecmp("/", directory_))) {
#else
    if (strcmp(pathname, directory_) &&
        (pathname[0] || strcasecmp("/", directory_))) {
#endif // WIN32 || __EMX__
      int p = fileName->position();
      int m = fileName->mark();

      directory(pathname);

      if (filename[0]) {
	char tempname[1024];

	snprintf(tempname, sizeof(tempname), "%s/%s", directory_, filename);
	fileName->value(tempname);
	strlcpy(pathname, tempname, sizeof(pathname));
      }

      fileName->position(p, m);
    }

    // Other key pressed - do filename completion as possible...
    num_files  = fileList->size();
    min_match  = strlen(filename);
    max_match  = min_match + 1;
    first_line = 0;

    for (i = 1; i <= num_files && max_match > min_match; i ++) {
      file = fileList->text(i);

#if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
      if (strncasecmp(filename, file, min_match) == 0) {
#else
      if (strncmp(filename, file, min_match) == 0) {
#endif // WIN32 || __EMX__
        // OK, this one matches; check against the previous match
	if (!first_line) {
	  // First match; copy stuff over...
	  strlcpy(matchname, file, sizeof(matchname));
	  max_match = strlen(matchname);

          // Strip trailing /, if any...
	  if (matchname[max_match - 1] == '/') {
	    max_match --;
	    matchname[max_match] = '\0';
	  }

	  // And then make sure that the item is visible
          fileList->topline(i);
	  first_line = i;
	} else {
	  // Succeeding match; compare to find maximum string match...
	  while (max_match > min_match)
#if (defined(WIN32) && ! defined(__CYGWIN__)) || defined(__EMX__)
	    if (strncasecmp(file, matchname, max_match) == 0)
#else
	    if (strncmp(file, matchname, max_match) == 0)
#endif // WIN32 || __EMX__
	      break;
	    else
	      max_match --;

          // Truncate the string as needed...
          matchname[max_match] = '\0';
	}
      }
    }

    // If we have any matches, add them to the input field...
    if (first_line > 0 && min_match == max_match &&
        max_match == (int)strlen(fileList->text(first_line))) {
      // This is the only possible match...
      fileList->deselect(0);
      fileList->select(first_line);
      fileList->redraw();
    } else if (max_match > min_match && first_line) {
      // Add the matching portion...
      fileName->replace(filename - pathname, filename - pathname + min_match,
                        matchname);

      // Highlight it with the cursor at the end of the selection so
      // s/he can press the right arrow to accept the selection
      // (Tab and End also do this for both cases.)
      fileName->position(filename - pathname + max_match,
	                 filename - pathname + min_match);
    } else if (max_match == 0) {
      fileList->deselect(0);
      fileList->redraw();
    }

    // See if we need to enable the OK button...
    if (((type_ & CREATE) || !access(fileName->value(), 0)) &&
        (!fl_filename_isdir(fileName->value()) || (type_ & DIRECTORY))) {
      okButton->activate();
    } else {
      okButton->deactivate();
    }
  } else {
    // FL_Delete or FL_BackSpace
    fileList->deselect(0);
    fileList->redraw();
    if (((type_ & CREATE) || !access(fileName->value(), 0)) &&
        (!fl_filename_isdir(fileName->value()) || (type_ & DIRECTORY))) {
      okButton->activate();
    } else {
      okButton->deactivate();
    }
  }
}


//
// 'Fl_File_Chooser::filter()' - Set the filter(s) for the chooser.
//

void
Fl_File_Chooser::filter(const char *p)		// I - Pattern(s)
{
  char		*copyp,				// Copy of pattern
		*start,				// Start of pattern
		*end;				// End of pattern
  int		allfiles;			// Do we have a "*" pattern?
  char		temp[1024];			// Temporary pattern string


  // Make sure we have a pattern...
  if (!p || !*p) p = "*";

  // Copy the pattern string...
  copyp = strdup(p);

  // Separate the pattern string as necessary...
  showChoice->clear();

  for (start = copyp, allfiles = 0; start && *start; start = end) {
    end = strchr(start, '\t');
    if (end) *end++ = '\0';

    if (strcmp(start, "*") == 0) {
      showChoice->add(all_files_label);
      allfiles = 1;
    } else {
      quote_pathname(temp, start, sizeof(temp));
      showChoice->add(temp);
      if (strstr(start, "(*)") != NULL) allfiles = 1;
    }
  }

  free(copyp);

  if (!allfiles) showChoice->add(all_files_label);

  showChoice->add(custom_filter_label);

  showChoice->value(0);
  showChoiceCB();
}


//
// 'Fl_File_Chooser::newdir()' - Make a new directory.
//

void
Fl_File_Chooser::newdir()
{
  const char	*dir;		// New directory name
  char		pathname[1024];	// Full path of directory


  // Get a directory name from the user
  if ((dir = fl_input(new_directory_label, NULL)) == NULL)
    return;

  // Make it relative to the current directory as needed...
#if (defined(WIN32) && ! defined (__CYGWIN__)) || defined(__EMX__)
  if (dir[0] != '/' && dir[0] != '\\' && dir[1] != ':')
#else
  if (dir[0] != '/' && dir[0] != '\\')
#endif /* WIN32 || __EMX__ */
    snprintf(pathname, sizeof(pathname), "%s/%s", directory_, dir);
  else
    strlcpy(pathname, dir, sizeof(pathname));

  // Create the directory; ignore EEXIST errors...
#if defined(WIN32) && ! defined (__CYGWIN__)
  if (mkdir(pathname))
#else
  if (mkdir(pathname, 0777))
#endif /* WIN32 */
    if (errno != EEXIST)
    {
      fl_alert("%s", strerror(errno));
      return;
    }

  // Show the new directory...
  directory(pathname);
}


//
// 'Fl_File_Chooser::preview()' - Enable or disable the preview tile.
//

void
Fl_File_Chooser::preview(int e)// I - 1 = enable preview, 0 = disable preview
{
  previewButton->value(e);
  prefs_.set("preview", e);
  prefs_.flush();

  Fl_Group *p = previewBox->parent();
  if (e) {
    int w = p->w() * 2 / 3;
    fileList->resize(fileList->x(), fileList->y(),
                     w, fileList->h());
    previewBox->resize(fileList->x()+w, previewBox->y(),
                       p->w()-w, previewBox->h());
    previewBox->show();
    update_preview();
  } else {
    fileList->resize(fileList->x(), fileList->y(),
                     p->w(), fileList->h());
    previewBox->resize(p->x()+p->w(), previewBox->y(),
                       0, previewBox->h());
    previewBox->hide();
  }
  p->init_sizes();

  fileList->parent()->redraw();
}


//
// 'Fl_File_Chooser::previewCB()' - Timeout handler for the preview box.
//

void
Fl_File_Chooser::previewCB(Fl_File_Chooser *fc) {	// I - File chooser
  fc->update_preview();
}


//
// 'Fl_File_Chooser::rescan()' - Rescan the current directory.
//

void
Fl_File_Chooser::rescan()
{
  char	pathname[1024];		// New pathname for filename field


  // Clear the current filename
  strlcpy(pathname, directory_, sizeof(pathname));
  if (pathname[0] && pathname[strlen(pathname) - 1] != '/') {
    strlcat(pathname, "/", sizeof(pathname));
  }
//  puts("Setting fileName in rescan()");
  fileName->value(pathname);

  if (type_ & DIRECTORY)
    okButton->activate();
  else
    okButton->deactivate();

  // Build the file list...
  fileList->load(directory_, sort);

  // Update the preview box...
  update_preview();
}

//
// 'Fl_File_Chooser::rescan_keep_filename()' - Rescan the current directory
// without clearing the filename, then select the file if it is in the list
//

void
Fl_File_Chooser::rescan_keep_filename()
{
  // if no filename was set, this is likely a diretory browser
  const char *fn = fileName->value();
  if (!fn || !*fn || fn[strlen(fn) - 1]=='/') {
    rescan();
    return;
  }

  int   i;
  char	pathname[1024];		// New pathname for filename field
  strlcpy(pathname, fn, sizeof(pathname));

  // Build the file list...
  fileList->load(directory_, sort);

  // Update the preview box...
  update_preview();

  // and select the chosen file
  char found = 0;
  char *slash = strrchr(pathname, '/');
  if (slash) 
    slash++;
  else
    slash = pathname;
  for (i = 1; i <= fileList->size(); i ++)
#if defined(WIN32) || defined(__EMX__)
    if (strcasecmp(fileList->text(i), slash) == 0) {
#else
    if (strcmp(fileList->text(i), slash) == 0) {
#endif // WIN32 || __EMX__
      fileList->topline(i);
      fileList->select(i);
      found = 1;
      break;
    }

  // update OK button activity
  if (found || type_ & CREATE)
    okButton->activate();
  else
    okButton->deactivate();
}


//
// 'Fl_File_Chooser::showChoiceCB()' - Handle show selections.
//

void
Fl_File_Chooser::showChoiceCB()
{
  const char	*item,			// Selected item
		*patstart;		// Start of pattern
  char		*patend;		// End of pattern
  char		temp[1024];		// Temporary string for pattern


  item = showChoice->text(showChoice->value());

  if (strcmp(item, custom_filter_label) == 0) {
    if ((item = fl_input(custom_filter_label, pattern_)) != NULL) {
      strlcpy(pattern_, item, sizeof(pattern_));

      quote_pathname(temp, item, sizeof(temp));
      showChoice->add(temp);
      showChoice->value(showChoice->size() - 2);
    }
  } else if ((patstart = strchr(item, '(')) == NULL) {
    strlcpy(pattern_, item, sizeof(pattern_));
  } else {
    strlcpy(pattern_, patstart + 1, sizeof(pattern_));
    if ((patend = strrchr(pattern_, ')')) != NULL) *patend = '\0';
  }

  fileList->filter(pattern_);

  if (shown()) {
    // Rescan the directory...
    rescan_keep_filename();
  }
}


//
// 'Fl_File_Chooser::update_favorites()' - Update the favorites menu.
//

void
Fl_File_Chooser::update_favorites()
{
  int		i;			// Looping var
  char		pathname[1024],		// Pathname
		menuname[2048];		// Menu name
  const char	*home;			// Home directory


  favoritesButton->clear();
  favoritesButton->add("bla");
  favoritesButton->clear();
  favoritesButton->add(add_favorites_label, FL_ALT + 'a', 0);
  favoritesButton->add(manage_favorites_label, FL_ALT + 'm', 0, 0, FL_MENU_DIVIDER);
  favoritesButton->add(filesystems_label, FL_ALT + 'f', 0);
    
  if ((home = getenv("HOME")) != NULL) {
    quote_pathname(menuname, home, sizeof(menuname));
    favoritesButton->add(menuname, FL_ALT + 'h', 0);
  }

  for (i = 0; i < 100; i ++) {
    sprintf(menuname, "favorite%02d", i);
    prefs_.get(menuname, pathname, "", sizeof(pathname));
    if (!pathname[0]) break;

    quote_pathname(menuname, pathname, sizeof(menuname));

    if (i < 10) favoritesButton->add(menuname, FL_ALT + '0' + i, 0);
    else favoritesButton->add(menuname);
  }

  if (i == 100) ((Fl_Menu_Item *)favoritesButton->menu())[0].deactivate();
}


//
// 'Fl_File_Chooser::update_preview()' - Update the preview box...
//

void
Fl_File_Chooser::update_preview()
{
  const char		*filename;	// Current filename
  Fl_Shared_Image	*image,		// New image
			*oldimage;	// Old image
  int			pbw, pbh;	// Width and height of preview box
  int			w, h;		// Width and height of preview image


  if (!previewButton->value()) return;

  if ((filename = value()) == NULL || fl_filename_isdir(filename)) image = NULL;
  else {
    window->cursor(FL_CURSOR_WAIT);
    Fl::check();

    image = Fl_Shared_Image::get(filename);

    if (image) {
      window->cursor(FL_CURSOR_DEFAULT);
      Fl::check();
    }
  }

  oldimage = (Fl_Shared_Image *)previewBox->image();

  if (oldimage) oldimage->release();

  previewBox->image(0);

  if (!image) {
    FILE	*fp;
    int		bytes;
    char	*ptr;

    if (filename) fp = fopen(filename, "rb");
    else fp = NULL;

    if (fp != NULL) {
      // Try reading the first 1k of data for a label...
      bytes = fread(preview_text_, 1, sizeof(preview_text_) - 1, fp);
      preview_text_[bytes] = '\0';
      fclose(fp);
    } else {
      // Assume we can't read any data...
      preview_text_[0] = '\0';
    }

    window->cursor(FL_CURSOR_DEFAULT);
    Fl::check();

    // Scan the buffer for printable chars...
    for (ptr = preview_text_;
         *ptr && (isprint(*ptr & 255) || isspace(*ptr & 255));
	 ptr ++);

    if (*ptr || ptr == preview_text_) {
      // Non-printable file, just show a big ?...
      previewBox->label(filename ? "?" : 0);
      previewBox->align(FL_ALIGN_CLIP);
      previewBox->labelsize(100);
      previewBox->labelfont(FL_HELVETICA);
    } else {
      // Show the first 1k of text...
      int size = previewBox->h() / 20;
      if (size < 6) size = 6;
      else if (size > 14) size = 14;

      previewBox->label(preview_text_);
      previewBox->align((Fl_Align)(FL_ALIGN_CLIP | FL_ALIGN_INSIDE |
                                   FL_ALIGN_LEFT | FL_ALIGN_TOP));
      previewBox->labelsize((uchar)size);
      previewBox->labelfont(FL_COURIER);
    }
  } else {
    pbw = previewBox->w() - 20;
    pbh = previewBox->h() - 20;

    if (image->w() > pbw || image->h() > pbh) {
      w   = pbw;
      h   = w * image->h() / image->w();

      if (h > pbh) {
	h = pbh;
	w = h * image->w() / image->h();
      }

      oldimage = (Fl_Shared_Image *)image->copy(w, h);
      previewBox->image((Fl_Image *)oldimage);

      image->release();
    } else {
      previewBox->image((Fl_Image *)image);
    }

    previewBox->align(FL_ALIGN_CLIP);
    previewBox->label(0);
  }

  previewBox->redraw();
}


//
// 'Fl_File_Chooser::value()' - Return a selected filename.
//

const char *			// O - Filename or NULL
Fl_File_Chooser::value(int f)	// I - File number
{
  int		i;		// Looping var
  int		fcount;		// Number of selected files
  const char	*name;		// Current filename
  static char	pathname[1024];	// Filename + directory


  name = fileName->value();

  if (!(type_ & MULTI)) {
    // Return the filename in the filename field...
    if (!name || !name[0]) return NULL;
    else return name;
  }

  // Return a filename from the list...
  for (i = 1, fcount = 0; i <= fileList->size(); i ++)
    if (fileList->selected(i)) {
      // See if this file is a selected file/directory...
      name = fileList->text(i);

      fcount ++;

      if (fcount == f) {
	if (directory_[0]) {
	  snprintf(pathname, sizeof(pathname), "%s/%s", directory_, name);
	} else {
	  strlcpy(pathname, name, sizeof(pathname));
	}

	return pathname;
      }
    }

  // If nothing is selected, use the filename field...
  if (!name || !name[0]) return NULL;
  else return name;
}


//
// 'Fl_File_Chooser::value()' - Set the current filename.
//

void
Fl_File_Chooser::value(const char *filename)
					// I - Filename + directory
{
  int	i,				// Looping var
  	fcount;				// Number of items in list
  char	*slash;				// Directory separator
  char	pathname[1024];			// Local copy of filename


//  printf("Fl_File_Chooser::value(\"%s\")\n", filename == NULL ? "(null)" : filename);

  // See if the filename is the "My System" directory...
  if (filename == NULL || !filename[0]) {
    // Yes, just change the current directory...
    directory(filename);
    fileName->value("");
    okButton->deactivate();
    return;
  }

#ifdef WIN32
  // See if the filename contains backslashes...
  char	fixpath[1024];			// Path with slashes converted
  if (strchr(filename, '\\')) {
    // Convert backslashes to slashes...
    strlcpy(fixpath, filename, sizeof(fixpath));

    for (slash = strchr(fixpath, '\\'); slash; slash = strchr(slash + 1, '\\'))
      *slash = '/';

    filename = fixpath;
  }
#endif // WIN32

  // See if there is a directory in there...
  fl_filename_absolute(pathname, sizeof(pathname), filename);

  if ((slash = strrchr(pathname, '/')) != NULL) {
    // Yes, change the display to the directory... 
    if (!fl_filename_isdir(pathname)) *slash++ = '\0';

    directory(pathname);
    if (*slash == '/') slash = pathname;
  } else {
    directory(".");
    slash = pathname;
  }

  // Set the input field to the absolute path...
  if (slash > pathname) slash[-1] = '/';

  fileName->value(pathname);
  fileName->position(0, strlen(pathname));
  okButton->activate();

  // Then find the file in the file list and select it...
  fcount = fileList->size();

  fileList->deselect(0);
  fileList->redraw();

  for (i = 1; i <= fcount; i ++)
#if defined(WIN32) || defined(__EMX__)
    if (strcasecmp(fileList->text(i), slash) == 0) {
#else
    if (strcmp(fileList->text(i), slash) == 0) {
#endif // WIN32 || __EMX__
//      printf("Selecting line %d...\n", i);
      fileList->topline(i);
      fileList->select(i);
      break;
    }
}


//
// 'compare_dirnames()' - Compare two directory names.
//

static int
compare_dirnames(const char *a, const char *b) {
  int alen, blen;

  // Get length of each string...
  alen = strlen(a) - 1;
  blen = strlen(b) - 1;

  if (alen < 0 || blen < 0) return alen - blen;

  // Check for trailing slashes...
  if (a[alen] != '/') alen ++;
  if (b[blen] != '/') blen ++;

  // If the lengths aren't the same, then return the difference...
  if (alen != blen) return alen - blen;

  // Do a comparison of the first N chars (alen == blen at this point)...
#ifdef WIN32
  return strncasecmp(a, b, alen);
#else
  return strncmp(a, b, alen);
#endif // WIN32
}


//
// 'quote_pathname()' - Quote a pathname for a menu.
//

static void
quote_pathname(char       *dst,		// O - Destination string
               const char *src,		// I - Source string
	       int        dstsize)	// I - Size of destination string
{
  dstsize --;

  while (*src && dstsize > 1) {
    if (*src == '\\') {
      // Convert backslash to forward slash...
      *dst++ = '\\';
      *dst++ = '/';
      src ++;
    } else {
      if (*src == '/') *dst++ = '\\';

      *dst++ = *src++;
    }
  }

  *dst = '\0';
}


//
// 'unquote_pathname()' - Unquote a pathname from a menu.
//

static void
unquote_pathname(char       *dst,	// O - Destination string
                 const char *src,	// I - Source string
	         int        dstsize)	// I - Size of destination string
{
  dstsize --;

  while (*src && dstsize > 1) {
    if (*src == '\\') src ++;
    *dst++ = *src++;
  }

  *dst = '\0';
}


//
// End of "$Id: Fl_File_Chooser2.cxx 6092 2008-04-11 12:57:37Z matt $".
//
