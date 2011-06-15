//
// "$Id: fl_draw_pixmap.cxx 8362 2011-02-02 18:39:34Z manolo $"
//
// Pixmap drawing code for the Fast Light Tool Kit (FLTK).
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

/**
  Get the dimensions of a pixmap.
  An XPM image contains the dimensions in its data. This function
  returns te width and height.
  \param[in]  data pointer to XPM image data.
  \param[out] w,h  width and height of image
  \returns non-zero if the dimensions were parsed OK
  \returns 0 if there were any problems
  */
int fl_measure_pixmap(/*const*/ char* const* data, int &w, int &h) {
  return fl_measure_pixmap((const char*const*)data,w,h);
}

/**
  Get the dimensions of a pixmap.
  \see fl_measure_pixmap(char* const* data, int &w, int &h)
  */
int fl_measure_pixmap(const char * const *cdata, int &w, int &h) {
  int i = sscanf(cdata[0],"%d%d%d%d",&w,&h,&ncolors,&chars_per_pixel);
  if (i<4 || w<=0 || h<=0 ||
      (chars_per_pixel!=1 && chars_per_pixel!=2) ) return w=0;
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

#endif // U64 else U32

uchar **fl_mask_bitmap; // if non-zero, create bitmap and store pointer here

/**
  Draw XPM image data, with the top-left corner at the given position.
  The image is dithered on 8-bit displays so you won't lose color
  space for programs displaying both images and pixmaps.
  \param[in] data pointer to XPM image data
  \param[in] x,y  position of top-left corner
  \param[in] bg   background color
  \returns 0 if there was any error decoding the XPM data.
  */
int fl_draw_pixmap(/*const*/ char* const* data, int x,int y,Fl_Color bg) {
  return fl_draw_pixmap((const char*const*)data,x,y,bg);
}

#ifdef WIN32
// to compute an unused color to be used for the pixmap background
FL_EXPORT UINT win_pixmap_bg_color; // the RGB() of the pixmap background color
static int color_count; // # of non-transparent colors used in pixmap
static uchar *used_colors; // used_colors[3*i+j] j=0,1,2 are the RGB values of the ith used color

static void make_unused_color(uchar &r, uchar &g, uchar &b)
// makes an RGB triplet different from all the colors used in the pixmap
// and compute win_pixmap_bg_color from this triplet
{
  int i;
  r = 2; g = 3; b = 4;
  while (1) {
    for ( i = 0; i < color_count; i++) {
      if(used_colors[3*i] == r && used_colors[3*i+1] == g && used_colors[3*i+2] == b) break;
      }
    if (i >= color_count) {
      free(used_colors);
      win_pixmap_bg_color = RGB(r, g, b);
      return;
      }
    if (r < 255) r++;
    else {
      r = 0;
      if (g < 255) g++;
      else {
	g = 0;
	b++;
	}
      }
    }
}
#endif

/**
  Draw XPM image data, with the top-left corner at the given position.
  \see fl_draw_pixmap(char* const* data, int x, int y, Fl_Color bg)
  */
int fl_draw_pixmap(const char*const* cdata, int x, int y, Fl_Color bg) {
  pixmap_data d;
  if (!fl_measure_pixmap(cdata, d.w, d.h)) return 0;
  const uchar*const* data = (const uchar*const*)(cdata+1);
  int transparent_index = -1;
  uchar *transparent_c = (uchar *)0; // such that transparent_c[0,1,2] are the RGB of the transparent color
#ifdef WIN32
  color_count = 0;
  used_colors = (uchar *)malloc(abs(ncolors)*3*sizeof(uchar));
#endif

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
      transparent_c = c;
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
#ifdef WIN32
      used_colors[3*color_count] = *p;
      used_colors[3*color_count+1] = *(p+1);
      used_colors[3*color_count+2] = *(p+2);
      color_count++;
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
      int parse = fl_parse_color((const char*)p, c[0], c[1], c[2]);
      if (parse) {
#ifdef WIN32
	used_colors[3*color_count] = c[0];
	used_colors[3*color_count+1] = c[1];
	used_colors[3*color_count+2] = c[2];
	color_count++;
#endif
	}
      else {
        // assume "None" or "#transparent" for any errors
	// "bg" should be transparent...
	Fl::get_color(bg, c[0], c[1], c[2]);
#ifdef __APPLE_QUARTZ__
        c[3] = 0;
#endif
	transparent_index = ind;
	transparent_c = c;
      }
    }
  }
  d.data = data;
#ifdef WIN32
  if (transparent_c) {
    make_unused_color(transparent_c[0], transparent_c[1], transparent_c[2]);
  }
  else {
    uchar r, g, b;
    make_unused_color(r, g, b);
  }
#endif
  
#ifdef  __APPLE_QUARTZ__
  if (fl_graphics_driver->class_name() == Fl_Quartz_Graphics_Driver::class_id ) {
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
	  U32* colors = (U32*)d.byte1[*p++];
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
    delete[] array;
    }
  else {
#endif // __APPLE_QUARTZ__

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
#ifdef __APPLE_QUARTZ__
    }
#endif

  if (chars_per_pixel > 1) for (int i = 0; i < 256; i++) delete[] d.byte1[i];
  return 1;
}

//
// End of "$Id: fl_draw_pixmap.cxx 8362 2011-02-02 18:39:34Z manolo $".
//
