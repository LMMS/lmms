//
// "$Id: fl_read_image_win32.cxx 8595 2011-04-17 08:48:40Z manolo $"
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

  int	d;			// Depth of image

  // Allocate the image data array as needed...
  d = alpha ? 4 : 3;

  if (!p) p = new uchar[w * h * d];

  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);

  // Grab all of the pixels in the image...

  // Assure that we are not trying to read non-existing data. If it is so, the
  // function should still work, but the out-of-bounds part of the image is
  // untouched (initialized with the alpha value or 0 (black), resp.).

  int ww = w; // We need the original width for output data line size

  int shift_x = 0; // X target shift if X modified
  int shift_y = 0; // Y target shift if X modified

  if (X < 0) {
    shift_x = -X;
    w += X;
    X = 0;
  }
  if (Y < 0) {
    shift_y = -Y;
    h += Y;
    Y = 0;
  }

  if (h < 1 || w < 1) return p;		// nothing to copy

  int line_size = ((3*w+3)/4) * 4;	// each line is aligned on a DWORD (4 bytes)
  uchar *dib = new uchar[line_size*h];	// create temporary buffer to read DIB

  // fill in bitmap info for GetDIBits

  BITMAPINFO   bi;
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;		// negative => top-down DIB
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 24;		// 24 bits RGB
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biSizeImage = 0;
  bi.bmiHeader.biXPelsPerMeter = 0;
  bi.bmiHeader.biYPelsPerMeter = 0;
  bi.bmiHeader.biClrUsed = 0;
  bi.bmiHeader.biClrImportant = 0;

  // copy bitmap from original DC (Window, Fl_Offscreen, ...)

  HDC hdc = CreateCompatibleDC(fl_gc);
  HBITMAP hbm = CreateCompatibleBitmap(fl_gc,w,h);

  int save_dc = SaveDC(hdc);			// save context for cleanup
  SelectObject(hdc,hbm);			// select bitmap
  BitBlt(hdc,0,0,w,h,fl_gc,X,Y,SRCCOPY);	// copy image section to DDB

  // copy RGB image data to the allocated DIB

  GetDIBits(hdc, hbm, 0, h, dib, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

  // finally copy the image data to the user buffer

  for (int j = 0; j<h; j++) {
    const uchar *src = dib + j * line_size;			// source line
    uchar *tg = p + (j + shift_y) * d * ww + shift_x * d;	// target line
    for (int i = 0; i<w; i++) {
      uchar b = *src++;
      uchar g = *src++;
      *tg++ = *src++;	// R
      *tg++ = g;	// G
      *tg++ = b;	// B
      if (alpha)
	*tg++ = alpha;	// alpha
    }
  }

  // free used GDI and other structures

  RestoreDC(hdc,save_dc);	// reset DC
  DeleteDC(hdc);
  DeleteObject(hbm);
  delete[] dib;		// delete DIB temporary buffer

  return p;
}

//
// End of "$Id: fl_read_image_win32.cxx 8595 2011-04-17 08:48:40Z manolo $".
//
