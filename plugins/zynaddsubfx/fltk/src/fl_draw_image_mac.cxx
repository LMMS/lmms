//
// "$Id: fl_draw_image_mac.cxx 5614 2007-01-18 15:25:09Z matt $"
//
// MacOS image drawing code for the Fast Light Tool Kit (FLTK).
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

////////////////////////////////////////////////////////////////

#include <config.h>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#define MAXBUFFER 0x40000 // 256k

/**
 * draw an image based on the input parameters
 *
 * buf:       image source data
 * X, Y:      position (in buffer?!)
 * W, H:      size of picture (in pixel?)
 * delta:     distance from pixel to pixel in buf in bytes
 * linedelta: distance from line to line in buf in bytes
 * mono:      if set, pixel is one byte - if zero, pixel is 3 byte
 * cb:        callback to copy image data into (RGB?) buffer
 *   buf:       pointer to first byte in image source
 *   x, y:      position in buffer
 *   w:         width (in bytes?)
 *   dst:       destination buffer
 * userdata:  ?
 */
static void innards(const uchar *buf, int X, int Y, int W, int H,
		    int delta, int linedelta, int mono,
		    Fl_Draw_Image_Cb cb, void* userdata)
{
  if (!linedelta) linedelta = W*delta;

#ifdef __APPLE_QD__
  // theoretically, if the current GPort permits, we could write
  // directly into it, avoiding the temporary GWorld. For now I
  // will go the safe way... .
  char direct = 0;
  GWorldPtr gw;
  Rect bounds;
  bounds.left=0; bounds.right=W; bounds.top=0; bounds.bottom=H;
  QDErr err = NewGWorld( &gw, 32, &bounds, 0L, 0L, useTempMem );
  if (err==noErr && gw) {
    PixMapHandle pm = GetGWorldPixMap( gw );
    if ( pm ) {
      LockPixels( pm );
      if ( *pm ) {
        uchar *base = (uchar*)GetPixBaseAddr( pm );
        if ( base ) {
          PixMapPtr pmp = *pm;
          // make absolutely sure that we can use a direct memory write to
          // create the pixmap!
          if ( pmp->pixelType == 16 || pmp->pixelSize == 32 || pmp->cmpCount == 3 || pmp->cmpSize == 8 ) {
            int rowBytes = pmp->rowBytes & 0x3fff;
            if ( cb )
            {
              uchar *tmpBuf = new uchar[ W*delta ];
              if ( mono ) delta -= 1; else delta -= 3; 
              for ( int i=0; i<H; i++ )
              {
                uchar *src = tmpBuf;
                uchar *dst = base + i*rowBytes;
                cb( userdata, 0, i, W, tmpBuf );
                if ( mono ) {
                  for ( int j=0; j<W; j++ )
                    { uchar c = *src++; *dst++ = 0; *dst++ = c; *dst++ = c; *dst++ = c; src += delta; }
                } else {
                  for ( int j=0; j<W; j++ )
                    { *dst++ = 0; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++; src += delta; }
                }
              }
              delete[] tmpBuf;
            }
            else
            {
              if ( mono ) delta -= 1; else delta -= 3; 
              for ( int i=0; i<H; i++ )
              {
                const uchar *src = buf+i*linedelta;
                uchar *dst = base + i*rowBytes;
                if ( mono ) {
                  for ( int j=0; j<W; j++ )
                    { uchar c = *src++; *dst++ = 0; *dst++ = c; *dst++ = c; *dst++ = c; src += delta; }
                } else {
                  for ( int j=0; j<W; j++ )
                    { *dst++ = 0; *dst++ = *src++; *dst++ = *src++; *dst++ = *src++; src += delta; }
                }
              }
            }
          
            fl_copy_offscreen( X, Y, W, H, gw, 0, 0 );
            direct = 1;
          }
        }
      }

      UnlockPixels( pm );
    }

    DisposeGWorld( gw );
  }

  // great. We were able to write the pixels directly into memory, so we can return now.
  if ( direct )
    return;

  // following the very save (and very slow) way to write the image into the give port
  if ( cb )
  {
    uchar *tmpBuf = new uchar[ W*3 ];
    for ( int i=0; i<H; i++ )
    {
      uchar *src = tmpBuf;
      cb( userdata, 0, i, W, tmpBuf );
      for ( int j=0; j<W; j++ )
      {
        if ( mono )          
          { fl_color( src[0], src[0], src[0] ); src++; }
        else
          { fl_color( src[0], src[1], src[2] ); src+=3; }
        MoveTo( X+j, Y+i );
        Line( 0, 0 );
      }
    }
    delete[] tmpBuf;
  }
  else
  {
    for ( int i=0; i<H; i++ )
    {
      const uchar *src = buf+i*linedelta;
      for ( int j=0; j<W; j++ )
      {
        if ( mono )          
          fl_color( src[0], src[0], src[0] );
        else
          fl_color( src[0], src[1], src[2] );
        MoveTo( X+j, Y+i );
        Line( 0, 0 );
        src += delta;
      }
    }
  }
#elif defined(__APPLE_QUARTZ__)
  const void *array = buf;
  uchar *tmpBuf = 0;
  if (cb) {
    tmpBuf = new uchar[ H*W*delta ];
    for (int i=0; i<H; i++) {
      cb(userdata, 0, i, W, tmpBuf+i*W*delta);
    }
    array = (void*)tmpBuf;
    linedelta = W*delta;
  }
  // create an image context
  CGColorSpaceRef   lut = 0;
  if (delta<=2) 
    lut = CGColorSpaceCreateDeviceGray();
  else
    lut = CGColorSpaceCreateDeviceRGB();
  CGDataProviderRef src = CGDataProviderCreateWithData( 0L, array, linedelta*H, 0L);
  CGImageRef        img = CGImageCreate( W, H, 8, 8*delta, linedelta,
                            lut, delta&1?kCGImageAlphaNone:kCGImageAlphaNoneSkipLast,
                            src, 0L, false, kCGRenderingIntentDefault);
  // draw the image into the destination context
  if (img) {
    CGRect rect = { { X, Y }, { W, H } };
    Fl_X::q_begin_image(rect, 0, 0, W, H);
    CGContextDrawImage(fl_gc, rect, img);
    Fl_X::q_end_image();
    // release all allocated resources
    CGImageRelease(img);
  }
  CGColorSpaceRelease(lut);
  CGDataProviderRelease(src);
  if (cb) {
    delete[] tmpBuf;
  }
  if (img) return; // else fall through to slow mode
  // following the very save (and very slow) way to write the image into the give port
  CGContextSetShouldAntialias(fl_gc, false);
  if ( cb )
  {
    uchar *tmpBuf = new uchar[ W*4 ];
    for ( int i=0; i<H; i++ )
    {
      uchar *src = tmpBuf;
      cb( userdata, 0, i, W, tmpBuf );
      for ( int j=0; j<W; j++ )
      {
        if ( mono )
          { fl_color( src[0], src[0], src[0] ); }
        else
          { fl_color( src[0], src[1], src[2] ); }
        CGContextMoveToPoint(fl_gc, X+j, Y+i);
        CGContextAddLineToPoint(fl_gc, X+j, Y+i);
        CGContextStrokePath(fl_gc);
        src+=delta;
      }
    }
    delete[] tmpBuf;
  }
  else
  {
    for ( int i=0; i<H; i++ )
    {
      const uchar *src = buf+i*linedelta;
      for ( int j=0; j<W; j++ )
      {
        if ( mono )
          fl_color( src[0], src[0], src[0] );
        else
          fl_color( src[0], src[1], src[2] );
        CGContextMoveToPoint(fl_gc, X+j, Y+i);
        CGContextAddLineToPoint(fl_gc, X+j, Y+i);
        CGContextStrokePath(fl_gc);
        src += delta;
      }
    }
  }
  CGContextSetShouldAntialias(fl_gc, true);
#else
# error : you must defined __APPLE_QD__ or __APPLE_QUARTZ__
#endif
}

void fl_draw_image(const uchar* buf, int x, int y, int w, int h, int d, int l){
  innards(buf,x,y,w,h,d,l,(d<3&&d>-3),0,0);
}
void fl_draw_image(Fl_Draw_Image_Cb cb, void* data,
		   int x, int y, int w, int h,int d) {
  innards(0,x,y,w,h,d,0,(d<3&&d>-3),cb,data);
}
void fl_draw_image_mono(const uchar* buf, int x, int y, int w, int h, int d, int l){
  innards(buf,x,y,w,h,d,l,1,0,0);
}
void fl_draw_image_mono(Fl_Draw_Image_Cb cb, void* data,
		   int x, int y, int w, int h,int d) {
  innards(0,x,y,w,h,d,0,1,cb,data);
}

void fl_rectf(int x, int y, int w, int h, uchar r, uchar g, uchar b) {
  fl_color(r,g,b);
  fl_rectf(x,y,w,h);
}

//
// End of "$Id: fl_draw_image_mac.cxx 5614 2007-01-18 15:25:09Z matt $".
//
