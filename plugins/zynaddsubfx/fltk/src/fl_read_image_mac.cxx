//
// "$Id: fl_read_image_mac.cxx 5614 2007-01-18 15:25:09Z matt $"
//
// WIN32 image reading routines for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2006 by Bill Spitzak and others.
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

// warning: this function is only implemented in Quickdraw. The function
//          below may not work If FLTK is compiled with Quartz enabled

//
// 'fl_read_image()' - Read an image from the current window.
//

uchar *				// O - Pixel buffer or NULL if failed
fl_read_image(uchar *p,		// I - Pixel buffer or NULL to allocate
              int   x,		// I - Left position
	      int   y,		// I - Top position
	      int   w,		// I - Width of area to read
	      int   h,		// I - Height of area to read
	      int   alpha) {	// I - Alpha value for image (0 for none)
  Rect		src,		// Source rectangle
		dst;		// Destination rectangle
  GWorldPtr	osbuffer;	// Temporary off-screen buffer for copy
  GrafPtr	srcPort;	// Source port
  RGBColor	rgb;		// RGB colors for copy mask...
  PixMapHandle	pm;		// Pixmap handle for off-screen buffer
  uchar		*base,		// Base address of off-screen buffer
		*psrc,		// Pointer into off-screen buffer
		*pdst;		// Pointer into pixel buffer
  int           idx, idy;	// Current X & Y in image
  int		d;		// Depth of image
  int		rowBytes;	// Number of bytes per row...

  // Set the source and destination rectangles...
  src.top    = y;
  src.left   = x;
  src.bottom = y + h;
  src.right  = x + w;

  dst.top    = 0;
  dst.left   = 0;
  dst.bottom = h;
  dst.right  = w;

  // Get an off-screen buffer for copying the image...
  QDErr err = NewGWorld(&osbuffer, 0, &dst, 0L, 0L, 0);
  if (!osbuffer) return 0;
  if (err!=noErr) {
    DisposeGWorld(osbuffer);
    return 0;
  }

  // Get the source port...
  GetPort(&srcPort);

  // Set the RGB copy mask via the foreground/background colors...
  rgb.red   = 0xffff;
  rgb.green = 0xffff;
  rgb.blue  = 0xffff;
  RGBBackColor(&rgb);

  rgb.red   = 0x0000;
  rgb.green = 0x0000;
  rgb.blue  = 0x0000;
  RGBForeColor(&rgb);

  // Copy the screen image to the off-screen buffer...
  CopyBits(GetPortBitMapForCopyBits(srcPort),
           GetPortBitMapForCopyBits(osbuffer), &src, &dst, srcCopy, 0L);

  // Allocate the image data array as needed...
  d = alpha ? 4 : 3;

  if (!p) p = new uchar[w * h * d];

  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);

  // Set the correct port for the off-screen buffer and lock the buffer
  SetGWorld(osbuffer, 0);

  pm = GetGWorldPixMap(osbuffer);
  LockPixels(pm);

  base     = (uchar *)GetPixBaseAddr(pm);
  rowBytes = (*pm)->rowBytes & 0x3fff;

  // Copy the image from the off-screen buffer to the memory buffer.
  for (idy = 0, pdst = p; idy < h; idy ++)
#ifdef __i386__
    for (idx = 0, psrc = base + idy * rowBytes; idx < w; idx ++, psrc += 4, pdst += d) {
      pdst[0] = psrc[2];
      pdst[1] = psrc[1];
      pdst[2] = psrc[0];
    }
#else
    for (idx = 0, psrc = base + idy * rowBytes + 1; idx < w; idx ++, psrc += 4, pdst += d) {
      pdst[0] = psrc[0];
      pdst[1] = psrc[1];
      pdst[2] = psrc[2];
    }
#endif // __i386__
  // Unlock and delete the off-screen buffer, then return...
  UnlockPixels(pm);
  DisposeGWorld(osbuffer);

  SetPort(srcPort);
  return p;
}


//
// End of "$Id: fl_read_image_mac.cxx 5614 2007-01-18 15:25:09Z matt $".
//
