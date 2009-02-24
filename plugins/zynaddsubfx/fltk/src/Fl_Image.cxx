//
// "$Id: Fl_Image.cxx 5888 2007-06-07 17:23:41Z matt $"
//
// Image drawing code for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Image.H>
#include "flstring.h"

#ifdef WIN32
void fl_release_dc(HWND, HDC); // from Fl_win32.cxx
#endif

void fl_restore_clip(); // from fl_rect.cxx

//
// Base image class...
//

Fl_Image::~Fl_Image() {
}

void Fl_Image::uncache() {
}

void Fl_Image::draw(int XP, int YP, int, int, int, int) {
  draw_empty(XP, YP);
}

void Fl_Image::draw_empty(int X, int Y) {
  if (w() > 0 && h() > 0) {
    fl_color(FL_FOREGROUND_COLOR);
    fl_rect(X, Y, w(), h());
    fl_line(X, Y, X + w() - 1, Y + h() - 1);
    fl_line(X, Y + h() - 1, X + w() - 1, Y);
  }
}

Fl_Image *Fl_Image::copy(int W, int H) {
  return new Fl_Image(W, H, d());
}

void Fl_Image::color_average(Fl_Color, float) {
}

void Fl_Image::desaturate() {
}

void Fl_Image::label(Fl_Widget* widget) {
  widget->image(this);
}

void Fl_Image::label(Fl_Menu_Item* m) {
  Fl::set_labeltype(_FL_IMAGE_LABEL, labeltype, measure);
  m->label(_FL_IMAGE_LABEL, (const char*)this);
}

void
Fl_Image::labeltype(const Fl_Label *lo,		// I - Label
                    int            lx,		// I - X position
		    int            ly,		// I - Y position
		    int            lw,		// I - Width of label
		    int            lh,		// I - Height of label
		    Fl_Align       la) {	// I - Alignment
  Fl_Image	*img;				// Image pointer
  int		cx, cy;				// Image position

  img = (Fl_Image *)(lo->value);

  if (la & FL_ALIGN_LEFT) cx = 0;
  else if (la & FL_ALIGN_RIGHT) cx = img->w() - lw;
  else cx = (img->w() - lw) / 2;

  if (la & FL_ALIGN_TOP) cy = 0;
  else if (la & FL_ALIGN_BOTTOM) cy = img->h() - lh;
  else cy = (img->h() - lh) / 2;

  fl_color((Fl_Color)lo->color);

  img->draw(lx, ly, lw, lh, cx, cy);
}

void
Fl_Image::measure(const Fl_Label *lo,		// I - Label
                  int            &lw,		// O - Width of image
		  int            &lh) {		// O - Height of image
  Fl_Image *img;				// Image pointer

  img = (Fl_Image *)(lo->value);

  lw = img->w();
  lh = img->h();
}


//
// RGB image class...
//

Fl_RGB_Image::~Fl_RGB_Image() {
  uncache();
  if (alloc_array) delete[] (uchar *)array;
}

void Fl_RGB_Image::uncache() {
#ifdef __APPLE_QUARTZ__
  if (id) {
    CGImageRelease((CGImageRef)id);
    id = 0;
  }
#else
  if (id) {
    fl_delete_offscreen((Fl_Offscreen)id);
    id = 0;
  }

  if (mask) {
    fl_delete_bitmask((Fl_Bitmask)mask);
    mask = 0;
  }
#endif
}

Fl_Image *Fl_RGB_Image::copy(int W, int H) {
  Fl_RGB_Image	*new_image;	// New RGB image
  uchar		*new_array;	// New array for image data

  // Optimize the simple copy where the width and height are the same,
  // or when we are copying an empty image...
  if ((W == w() && H == h()) ||
      !w() || !h() || !d() || !array) {
    if (array) {
      // Make a copy of the image data and return a new Fl_RGB_Image...
      new_array = new uchar[w() * h() * d()];
      if (ld() && ld()!=w()*d()) {
        const uchar *src = array;
        uchar *dst = new_array;
        int dy, dh = h(), wd = w()*d(), wld = ld();
        for (dy=0; dy<dh; dy++) {
          memcpy(dst, src, wd);
          src += wld;
          dst += wd;
        }
      } else {
        memcpy(new_array, array, w() * h() * d());
      }
      new_image = new Fl_RGB_Image(new_array, w(), h(), d());
      new_image->alloc_array = 1;

      return new_image;
    } else return new Fl_RGB_Image(array, w(), h(), d(), ld());
  }
  if (W <= 0 || H <= 0) return 0;

  // OK, need to resize the image data; allocate memory and 
  uchar		*new_ptr;	// Pointer into new array
  const uchar	*old_ptr;	// Pointer into old array
  int		c,		// Channel number
		sy,		// Source coordinate
		dx, dy,		// Destination coordinates
		xerr, yerr,	// X & Y errors
		xmod, ymod,	// X & Y moduli
		xstep, ystep,	// X & Y step increments
    line_d; // stride from line to line


  // Figure out Bresenheim step/modulus values...
  xmod   = w() % W;
  xstep  = (w() / W) * d();
  ymod   = h() % H;
  ystep  = h() / H;
  line_d = ld() ? ld() : w() * d();

  // Allocate memory for the new image...
  new_array = new uchar [W * H * d()];
  new_image = new Fl_RGB_Image(new_array, W, H, d());
  new_image->alloc_array = 1;

  // Scale the image using a nearest-neighbor algorithm...
  for (dy = H, sy = 0, yerr = H, new_ptr = new_array; dy > 0; dy --) {
    for (dx = W, xerr = W, old_ptr = array + sy * line_d; dx > 0; dx --) {
      for (c = 0; c < d(); c ++) *new_ptr++ = old_ptr[c];

      old_ptr += xstep;
      xerr    -= xmod;

      if (xerr <= 0) {
	xerr    += W;
	old_ptr += d();
      }
    }

    sy   += ystep;
    yerr -= ymod;
    if (yerr <= 0) {
      yerr += H;
      sy ++;
    }
  }

  return new_image;
}

void Fl_RGB_Image::color_average(Fl_Color c, float i) {
  // Don't average an empty image...
  if (!w() || !h() || !d() || !array) return;

  // Delete any existing pixmap/mask objects...
  uncache();

  // Allocate memory as needed...
  uchar		*new_array,
		*new_ptr;

  if (!alloc_array) new_array = new uchar[h() * w() * d()];
  else new_array = (uchar *)array;

  // Get the color to blend with...
  uchar		r, g, b;
  unsigned	ia, ir, ig, ib;

  Fl::get_color(c, r, g, b);
  if (i < 0.0f) i = 0.0f;
  else if (i > 1.0f) i = 1.0f;

  ia = (unsigned)(256 * i);
  ir = r * (256 - ia);
  ig = g * (256 - ia);
  ib = b * (256 - ia);

  // Update the image data to do the blend...
  const uchar	*old_ptr;
  int		x, y;
  int   line_i = ld() ? ld() - (w()*d()) : 0; // increment from line end to beginning of next line

  if (d() < 3) {
    ig = (r * 31 + g * 61 + b * 8) / 100 * (256 - ia);

    for (new_ptr = new_array, old_ptr = array, y = 0; y < h(); y ++, old_ptr += line_i)
      for (x = 0; x < w(); x ++) {
	*new_ptr++ = (*old_ptr++ * ia + ig) >> 8;
	if (d() > 1) *new_ptr++ = *old_ptr++;
      }
  } else {
    for (new_ptr = new_array, old_ptr = array, y = 0; y < h(); y ++, old_ptr += line_i)
      for (x = 0; x < w(); x ++) {
	*new_ptr++ = (*old_ptr++ * ia + ir) >> 8;
	*new_ptr++ = (*old_ptr++ * ia + ig) >> 8;
	*new_ptr++ = (*old_ptr++ * ia + ib) >> 8;
	if (d() > 3) *new_ptr++ = *old_ptr++;
      }
  }

  // Set the new pointers/values as needed...
  if (!alloc_array) {
    array       = new_array;
    alloc_array = 1;

    ld(0);
  }
}

void Fl_RGB_Image::desaturate() {
  // Don't desaturate an empty image...
  if (!w() || !h() || !d() || !array) return;

  // Can only desaturate color images...
  if (d() < 3) return;

  // Delete any existing pixmap/mask objects...
  uncache();

  // Allocate memory for a grayscale image...
  uchar		*new_array,
		*new_ptr;
  int		new_d;

  new_d     = d() - 2;
  new_array = new uchar[h() * w() * new_d];

  // Copy the image data, converting to grayscale...
  const uchar	*old_ptr;
  int		x, y;
  int   line_i = ld() ? ld() - (w()*d()) : 0; // increment from line end to beginning of next line

  for (new_ptr = new_array, old_ptr = array, y = 0; y < h(); y ++, old_ptr += line_i)
    for (x = 0; x < w(); x ++, old_ptr += d()) {
      *new_ptr++ = (uchar)((31 * old_ptr[0] + 61 * old_ptr[1] + 8 * old_ptr[2]) / 100);
      if (d() > 3) *new_ptr++ = old_ptr[3];
    }

  // Free the old array as needed, and then set the new pointers/values...
  if (alloc_array) delete[] (uchar *)array;

  array       = new_array;
  alloc_array = 1;

  ld(0);
  d(new_d);
}

#if !defined(WIN32) && !USE_QUARTZ
// Composite an image with alpha on systems that don't have accelerated
// alpha compositing...
static void alpha_blend(Fl_RGB_Image *img, int X, int Y, int W, int H, int cx, int cy) {
  uchar *srcptr = (uchar*)img->array + img->d() * (img->w() * cy + cx);
  int srcskip = img->d() * (img->w() - W);

  uchar *dst = new uchar[W * H * 3];
  uchar *dstptr = dst;

  fl_read_image(dst, X, Y, W, H, 0);

  uchar srcr, srcg, srcb, srca;
  uchar dstr, dstg, dstb, dsta;

  if (img->d() == 2) {
    // Composite grayscale + alpha over RGB...
    // Composite RGBA over RGB...
    for (int y = H; y > 0; y--, srcptr+=srcskip)
      for (int x = W; x > 0; x--) {
	srcg = *srcptr++;
	srca = *srcptr++;

	dstr = dstptr[0];
	dstg = dstptr[1];
	dstb = dstptr[2];
	dsta = 255 - srca;

	*dstptr++ = (srcg * srca + dstr * dsta) >> 8;
	*dstptr++ = (srcg * srca + dstg * dsta) >> 8;
	*dstptr++ = (srcg * srca + dstb * dsta) >> 8;
      }
  } else {
    // Composite RGBA over RGB...
    for (int y = H; y > 0; y--, srcptr+=srcskip)
      for (int x = W; x > 0; x--) {
	srcr = *srcptr++;
	srcg = *srcptr++;
	srcb = *srcptr++;
	srca = *srcptr++;

	dstr = dstptr[0];
	dstg = dstptr[1];
	dstb = dstptr[2];
	dsta = 255 - srca;

	*dstptr++ = (srcr * srca + dstr * dsta) >> 8;
	*dstptr++ = (srcg * srca + dstg * dsta) >> 8;
	*dstptr++ = (srcb * srca + dstb * dsta) >> 8;
      }
  }

  fl_draw_image(dst, X, Y, W, H, 3, 0);

  delete[] dst;
}
#endif // !WIN32 && !USE_QUARTZ

void Fl_RGB_Image::draw(int XP, int YP, int WP, int HP, int cx, int cy) {
  // Don't draw an empty image...
  if (!d() || !array) {
    draw_empty(XP, YP);
    return;
  }

  // account for current clip region (faster on Irix):
  int X,Y,W,H; fl_clip_box(XP,YP,WP,HP,X,Y,W,H);
  cx += X-XP; cy += Y-YP;
  // clip the box down to the size of image, quit if empty:
  if (cx < 0) {W += cx; X -= cx; cx = 0;}
  if (cx+W > w()) W = w()-cx;
  if (W <= 0) return;
  if (cy < 0) {H += cy; Y -= cy; cy = 0;}
  if (cy+H > h()) H = h()-cy;
  if (H <= 0) return;
  if (!id) {
#ifdef __APPLE_QUARTZ__
    CGColorSpaceRef lut = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef src = CGDataProviderCreateWithData( 0L, array, w()*h()*d(), 0L);
    id = CGImageCreate( w(), h(), 8, d()*8, ld()?ld():w()*d(),
        lut, (d()&1)?kCGImageAlphaNone:kCGImageAlphaLast,
        src, 0L, false, kCGRenderingIntentDefault);
    CGColorSpaceRelease(lut);
    CGDataProviderRelease(src);
#elif defined(WIN32)
    id = fl_create_offscreen(w(), h());
    if (d() == 2 || d() == 4 && fl_can_do_alpha_blending()) {
      fl_begin_offscreen((Fl_Offscreen)id);
      fl_draw_image(array, 0, 0, w(), h(), d()|FL_IMAGE_WITH_ALPHA, ld());
      fl_end_offscreen();
    } else {
      fl_begin_offscreen((Fl_Offscreen)id);
      fl_draw_image(array, 0, 0, w(), h(), d(), ld());
      fl_end_offscreen();
      if (d() == 2 || d() == 4) {
        mask = fl_create_alphamask(w(), h(), d(), ld(), array);
      }
    }
#else
    if (d() == 1 || d() == 3) {
      id = fl_create_offscreen(w(), h());
      fl_begin_offscreen((Fl_Offscreen)id);
      fl_draw_image(array, 0, 0, w(), h(), d(), ld());
      fl_end_offscreen();
    }
#endif
  }
#ifdef WIN32
  if (mask) {
    HDC new_gc = CreateCompatibleDC(fl_gc);
    int save = SaveDC(new_gc);
    SelectObject(new_gc, (void*)mask);
    BitBlt(fl_gc, X, Y, W, H, new_gc, cx, cy, SRCAND);
    SelectObject(new_gc, (void*)id);
    BitBlt(fl_gc, X, Y, W, H, new_gc, cx, cy, SRCPAINT);
    RestoreDC(new_gc,save);
    DeleteDC(new_gc);
  } else if (d()==2 || d()==4) {
    fl_copy_offscreen_with_alpha(X, Y, W, H, (Fl_Offscreen)id, cx, cy);
  } else {
    fl_copy_offscreen(X, Y, W, H, (Fl_Offscreen)id, cx, cy);
  }
#elif defined(__APPLE_QD__)
  if (mask) {
    Rect src, dst;
    // MRS: STR #114 says we should be using cx, cy, W, and H...
//    src.left = 0; src.right = w();
//    src.top = 0; src.bottom = h();
//    dst.left = X; dst.right = X+w();
//    dst.top = Y; dst.bottom = Y+h();
    src.left = cx; src.right = cx+W;
    src.top = cy; src.bottom = cy+H;
    dst.left = X; dst.right = X+W;
    dst.top = Y; dst.bottom = Y+H;
    RGBColor rgb;
    rgb.red = 0xffff; rgb.green = 0xffff; rgb.blue = 0xffff;
    RGBBackColor(&rgb);
    rgb.red = 0x0000; rgb.green = 0x0000; rgb.blue = 0x0000;
    RGBForeColor(&rgb);

    CopyMask(GetPortBitMapForCopyBits((GrafPtr)id),
	     GetPortBitMapForCopyBits((GrafPtr)mask), 
	     GetPortBitMapForCopyBits(GetWindowPort(fl_window)),
             &src, &src, &dst);
  } else if (id) fl_copy_offscreen(X, Y, W, H, (Fl_Offscreen)id, cx, cy);
  else {
    // Composite image with alpha manually each time...
    alpha_blend(this, X, Y, W, H, cx, cy);
  }
#elif defined(__APPLE_QUARTZ__)
  if (id && fl_gc) {
    CGRect rect = { { X, Y }, { W, H } };
    Fl_X::q_begin_image(rect, cx, cy, w(), h());
    CGContextDrawImage(fl_gc, rect, (CGImageRef)id);
    Fl_X::q_end_image();
  }
#else
  if (id) {
    if (mask) {
      // I can't figure out how to combine a mask with existing region,
      // so cut the image down to a clipped rectangle:
      int nx, ny; fl_clip_box(X,Y,W,H,nx,ny,W,H);
      cx += nx-X; X = nx;
      cy += ny-Y; Y = ny;
      // make X use the bitmap as a mask:
      XSetClipMask(fl_display, fl_gc, mask);
      int ox = X-cx; if (ox < 0) ox += w();
      int oy = Y-cy; if (oy < 0) oy += h();
      XSetClipOrigin(fl_display, fl_gc, X-cx, Y-cy);
    }

    fl_copy_offscreen(X, Y, W, H, id, cx, cy);

    if (mask) {
      // put the old clip region back
      XSetClipOrigin(fl_display, fl_gc, 0, 0);
      fl_restore_clip();
    }
  } else {
    // Composite image with alpha manually each time...
    alpha_blend(this, X, Y, W, H, cx, cy);
  }
#endif
}

void Fl_RGB_Image::label(Fl_Widget* widget) {
  widget->image(this);
}

void Fl_RGB_Image::label(Fl_Menu_Item* m) {
  Fl::set_labeltype(_FL_IMAGE_LABEL, labeltype, measure);
  m->label(_FL_IMAGE_LABEL, (const char*)this);
}


//
// End of "$Id: Fl_Image.cxx 5888 2007-06-07 17:23:41Z matt $".
//
