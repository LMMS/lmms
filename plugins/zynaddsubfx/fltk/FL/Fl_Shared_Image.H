//
// "$Id: Fl_Shared_Image.H 4288 2005-04-16 00:13:17Z mike $"
//
// Shared image header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Shared_Image_H
#  define Fl_Shared_Image_H

#  include "Fl_Image.H"


// Test function for adding new formats
typedef Fl_Image *(*Fl_Shared_Handler)(const char *name, uchar *header,
                                       int headerlen);

// Shared images class. 
class FL_EXPORT Fl_Shared_Image : public Fl_Image {
  protected:

  static Fl_Shared_Image **images_;	// Shared images
  static int	num_images_;		// Number of shared images
  static int	alloc_images_;		// Allocated shared images
  static Fl_Shared_Handler *handlers_;	// Additional format handlers
  static int	num_handlers_;		// Number of format handlers
  static int	alloc_handlers_;	// Allocated format handlers

  const char	*name_;			// Name of image file
  int		original_;		// Original image?
  int		refcount_;		// Number of times this image has been used
  Fl_Image	*image_;		// The image that is shared
  int		alloc_image_;		// Was the image allocated?

  static int	compare(Fl_Shared_Image **i0, Fl_Shared_Image **i1);

  // Use get() and release() to load/delete images in memory...
  Fl_Shared_Image();
  Fl_Shared_Image(const char *n, Fl_Image *img = 0);
  virtual ~Fl_Shared_Image();
  void add();
  void update();

  public:

  const char	*name() { return name_; }
  int		refcount() { return refcount_; }
  void		release();
  void		reload();

  virtual Fl_Image *copy(int W, int H);
  Fl_Image *copy() { return copy(w(), h()); }
  virtual void color_average(Fl_Color c, float i);
  virtual void desaturate();
  virtual void draw(int X, int Y, int W, int H, int cx, int cy);
  void draw(int X, int Y) { draw(X, Y, w(), h(), 0, 0); }
  virtual void uncache();

  static Fl_Shared_Image *find(const char *n, int W = 0, int H = 0);
  static Fl_Shared_Image *get(const char *n, int W = 0, int H = 0);
  static Fl_Shared_Image **images();
  static int		num_images();
  static void		add_handler(Fl_Shared_Handler f);
  static void		remove_handler(Fl_Shared_Handler f);
};

//
// The following function is provided in the fltk_images library and
// registers all of the "extra" image file formats that are not part
// of the core FLTK library...
//

FL_EXPORT extern void fl_register_images();

#endif // !Fl_Shared_Image_H

//
// End of "$Id: Fl_Shared_Image.H 4288 2005-04-16 00:13:17Z mike $"
//
