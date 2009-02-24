//
// "$Id: Fl_File_Browser.H 4288 2005-04-16 00:13:17Z mike $"
//
// FileBrowser definitions.
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

//
// Include necessary header files...
//

#ifndef _Fl_File_Browser_H_
#  define _Fl_File_Browser_H_

#  include "Fl_Browser.H"
#  include "Fl_File_Icon.H"
#  include "filename.H"


//
// Fl_File_Browser class...
//

class FL_EXPORT Fl_File_Browser : public Fl_Browser
{
  int		filetype_;
  const char	*directory_;
  uchar		iconsize_;
  const char	*pattern_;

  int		full_height() const;
  int		item_height(void *) const;
  int		item_width(void *) const;
  void		item_draw(void *, int, int, int, int) const;
  int		incr_height() const { return (item_height(0)); }

public:
  enum { FILES, DIRECTORIES };

  Fl_File_Browser(int, int, int, int, const char * = 0);

  uchar		iconsize() const { return (iconsize_); };
  void		iconsize(uchar s) { iconsize_ = s; redraw(); };

  void	filter(const char *pattern);
  const char	*filter() const { return (pattern_); };

  int		load(const char *directory, Fl_File_Sort_F *sort = fl_numericsort);

  uchar		textsize() const { return (Fl_Browser::textsize()); };
  void		textsize(uchar s) { Fl_Browser::textsize(s); iconsize_ = (uchar)(3 * s / 2); };

  int		filetype() const { return (filetype_); };
  void		filetype(int t) { filetype_ = t; };
};

#endif // !_Fl_File_Browser_H_

//
// End of "$Id: Fl_File_Browser.H 4288 2005-04-16 00:13:17Z mike $".
//
