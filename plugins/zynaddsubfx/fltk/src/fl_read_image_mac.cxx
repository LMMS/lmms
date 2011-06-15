//
// "$Id: fl_read_image_mac.cxx 8362 2011-02-02 18:39:34Z manolo $"
//
// WIN32 image reading routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

#include <config.h>

//
// 'fl_read_image()' - Read an image from the current window or off-screen buffer.
//

uchar *				// O - Pixel buffer or NULL if failed
fl_read_image(uchar *p,		// I - Pixel buffer or NULL to allocate
              int   x,		// I - Left position
	      int   y,		// I - Top position
	      int   w,		// I - Width of area to read
	      int   h,		// I - Height of area to read
	      int   alpha) {	// I - Alpha value for image (0 for none)
  uchar *base;
  int rowBytes, delta;
  if(fl_window == NULL) { // reading from an offscreen buffer
    CGContextRef src = (CGContextRef)fl_gc;   // get bitmap context
    base = (uchar *)CGBitmapContextGetData(src);  // get data
    if(!base) return NULL;
    int sw = CGBitmapContextGetWidth(src);
    int sh = CGBitmapContextGetHeight(src);
    rowBytes = CGBitmapContextGetBytesPerRow(src);
    delta = CGBitmapContextGetBitsPerPixel(src)/8;
    if( (sw - x < w) || (sh - y < h) )  return NULL;
    }
  else { // reading from current window
    Fl_Window *window = Fl_Window::current();
    while(window->window()) window = window->window();
    base = Fl_X::bitmap_from_window_rect(window,x,y,w,h,&delta);
    rowBytes = delta*w;
    x = y = 0;
    }
  // Allocate the image data array as needed...
  int d = alpha ? 4 : 3;
  if (!p) p = new uchar[w * h * d];
  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);
  // Copy the image from the off-screen buffer to the memory buffer.
  int           idx, idy;	// Current X & Y in image
  uchar *pdst, *psrc;
  for (idy = y, pdst = p; idy < h + y; idy ++) {
    for (idx = 0, psrc = base + idy * rowBytes + x * delta; idx < w; idx ++, psrc += delta, pdst += d) {
      pdst[0] = psrc[0];  // R
      pdst[1] = psrc[1];  // G
      pdst[2] = psrc[2];  // B
    }
  }
  if(fl_window != NULL) delete[] base;
  return p;
}


//
// End of "$Id: fl_read_image_mac.cxx 8362 2011-02-02 18:39:34Z manolo $".
//
