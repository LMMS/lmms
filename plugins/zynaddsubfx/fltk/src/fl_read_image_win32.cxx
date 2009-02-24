//
// "$Id: fl_read_image_win32.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// WIN32 image reading routines for the Fast Light Tool Kit (FLTK).
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

//
// 'fl_read_image()' - Read an image from the current window.
//

uchar *				// O - Pixel buffer or NULL if failed
fl_read_image(uchar *p,		// I - Pixel buffer or NULL to allocate
              int   X,		// I - Left position
	      int   Y,		// I - Top position
	      int   w,		// I - Width of area to read
	      int   h,		// I - Height of area to read
	      int   alpha) {	// I - Alpha value for image (0 for none)
  int	x, y;			// Looping vars
  int	d;			// Depth of image
  uchar	*ptr;			// Pointer in image data


  // Allocate the image data array as needed...
  d = alpha ? 4 : 3;

  if (!p) p = new uchar[w * h * d];

  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);

  // Grab all of the pixels in the image, one at a time...
  // MRS: there has to be a better way than this!
  for (y = 0, ptr = p; y < h; y ++) {
    for (x = 0; x < w; x ++, ptr += d) {
      COLORREF c = GetPixel(fl_gc, X + x, Y + y);

      ptr[0] = (uchar)c;
      c >>= 8;
      ptr[1] = (uchar)c;
      c >>= 8;
      ptr[2] = (uchar)c;
    }
  }

  return p;
}


//
// End of "$Id: fl_read_image_win32.cxx 5190 2006-06-09 16:16:34Z mike $".
//
