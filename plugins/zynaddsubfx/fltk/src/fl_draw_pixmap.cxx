//
// "$Id: fl_draw_pixmap.cxx 6026 2008-02-14 18:17:06Z matt $"
//
// Pixmap drawing code for the Fast Light Tool Kit (FLTK).
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

// Implemented without using the xpm library (which I can't use because
// it interferes with the color cube used by fl_draw_image).
// Current implementation is cheap and slow, and works best on a full-color
// display.  Transparency is not handled, and colors are dithered to
// the color cube.  Color index is achieved by adding the id
// characters together!  Also mallocs a lot of temporary memory!
// Notice that there is no pixmap file interface.  This is on purpose,
// as I want to discourage programs that require support files to work.
// All data needed by a program ui should be compiled in!!!

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <stdio.h>
#include "flstring.h"

static int ncolors, chars_per_pixel;

int fl_measure_pixmap(/*const*/ char* const* data, int &w, int &h) {
  return fl_measure_pixmap((const char*const*)data,w,h);
}

int fl_measure_pixmap(const char * const *data, int &w, int &h) {
  int i = sscanf(data[0],"%d%d%d%d",&w,&h,&ncolors,&chars_per_pixel);
  if (i<4 || w<=0 || h<=0 ||
      chars_per_pixel!=1 && chars_per_pixel!=2) return w=0;
  return 1;
}

#ifdef U64

// The callback from fl_draw_image to get a row of data passes this:
struct pixmap_data {
  int w, h;
  const uchar*const* data;
  union {
    U64 colors[256];
    U64* byte1[256];
  };
};

// callback for 1 byte per pixel:
static void cb1(void*v, int x, int y, int w, uchar* buf) {
  pixmap_data& d = *(pixmap_data*)v;
  const uchar* p = d.data[y]+x;
  U64* q = (U64*)buf;
  for (int X=w; X>0; X-=2, p += 2) {
    if (X>1) {
#  if WORDS_BIGENDIAN
      *q++ = (d.colors[p[0]]<<32) | d.colors[p[1]];
#  else
      *q++ = (d.colors[p[1]]<<32) | d.colors[p[0]];
#  endif
    } else {
#  if WORDS_BIGENDIAN
      *q++ = d.colors[p[0]]<<32;
#  else
      *q++ = d.colors[p[0]];
#  endif
    }
  }
}

// callback for 2 bytes per pixel:
static void cb2(void*v, int x, int y, int w, uchar* buf) {
  pixmap_data& d = *(pixmap_data*)v;
  const uchar* p = d.data[y]+2*x;
  U64* q = (U64*)buf;
  for (int X=w; X>0; X-=2) {
    U64* colors = d.byte1[*p++];
    int index = *p++;
    if (X>1) {
      U64* colors1 = d.byte1[*p++];
      int index1 = *p++;
#  if WORDS_BIGENDIAN
      *q++ = (colors[index]<<32) | colors1[index1];
#  else
      *q++ = (colors1[index1]<<32) | colors[index];
#  endif
    } else {
#  if WORDS_BIGENDIAN
      *q++ = colors[index]<<32;
#  else
      *q++ = colors[index];
#  endif
    }
  }
}

#else // U32

// The callback from fl_draw_image to get a row of data passes this:
struct pixmap_data {
  int w, h;
  const uchar*const* data;
  union {
    U32 colors[256];
    U32* byte1[256];
  };
};

#  ifndef __APPLE_QUARTZ__

// callback for 1 byte per pixel:
static void cb1(void*v, int x, int y, int w, uchar* buf) {
  pixmap_data& d = *(pixmap_data*)v;
  const uchar* p = d.data[y]+x;
  U32* q = (U32*)buf;
  for (int X=w; X--;) *q++ = d.colors[*p++];
}

// callback for 2 bytes per pixel:
static void cb2(void*v, int x, int y, int w, uchar* buf) {
  pixmap_data& d = *(pixmap_data*)v;
  const uchar* p = d.data[y]+2*x;
  U32* q = (U32*)buf;
  for (int X=w; X--;) {
    U32* colors = d.byte1[*p++];
    *q++ = colors[*p++];
  }
}

#  endif  // !__APPLE_QUARTZ__

#endif // U64 else U32

uchar **fl_mask_bitmap; // if non-zero, create bitmap and store pointer here

int fl_draw_pixmap(/*const*/ char* const* data, int x,int y,Fl_Color bg) {
  return fl_draw_pixmap((const char*const*)data,x,y,bg);
}

int fl_draw_pixmap(const char*const* di, int x, int y, Fl_Color bg) {
  pixmap_data d;
  if (!fl_measure_pixmap(di, d.w, d.h)) return 0;
  const uchar*const* data = (const uchar*const*)(di+1);
  int transparent_index = -1;

  if (ncolors < 0) {	// FLTK (non standard) compressed colormap
    ncolors = -ncolors;
    const uchar *p = *data++;
    // if first color is ' ' it is transparent (put it later to make
    // it not be transparent):
    if (*p == ' ') {
      uchar* c = (uchar*)&d.colors[(int)' '];
#ifdef U64
      *(U64*)c = 0;
#  if WORDS_BIGENDIAN
      c += 4;
#  endif
#endif
      transparent_index = ' ';
      Fl::get_color(bg, c[0], c[1], c[2]); c[3] = 0;
      p += 4;
      ncolors--;
    }
    // read all the rest of the colors:
    for (int i=0; i < ncolors; i++) {
      uchar* c = (uchar*)&d.colors[*p++];
#ifdef U64
      *(U64*)c = 0;
#  if WORDS_BIGENDIAN
      c += 4;
#  endif
#endif
      *c++ = *p++;
      *c++ = *p++;
      *c++ = *p++;
#ifdef __APPLE_QUARTZ__
      *c = 255;
#else
      *c = 0;
#endif
    }
  } else {	// normal XPM colormap with names
    if (chars_per_pixel>1) memset(d.byte1, 0, sizeof(d.byte1));
    for (int i=0; i<ncolors; i++) {
      const uchar *p = *data++;
      // the first 1 or 2 characters are the color index:
      int ind = *p++;
      uchar* c;
      if (chars_per_pixel>1) {
#ifdef U64
	U64* colors = d.byte1[ind];
	if (!colors) colors = d.byte1[ind] = new U64[256];
#else
	U32* colors = d.byte1[ind];
	if (!colors) colors = d.byte1[ind] = new U32[256];
#endif
	c = (uchar*)&colors[*p];
	ind = (ind<<8)|*p++;
      } else {
	c = (uchar *)&d.colors[ind];
      }
      // look for "c word", or last word if none:
      const uchar *previous_word = p;
      for (;;) {
	while (*p && isspace(*p)) p++;
	uchar what = *p++;
	while (*p && !isspace(*p)) p++;
	while (*p && isspace(*p)) p++;
	if (!*p) {p = previous_word; break;}
	if (what == 'c') break;
	previous_word = p;
	while (*p && !isspace(*p)) p++;
      }
#ifdef U64
      *(U64*)c = 0;
#  if WORDS_BIGENDIAN
      c += 4;
#  endif
#endif
#ifdef __APPLE_QUARTZ__
      c[3] = 255;
#endif
      if (!fl_parse_color((const char*)p, c[0], c[1], c[2])) {
        // assume "None" or "#transparent" for any errors
	// "bg" should be transparent...
	Fl::get_color(bg, c[0], c[1], c[2]);
#ifdef __APPLE_QUARTZ__
        c[3] = 0;
#endif
	transparent_index = ind;
      }
    }
  }
  d.data = data;

#ifndef __APPLE_QUARTZ__

  // build the mask bitmap used by Fl_Pixmap:
  if (fl_mask_bitmap && transparent_index >= 0) {
    int W = (d.w+7)/8;
    uchar* bitmap = new uchar[W * d.h];
    *fl_mask_bitmap = bitmap;
    for (int Y = 0; Y < d.h; Y++) {
      const uchar* p = data[Y];
      if (chars_per_pixel <= 1) {
	int dw = d.w;
	for (int X = 0; X < W; X++) {
	  uchar b = (dw-->0 && *p++ != transparent_index);
	  if (dw-->0 && *p++ != transparent_index) b |= 2;
	  if (dw-->0 && *p++ != transparent_index) b |= 4;
	  if (dw-->0 && *p++ != transparent_index) b |= 8;
	  if (dw-->0 && *p++ != transparent_index) b |= 16;
	  if (dw-->0 && *p++ != transparent_index) b |= 32;
	  if (dw-->0 && *p++ != transparent_index) b |= 64;
	  if (dw-->0 && *p++ != transparent_index) b |= 128;
	  *bitmap++ = b;
	}
      } else {
        uchar b = 0, bit = 1;
	for (int X = 0; X < d.w; X++) {
	  int ind = *p++;
	  ind = (ind<<8) | (*p++);
	  if (ind != transparent_index) b |= bit;

          if (bit < 128) bit <<= 1;
	  else {
	    *bitmap++ = b;
	    b = 0;
	    bit = 1;
	  }
	}

        if (bit > 1) *bitmap++ = b;
      }
    }
  }

  fl_draw_image(chars_per_pixel==1 ? cb1 : cb2, &d, x, y, d.w, d.h, 4);

#else // __APPLE_QUARTZ__

  bool transparent = (transparent_index>=0);
  transparent = true;
  U32 *array = new U32[d.w * d.h], *q = array;
  for (int Y = 0; Y < d.h; Y++) {
    const uchar* p = data[Y];
    if (chars_per_pixel <= 1) {
      for (int X = 0; X < d.w; X++) {
        *q++ = d.colors[*p++];
      }
    } else {
      for (int X = 0; X < d.w; X++) {
        U32* colors = d.byte1[*p++];
        *q++ = colors[*p++];
      }
    }
  }
  CGColorSpaceRef lut = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef src = CGDataProviderCreateWithData( 0L, array, d.w * d.h * 4, 0L);
  CGImageRef img = CGImageCreate(d.w, d.h, 8, 4*8, 4*d.w,
        lut, transparent?kCGImageAlphaLast:kCGImageAlphaNoneSkipLast,
        src, 0L, false, kCGRenderingIntentDefault);
  CGColorSpaceRelease(lut);
  CGDataProviderRelease(src);
  CGRect rect = { { x, y} , { d.w, d.h } };
  Fl_X::q_begin_image(rect, 0, 0, d.w, d.h);
  CGContextDrawImage(fl_gc, rect, img);
  Fl_X::q_end_image();
  CGImageRelease(img);
  delete array;

#endif // !__APPLE_QUARTZ__

  if (chars_per_pixel > 1) for (int i = 0; i < 256; i++) delete[] d.byte1[i];
  return 1;
}

//
// End of "$Id: fl_draw_pixmap.cxx 6026 2008-02-14 18:17:06Z matt $".
//
