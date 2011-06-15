//
// "$Id: Fl_Bitmap.cxx 8360 2011-02-02 12:42:47Z manolo $"
//
// Bitmap drawing routines for the Fast Light Tool Kit (FLTK).
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

/** \fn Fl_Bitmap::Fl_Bitmap(const char *array, int W, int H)
  The constructors create a new bitmap from the specified bitmap data.*/

/** \fn Fl_Bitmap::Fl_Bitmap(const unsigned char *array, int W, int H)
  The constructors create a new bitmap from the specified bitmap data.*/

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Bitmap.H>
#include <FL/Fl_Printer.H>
#include "flstring.h"

#if defined(__APPLE_QUARTZ__)


Fl_Bitmask fl_create_bitmask(int w, int h, const uchar *array) {
  static uchar reverse[16] =    /* Bit reversal lookup table */
    { 0x00, 0x88, 0x44, 0xcc, 0x22, 0xaa, 0x66, 0xee, 
      0x11, 0x99, 0x55, 0xdd, 0x33, 0xbb, 0x77, 0xff };
  int rowBytes = (w+7)>>3 ;
  uchar *bmask = (uchar*)malloc(rowBytes*h), *dst = bmask;
  const uchar *src = array;
  for ( int i=rowBytes*h; i>0; i--,src++ ) {
    *dst++ = ((reverse[*src & 0x0f] & 0xf0) | (reverse[(*src >> 4) & 0x0f] & 0x0f))^0xff;
  }
  CGDataProviderRef srcp = CGDataProviderCreateWithData( 0L, bmask, rowBytes*h, 0L);
  CGImageRef id_ = CGImageMaskCreate( w, h, 1, 1, rowBytes, srcp, 0L, false);
  CGDataProviderRelease(srcp);
  return (Fl_Bitmask)id_;
}
void fl_delete_bitmask(Fl_Bitmask bm) {
  if (bm) CGImageRelease((CGImageRef)bm);
}


#elif defined(WIN32) // Windows bitmask functions...


// 'fl_create_bitmap()' - Create a 1-bit bitmap for drawing...
static Fl_Bitmask fl_create_bitmap(int w, int h, const uchar *data) {
  // we need to pad the lines out to words & swap the bits
  // in each byte.
  int w1 = (w+7)/8;
  int w2 = ((w+15)/16)*2;
  uchar* newarray = new uchar[w2*h];
  const uchar* src = data;
  uchar* dest = newarray;
  Fl_Bitmask bm;
  static uchar reverse[16] =	/* Bit reversal lookup table */
  	      { 0x00, 0x88, 0x44, 0xcc, 0x22, 0xaa, 0x66, 0xee,
		0x11, 0x99, 0x55, 0xdd, 0x33, 0xbb, 0x77, 0xff };

  for (int y=0; y < h; y++) {
    for (int n = 0; n < w1; n++, src++)
      *dest++ = (uchar)((reverse[*src & 0x0f] & 0xf0) |
	                (reverse[(*src >> 4) & 0x0f] & 0x0f));
    dest += w2-w1;
  }

  bm = CreateBitmap(w, h, 1, 1, newarray);

  delete[] newarray;

  return bm;
}

// 'fl_create_bitmask()' - Create an N-bit bitmap for masking...
Fl_Bitmask fl_create_bitmask(int w, int h, const uchar *data) {
  // this won't work when the user changes display mode during run or
  // has two screens with differnet depths
  Fl_Bitmask bm;
  static uchar hiNibble[16] =
  { 0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0 };
  static uchar loNibble[16] =
  { 0x00, 0x08, 0x04, 0x0c, 0x02, 0x0a, 0x06, 0x0e,
    0x01, 0x09, 0x05, 0x0d, 0x03, 0x0b, 0x07, 0x0f };
  int np  = GetDeviceCaps(fl_gc, PLANES);	//: was always one on sample machines
  int bpp = GetDeviceCaps(fl_gc, BITSPIXEL);//: 1,4,8,16,24,32 and more odd stuff?
  int Bpr = (bpp*w+7)/8;			//: bytes per row
  int pad = Bpr&1, w1 = (w+7)/8, shr = ((w-1)&7)+1;
  if (bpp==4) shr = (shr+1)/2;
  uchar *newarray = new uchar[(Bpr+pad)*h];
  uchar *dst = newarray;
  const uchar *src = data;

  for (int i=0; i<h; i++) {
    // This is slooow, but we do it only once per pixmap
    for (int j=w1; j>0; j--) {
      uchar b = *src++;
      if (bpp==1) {
        *dst++ = (uchar)( hiNibble[b&15] ) | ( loNibble[(b>>4)&15] );
      } else if (bpp==4) {
        for (int k=(j==1)?shr:4; k>0; k--) {
          *dst++ = (uchar)("\377\360\017\000"[b&3]);
          b = b >> 2;
        }
      } else {
        for (int k=(j==1)?shr:8; k>0; k--) {
          if (b&1) {
            *dst++=0;
	    if (bpp>8) *dst++=0;
            if (bpp>16) *dst++=0;
	    if (bpp>24) *dst++=0;
	  } else {
	    *dst++=0xff;
	    if (bpp>8) *dst++=0xff;
	    if (bpp>16) *dst++=0xff;
	    if (bpp>24) *dst++=0xff;
	  }

	  b = b >> 1;
        }
      }
    }

    dst += pad;
  }

  bm = CreateBitmap(w, h, np, bpp, newarray);
  delete[] newarray;

  return bm;
}


void fl_delete_bitmask(Fl_Bitmask bm) {
  DeleteObject((HGDIOBJ)bm);
}


#else // X11 bitmask functions


Fl_Bitmask fl_create_bitmask(int w, int h, const uchar *data) {
  return XCreateBitmapFromData(fl_display, fl_window, (const char *)data,
                               (w+7)&-8, h);
}

void fl_delete_bitmask(Fl_Bitmask bm) {
  fl_delete_offscreen((Fl_Offscreen)bm);
}


#endif // __APPLE__


// Create a 1-bit mask used for alpha blending
Fl_Bitmask fl_create_alphamask(int w, int h, int d, int ld, const uchar *array) {
  Fl_Bitmask bm;
  int bmw = (w + 7) / 8;
  uchar *bitmap = new uchar[bmw * h];
  uchar *bitptr, bit;
  const uchar *dataptr;
  int x, y;
  static uchar dither[16][16] = { // Simple 16x16 Floyd dither
    { 0,   128, 32,  160, 8,   136, 40,  168,
      2,   130, 34,  162, 10,  138, 42,  170 },
    { 192, 64,  224, 96,  200, 72,  232, 104,
      194, 66,  226, 98,  202, 74,  234, 106 },
    { 48,  176, 16,  144, 56,  184, 24,  152,
      50,  178, 18,  146, 58,  186, 26,  154 },
    { 240, 112, 208, 80,  248, 120, 216, 88,
      242, 114, 210, 82,  250, 122, 218, 90 },
    { 12,  140, 44,  172, 4,   132, 36,  164,
      14,  142, 46,  174, 6,   134, 38,  166 },
    { 204, 76,  236, 108, 196, 68,  228, 100,
      206, 78,  238, 110, 198, 70,  230, 102 },
    { 60,  188, 28,  156, 52,  180, 20,  148,
      62,  190, 30,  158, 54,  182, 22,  150 },
    { 252, 124, 220, 92,  244, 116, 212, 84,
      254, 126, 222, 94,  246, 118, 214, 86 },
    { 3,   131, 35,  163, 11,  139, 43,  171,
      1,   129, 33,  161, 9,   137, 41,  169 },
    { 195, 67,  227, 99,  203, 75,  235, 107,
      193, 65,  225, 97,  201, 73,  233, 105 },
    { 51,  179, 19,  147, 59,  187, 27,  155,
      49,  177, 17,  145, 57,  185, 25,  153 },
    { 243, 115, 211, 83,  251, 123, 219, 91,
      241, 113, 209, 81,  249, 121, 217, 89 },
    { 15,  143, 47,  175, 7,   135, 39,  167,
      13,  141, 45,  173, 5,   133, 37,  165 },
    { 207, 79,  239, 111, 199, 71,  231, 103,
      205, 77,  237, 109, 197, 69,  229, 101 },
    { 63,  191, 31,  159, 55,  183, 23,  151,
      61,  189, 29,  157, 53,  181, 21,  149 },
    { 254, 127, 223, 95,  247, 119, 215, 87,
      253, 125, 221, 93,  245, 117, 213, 85 }
  };

  // Generate a 1-bit "screen door" alpha mask; not always pretty, but
  // definitely fast...  In the future we may be able to support things
  // like the RENDER extension in XFree86, when available, to provide
  // true RGBA-blended rendering.  See:
  //
  //     http://www.xfree86.org/~keithp/render/protocol.html
  //
  // for more info on XRender...
  //
  // MacOS already provides alpha blending support and has its own
  // fl_create_alphamask() function...
  memset(bitmap, 0, bmw * h);

  for (dataptr = array + d - 1, y = 0; y < h; y ++, dataptr += ld)
    for (bitptr = bitmap + y * bmw, bit = 1, x = 0; x < w; x ++, dataptr += d) {
      if (*dataptr > dither[x & 15][y & 15])
	*bitptr |= bit;
      if (bit < 128) bit <<= 1;
      else {
	bit = 1;
	bitptr ++;
      }
    }

  bm = fl_create_bitmask(w, h, bitmap);
  delete[] bitmap;

  return (bm);
}

void Fl_Bitmap::draw(int XP, int YP, int WP, int HP, int cx, int cy) {
  fl_graphics_driver->draw(this, XP, YP, WP, HP, cx, cy);
}

static int start(Fl_Bitmap *bm, int XP, int YP, int WP, int HP, int w, int h, int &cx, int &cy, 
		 int &X, int &Y, int &W, int &H)
{
  // account for current clip region (faster on Irix):
  fl_clip_box(XP,YP,WP,HP,X,Y,W,H);
  cx += X-XP; cy += Y-YP;
  // clip the box down to the size of image, quit if empty:
  if (cx < 0) {W += cx; X -= cx; cx = 0;}
  if (cx+W > w) W = w-cx;
  if (W <= 0) return 1;
  if (cy < 0) {H += cy; Y -= cy; cy = 0;}
  if (cy+H > h) H = h-cy;
  if (H <= 0) return 1;
  return 0;
}

#ifdef __APPLE__
void Fl_Quartz_Graphics_Driver::draw(Fl_Bitmap *bm, int XP, int YP, int WP, int HP, int cx, int cy) {
  int X, Y, W, H;
  if (!bm->array) {
    bm->draw_empty(XP, YP);
    return;
  }
  if (start(bm, XP, YP, WP, HP, bm->w(), bm->h(), cx, cy, X, Y, W, H)) {
    return;
  }
  if (!bm->id_) bm->id_ = fl_create_bitmask(bm->w(), bm->h(), bm->array);
  if (bm->id_ && fl_gc) {
    CGRect rect = { { X, Y }, { W, H } };
    Fl_X::q_begin_image(rect, cx, cy, bm->w(), bm->h());
    CGContextDrawImage(fl_gc, rect, (CGImageRef)bm->id_);
    Fl_X::q_end_image();
  }
}

#elif defined(WIN32)
void Fl_GDI_Graphics_Driver::draw(Fl_Bitmap *bm, int XP, int YP, int WP, int HP, int cx, int cy) {
  int X, Y, W, H;
  if (!bm->array) {
    bm->draw_empty(XP, YP);
    return;
  }
  if (start(bm, XP, YP, WP, HP, bm->w(), bm->h(), cx, cy, X, Y, W, H)) {
    return;
  }
  if (!bm->id_) bm->id_ = fl_create_bitmap(bm->w(), bm->h(), bm->array);
  
  typedef BOOL (WINAPI* fl_transp_func)  (HDC,int,int,int,int,HDC,int,int,int,int,UINT);
  static fl_transp_func fl_TransparentBlt;
  HDC tempdc;
  int save;
  BOOL use_print_algo = false;
  if (Fl_Surface_Device::surface()->class_name() == Fl_Printer::class_id) {
    static HMODULE hMod = NULL;
    if (!hMod) {
      hMod = LoadLibrary("MSIMG32.DLL");
      if (hMod) fl_TransparentBlt = (fl_transp_func)GetProcAddress(hMod, "TransparentBlt");
    }
    if (fl_TransparentBlt) use_print_algo = true;
  }
  if (use_print_algo) { // algorithm for bitmap output to Fl_GDI_Printer
    Fl_Offscreen tmp_id = fl_create_offscreen(W, H);
    fl_begin_offscreen(tmp_id);
    Fl_Color save_c = fl_color(); // save bitmap's desired color
    uchar r, g, b;
    Fl::get_color(save_c, r, g, b);
    r = 255-r;
    g = 255-g;
    b = 255-b;
    Fl_Color background = fl_rgb_color(r, g, b); // a color very different from the bitmap's
    fl_color(background);
    fl_rectf(0,0,W,H); // use this color as offscreen background
    fl_color(save_c); // back to bitmap's color
    tempdc = CreateCompatibleDC(fl_gc);
    save = SaveDC(tempdc);
    SelectObject(tempdc, (HGDIOBJ)bm->id_);
    SelectObject(fl_gc, fl_brush()); // use bitmap's desired color
    BitBlt(fl_gc, 0, 0, W, H, tempdc, 0, 0, 0xE20746L); // draw bitmap to offscreen
    fl_end_offscreen(); // offscreen data is in tmp_id
    SelectObject(tempdc, (HGDIOBJ)tmp_id); // use offscreen data
    // draw it to printer context with background color as transparent
    fl_TransparentBlt(fl_gc, X,Y,W,H, tempdc, cx, cy, bm->w(), bm->h(), RGB(r, g, b) ); 
    fl_delete_offscreen(tmp_id);
  }
  else { // algorithm for bitmap output to display
    tempdc = CreateCompatibleDC(fl_gc);
    save = SaveDC(tempdc);
    SelectObject(tempdc, (HGDIOBJ)bm->id_);
    SelectObject(fl_gc, fl_brush());
    // secret bitblt code found in old MSWindows reference manual:
    BitBlt(fl_gc, X, Y, W, H, tempdc, cx, cy, 0xE20746L);
  }
  RestoreDC(tempdc, save);
  DeleteDC(tempdc);
}  

#else // Xlib
void Fl_Xlib_Graphics_Driver::draw(Fl_Bitmap *bm, int XP, int YP, int WP, int HP, int cx, int cy) {
  int X, Y, W, H;
  if (!bm->array) {
    bm->draw_empty(XP, YP);
    return;
  }
  if (start(bm, XP, YP, WP, HP, bm->w(), bm->h(), cx, cy, X, Y, W, H)) {
    return;
  }
  if (!bm->id_) bm->id_ = fl_create_bitmask(bm->w(), bm->h(), bm->array);
  
  XSetStipple(fl_display, fl_gc, bm->id_);
  int ox = X-cx; if (ox < 0) ox += bm->w();
  int oy = Y-cy; if (oy < 0) oy += bm->h();
  XSetTSOrigin(fl_display, fl_gc, ox, oy);
  XSetFillStyle(fl_display, fl_gc, FillStippled);
  XFillRectangle(fl_display, fl_window, fl_gc, X, Y, W, H);
  XSetFillStyle(fl_display, fl_gc, FillSolid);
}
#endif

/**
  The destructor free all memory and server resources that are used by
  the bitmap.
*/
Fl_Bitmap::~Fl_Bitmap() {
  uncache();
  if (alloc_array) delete[] (uchar *)array;
}

void Fl_Bitmap::uncache() {
  if (id_) {
#ifdef __APPLE_QUARTZ__
    fl_delete_bitmask((Fl_Bitmask)id_);
#else
    fl_delete_bitmask((Fl_Offscreen)id_);
#endif
    id_ = 0;
  }
}

void Fl_Bitmap::label(Fl_Widget* widget) {
  widget->image(this);
}

void Fl_Bitmap::label(Fl_Menu_Item* m) {
  Fl::set_labeltype(_FL_IMAGE_LABEL, labeltype, measure);
  m->label(_FL_IMAGE_LABEL, (const char*)this);
}

Fl_Image *Fl_Bitmap::copy(int W, int H) {
  Fl_Bitmap	*new_image;	// New RGB image
  uchar		*new_array;	// New array for image data

  // Optimize the simple copy where the width and height are the same...
  if (W == w() && H == h()) {
    new_array = new uchar [H * ((W + 7) / 8)];
    memcpy(new_array, array, H * ((W + 7) / 8));

    new_image = new Fl_Bitmap(new_array, W, H);
    new_image->alloc_array = 1;

    return new_image;
  }
  if (W <= 0 || H <= 0) return 0;

  // OK, need to resize the image data; allocate memory and 
  uchar		*new_ptr,	// Pointer into new array
		new_bit,	// Bit for new array
		old_bit;	// Bit for old array
  const uchar	*old_ptr;	// Pointer into old array
  int		sx, sy,		// Source coordinates
		dx, dy,		// Destination coordinates
		xerr, yerr,	// X & Y errors
		xmod, ymod,	// X & Y moduli
		xstep, ystep;	// X & Y step increments


  // Figure out Bresenheim step/modulus values...
  xmod   = w() % W;
  xstep  = w() / W;
  ymod   = h() % H;
  ystep  = h() / H;

  // Allocate memory for the new image...
  new_array = new uchar [H * ((W + 7) / 8)];
  new_image = new Fl_Bitmap(new_array, W, H);
  new_image->alloc_array = 1;

  memset(new_array, 0, H * ((W + 7) / 8));

  // Scale the image using a nearest-neighbor algorithm...
  for (dy = H, sy = 0, yerr = H, new_ptr = new_array; dy > 0; dy --) {
    for (dx = W, xerr = W, old_ptr = array + sy * ((w() + 7) / 8), sx = 0, new_bit = 1;
	 dx > 0;
	 dx --) {
      old_bit = (uchar)(1 << (sx & 7));
      if (old_ptr[sx / 8] & old_bit) *new_ptr |= new_bit;

      if (new_bit < 128) new_bit <<= 1;
      else {
        new_bit = 1;
	new_ptr ++;
      }

      sx   += xstep;
      xerr -= xmod;

      if (xerr <= 0) {
	xerr += W;
	sx ++;
      }
    }

    if (new_bit > 1) new_ptr ++;

    sy   += ystep;
    yerr -= ymod;
    if (yerr <= 0) {
      yerr += H;
      sy ++;
    }
  }

  return new_image;
}


//
// End of "$Id: Fl_Bitmap.cxx 8360 2011-02-02 12:42:47Z manolo $".
//
