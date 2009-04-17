//
// "$Id: fl_read_image.cxx 6065 2008-03-09 17:58:10Z matt $"
//
// X11 image reading routines for the Fast Light Tool Kit (FLTK).
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

#include <FL/x.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include "flstring.h"

#ifdef DEBUG
#  include <stdio.h>
#endif // DEBUG

#ifdef WIN32
#  include "fl_read_image_win32.cxx"
#elif defined(__APPLE__)
#  include "fl_read_image_mac.cxx"
#else
#  include <X11/Xutil.h>
#  ifdef __sgi
#    include <X11/extensions/readdisplay.h>
#  else
#    include <stdlib.h>
#  endif // __sgi

// Defined in fl_color.cxx
extern uchar fl_redmask, fl_greenmask, fl_bluemask;
extern int fl_redshift, fl_greenshift, fl_blueshift, fl_extrashift;

//
// 'fl_subimage_offsets()' - Calculate subimage offsets for an axis
static inline int
fl_subimage_offsets(int a, int aw, int b, int bw, int &obw)
{
  int off;
  int ob;

  if (b >= a) {
    ob = b;
    off = 0;
  } else {
    ob = a;
    off = a - b;
  }

  bw -= off;

  if (ob + bw <= a + aw) {
    obw = bw;
  } else {
    obw = (a + aw) - ob;
  }

  return off;
}


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
  XImage	*image;		// Captured image
  int		i, maxindex;	// Looping vars
  int           x, y;		// Current X & Y in image
  int		d;		// Depth of image
  unsigned char *line,		// Array to hold image row
		*line_ptr;	// Pointer to current line image
  unsigned char	*pixel;		// Current color value
  XColor	colors[4096];	// Colors from the colormap...
  unsigned char	cvals[4096][3];	// Color values from the colormap...
  unsigned	index_mask,
		index_shift,
		red_mask,
		red_shift,
		green_mask,
		green_shift,
		blue_mask,
		blue_shift;


  //
  // Under X11 we have the option of the XGetImage() interface or SGI's
  // ReadDisplay extension which does all of the really hard work for
  // us...
  //

#  ifdef __sgi
  if (XReadDisplayQueryExtension(fl_display, &i, &i)) {
    image = XReadDisplay(fl_display, fl_window, X, Y, w, h, 0, NULL);
  } else
#  else
  image = 0;
#  endif // __sgi

  if (!image) {
    // fetch absolute coordinates
    int dx, dy, sx, sy, sw, sh;
    Window child_win;
    Fl_Window *win = fl_find(fl_window);
    if (win) {
      XTranslateCoordinates(fl_display, fl_window,
          RootWindow(fl_display, fl_screen), X, Y, &dx, &dy, &child_win);
      // screen dimensions
      Fl::screen_xywh(sx, sy, sw, sh, fl_screen);
    }
    if (!win || (dx >= sx && dy >= sy && dx + w <= sw && dy + h <= sh)) {
      // the image is fully contained, we can use the traditional method
      image = XGetImage(fl_display, fl_window, X, Y, w, h, AllPlanes, ZPixmap);
    } else {
      // image is crossing borders, determine visible region
      int nw, nh, noffx, noffy;
      noffx = fl_subimage_offsets(sx, sw, dx, w, nw);
      noffy = fl_subimage_offsets(sy, sh, dy, h, nh);
      if (nw <= 0 || nh <= 0) return 0;

      // allocate the image
      int bpp = fl_visual->depth + ((fl_visual->depth / 8) % 2) * 8;
      char* buf = (char*)malloc(bpp / 8 * w * h);
      image = XCreateImage(fl_display, fl_visual->visual,
	  fl_visual->depth, ZPixmap, 0, buf, w, h, bpp, 0);
      if (!image) {
	if (buf) free(buf);
	return 0;
  }

      if (!XGetSubImage(fl_display, fl_window, X + noffx, Y + noffy,
	      nw, nh, AllPlanes, ZPixmap, image, noffx, noffy)) {
	XDestroyImage(image);
	return 0;
      }
    }
  }

  if (!image) return 0;

#ifdef DEBUG
  printf("width            = %d\n", image->width);
  printf("height           = %d\n", image->height);
  printf("xoffset          = %d\n", image->xoffset);
  printf("format           = %d\n", image->format);
  printf("data             = %p\n", image->data);
  printf("byte_order       = %d\n", image->byte_order);
  printf("bitmap_unit      = %d\n", image->bitmap_unit);
  printf("bitmap_bit_order = %d\n", image->bitmap_bit_order);
  printf("bitmap_pad       = %d\n", image->bitmap_pad);
  printf("depth            = %d\n", image->depth);
  printf("bytes_per_line   = %d\n", image->bytes_per_line);
  printf("bits_per_pixel   = %d\n", image->bits_per_pixel);
  printf("red_mask         = %08x\n", image->red_mask);
  printf("green_mask       = %08x\n", image->green_mask);
  printf("blue_mask        = %08x\n", image->blue_mask);
  printf("map_entries      = %d\n", fl_visual->visual->map_entries);
#endif // DEBUG

  d = alpha ? 4 : 3;

  // Allocate the image data array as needed...
  if (!p) p = new uchar[w * h * d];

  // Initialize the default colors/alpha in the whole image...
  memset(p, alpha, w * h * d);

  // Check that we have valid mask/shift values...
  if (!image->red_mask && image->bits_per_pixel > 12) {
    // Greater than 12 bits must be TrueColor...
    image->red_mask   = fl_visual->visual->red_mask;
    image->green_mask = fl_visual->visual->green_mask;
    image->blue_mask  = fl_visual->visual->blue_mask;

#ifdef DEBUG
    puts("\n---- UPDATED ----");
    printf("fl_redmask       = %08x\n", fl_redmask);
    printf("fl_redshift      = %d\n", fl_redshift);
    printf("fl_greenmask     = %08x\n", fl_greenmask);
    printf("fl_greenshift    = %d\n", fl_greenshift);
    printf("fl_bluemask      = %08x\n", fl_bluemask);
    printf("fl_blueshift     = %d\n", fl_blueshift);
    printf("red_mask         = %08x\n", image->red_mask);
    printf("green_mask       = %08x\n", image->green_mask);
    printf("blue_mask        = %08x\n", image->blue_mask);
#endif // DEBUG
  }

  // Check if we have colormap image...
  if (!image->red_mask) {
    // Get the colormap entries for this window...
    maxindex = fl_visual->visual->map_entries;

    for (i = 0; i < maxindex; i ++) colors[i].pixel = i;

    XQueryColors(fl_display, fl_colormap, colors, maxindex);

    for (i = 0; i < maxindex; i ++) {
      cvals[i][0] = colors[i].red >> 8;
      cvals[i][1] = colors[i].green >> 8;
      cvals[i][2] = colors[i].blue >> 8;
    }

    // Read the pixels and output an RGB image...
    for (y = 0; y < image->height; y ++) {
      pixel = (unsigned char *)(image->data + y * image->bytes_per_line);
      line  = p + y * w * d;

      switch (image->bits_per_pixel) {
        case 1 :
	  for (x = image->width, line_ptr = line, index_mask = 128;
	       x > 0;
	       x --, line_ptr += d) {
	    if (*pixel & index_mask) {
	      line_ptr[0] = cvals[1][0];
	      line_ptr[1] = cvals[1][1];
	      line_ptr[2] = cvals[1][2];
            } else {
	      line_ptr[0] = cvals[0][0];
	      line_ptr[1] = cvals[0][1];
	      line_ptr[2] = cvals[0][2];
            }

            if (index_mask > 1) {
	      index_mask >>= 1;
	    } else {
              index_mask = 128;
              pixel ++;
            }
	  }
          break;

        case 2 :
	  for (x = image->width, line_ptr = line, index_shift = 6;
	       x > 0;
	       x --, line_ptr += d) {
	    i = (*pixel >> index_shift) & 3;

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift > 0) {
              index_mask >>= 2;
              index_shift -= 2;
            } else {
              index_mask  = 192;
              index_shift = 6;
              pixel ++;
            }
	  }
          break;

        case 4 :
	  for (x = image->width, line_ptr = line, index_shift = 4;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 4) i = (*pixel >> 4) & 15;
	    else i = *pixel & 15;

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift > 0) {
              index_shift = 0;
	    } else {
              index_shift = 4;
              pixel ++;
            }
	  }
          break;

        case 8 :
	  for (x = image->width, line_ptr = line;
	       x > 0;
	       x --, line_ptr += d, pixel ++) {
	    line_ptr[0] = cvals[*pixel][0];
	    line_ptr[1] = cvals[*pixel][1];
	    line_ptr[2] = cvals[*pixel][2];
	  }
          break;

        case 12 :
	  for (x = image->width, line_ptr = line, index_shift = 0;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 0) {
	      i = ((pixel[0] << 4) | (pixel[1] >> 4)) & 4095;
	    } else {
	      i = ((pixel[1] << 8) | pixel[2]) & 4095;
	    }

	    line_ptr[0] = cvals[i][0];
	    line_ptr[1] = cvals[i][1];
	    line_ptr[2] = cvals[i][2];

            if (index_shift == 0) {
              index_shift = 4;
            } else {
              index_shift = 0;
              pixel += 3;
            }
	  }
          break;
      }
    }
  } else {
    // RGB(A) image, so figure out the shifts & masks...
    red_mask  = image->red_mask;
    red_shift = 0;

    while ((red_mask & 1) == 0) {
      red_mask >>= 1;
      red_shift ++;
    }

    green_mask  = image->green_mask;
    green_shift = 0;

    while ((green_mask & 1) == 0) {
      green_mask >>= 1;
      green_shift ++;
    }

    blue_mask  = image->blue_mask;
    blue_shift = 0;

    while ((blue_mask & 1) == 0) {
      blue_mask >>= 1;
      blue_shift ++;
    }

    // Read the pixels and output an RGB image...
    for (y = 0; y < image->height; y ++) {
      pixel = (unsigned char *)(image->data + y * image->bytes_per_line);
      line  = p + y * w * d;

      switch (image->bits_per_pixel) {
        case 8 :
	  for (x = image->width, line_ptr = line;
	       x > 0;
	       x --, line_ptr += d, pixel ++) {
	    i = *pixel;

	    line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	    line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	    line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	  }
          break;

        case 12 :
	  for (x = image->width, line_ptr = line, index_shift = 0;
	       x > 0;
	       x --, line_ptr += d) {
	    if (index_shift == 0) {
	      i = ((pixel[0] << 4) | (pixel[1] >> 4)) & 4095;
	    } else {
	      i = ((pixel[1] << 8) | pixel[2]) & 4095;
            }

	    line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	    line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	    line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;

            if (index_shift == 0) {
              index_shift = 4;
            } else {
              index_shift = 0;
              pixel += 3;
            }
	  }
          break;

        case 16 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 2) {
	      i = (pixel[1] << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 2) {
	      i = (pixel[0] << 8) | pixel[1];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;

        case 24 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 3) {
	      i = (((pixel[2] << 8) | pixel[1]) << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 3) {
	      i = (((pixel[0] << 8) | pixel[1]) << 8) | pixel[2];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;

        case 32 :
          if (image->byte_order == LSBFirst) {
            // Little-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 4) {
	      i = (((((pixel[3] << 8) | pixel[2]) << 8) | pixel[1]) << 8) | pixel[0];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  } else {
            // Big-endian...
	    for (x = image->width, line_ptr = line;
	         x > 0;
	         x --, line_ptr += d, pixel += 4) {
	      i = (((((pixel[0] << 8) | pixel[1]) << 8) | pixel[2]) << 8) | pixel[3];

	      line_ptr[0] = 255 * ((i >> red_shift) & red_mask) / red_mask;
	      line_ptr[1] = 255 * ((i >> green_shift) & green_mask) / green_mask;
	      line_ptr[2] = 255 * ((i >> blue_shift) & blue_mask) / blue_mask;
	    }
	  }
          break;
      }
    }
  }

  // Destroy the X image we've read and return the RGB(A) image...
  XDestroyImage(image);

  return p;
}

#endif

//
// End of "$Id: fl_read_image.cxx 6065 2008-03-09 17:58:10Z matt $".
//
