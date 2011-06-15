//
// "$Id: Fl_File_Icon.H 8306 2011-01-24 17:04:22Z matt $"
//
// Fl_File_Icon definitions.
//
// Copyright 1999-2010 by Michael Sweet.
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
   Fl_File_Icon widget . */

//
// Include necessary header files...
//

#ifndef _Fl_Fl_File_Icon_H_
#  define _Fl_Fl_File_Icon_H_

#  include "Fl.H"


//
// Special color value for the icon color.
//

#  define FL_ICON_COLOR (Fl_Color)0xffffffff	/**< icon color [background?]*/


//
// Fl_File_Icon class...
//

/** 
  The Fl_File_Icon class manages icon images that can be used 
  as labels in other widgets and as icons in the FileBrowser widget.
*/
class FL_EXPORT Fl_File_Icon {			//// Icon data

  static Fl_File_Icon *first_;	// Pointer to first icon/filetype
  Fl_File_Icon	*next_;		// Pointer to next icon/filetype
  const char	*pattern_;	// Pattern string
  int		type_;		// Match only if directory or file?
  int		num_data_;	// Number of data elements
  int		alloc_data_;	// Number of allocated elements
  short		*data_;		// Icon data

  public:

  enum				// File types
  {
    ANY,			// Any kind of file
    PLAIN,			// Only plain files
    FIFO,			// Only named pipes
    DEVICE,			// Only character and block devices
    LINK,			// Only symbolic links
    DIRECTORY			// Only directories
  };

  enum				// Data opcodes
  {
    END,			// End of primitive/icon
    COLOR,			// Followed by color value (2 shorts)
    LINE,			// Start of line
    CLOSEDLINE,			// Start of closed line
    POLYGON,			// Start of polygon
    OUTLINEPOLYGON,		// Followed by outline color (2 shorts)
    VERTEX			// Followed by scaled X,Y
  };

  Fl_File_Icon(const char *p, int t, int nd = 0, short *d = 0);
  ~Fl_File_Icon();

  short		*add(short d);

  /**
    Adds a color value to the icon array, returning a pointer to it.
    \param[in] c color value
  */
  short		*add_color(Fl_Color c)
		{ short *d = add((short)COLOR); add((short)(c >> 16)); add((short)c); return (d); }

  /**
    Adds a vertex value to the icon array, returning a pointer to it.
    The integer version accepts coordinates from 0 to 10000.
    The origin (0.0) is in the lower-lefthand corner of the icon.
    \param[in] x, y vertex coordinates
  */
  short		*add_vertex(int x, int y)
		{ short *d = add((short)VERTEX); add((short)x); add((short)y); return (d); }

  /**
    Adds a vertex value to the icon array, returning a pointer to it.
    The floating point version goes from 0.0 to 1.0.
    The origin (0.0) is in the lower-lefthand corner of the icon.
    \param[in] x, y vertex coordinates
  */
  short		*add_vertex(float x, float y)
		{ short *d = add((short)VERTEX); add((short)(x * 10000.0));
		  add((short)(y * 10000.0)); return (d); }

  /** Clears all icon data from the icon.*/
  void		clear() { num_data_ = 0; }

  void		draw(int x, int y, int w, int h, Fl_Color ic, int active = 1);

  void		label(Fl_Widget *w);

  static void	labeltype(const Fl_Label *o, int x, int y, int w, int h, Fl_Align a);
  void		load(const char *f);
  int		load_fti(const char *fti);
  int		load_image(const char *i);

  /** Returns next file icon object. See Fl_File_Icon::first() */
  Fl_File_Icon	*next() { return (next_); }

  /** Returns the filename matching pattern for the icon.*/
  const char	*pattern() { return (pattern_); }

  /**  Returns the number of words of data used by the icon.*/
  int		size() { return (num_data_); }

  /**
    Returns the filetype associated with the icon, which can be one of the
    following:
  
    \li Fl_File_Icon::ANY, any kind of file.
    \li Fl_File_Icon::PLAIN, plain files.
    \li Fl_File_Icon::FIFO, named pipes.
    \li Fl_File_Icon::DEVICE, character and block devices.
    \li Fl_File_Icon::LINK, symbolic links.
    \li Fl_File_Icon::DIRECTORY, directories.
  */
  int		type() { return (type_); }

  /**  Returns the data array for the icon.*/
  short		*value() { return (data_); }

  static Fl_File_Icon *find(const char *filename, int filetype = ANY);

  /** Returns a pointer to the first icon in the list.*/
  static Fl_File_Icon *first() { return (first_); }
  static void	load_system_icons(void);
};

#endif // !_Fl_Fl_File_Icon_H_

//
// End of "$Id: Fl_File_Icon.H 8306 2011-01-24 17:04:22Z matt $".
//
