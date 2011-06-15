//
// "$Id: fl_cursor.cxx 8055 2010-12-18 22:31:01Z manolo $"
//
// Mouse cursor support for the Fast Light Tool Kit (FLTK).
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

// Change the current cursor.
// Under X the cursor is attached to the X window.  I tried to hide
// this and pretend that changing the cursor is a drawing function.
// This avoids a field in the Fl_Window, and I suspect is more
// portable to other systems.

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/x.H>
#if !defined(WIN32) && !defined(__APPLE__)
#  include <X11/cursorfont.h>
#endif
#include <FL/fl_draw.H>

/**
  Sets the cursor for the current window to the specified shape and colors.
  The cursors are defined in the <FL/Enumerations.H> header file. 
  */
void fl_cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg) {
  if (Fl::first_window()) Fl::first_window()->cursor(c,fg,bg);
}
/** 
    Sets the default window cursor as well as its color.

    For back compatibility only.
*/
void Fl_Window::default_cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg) {
//  if (c == FL_CURSOR_DEFAULT) c = FL_CURSOR_ARROW;

  cursor_default = c;
  cursor_fg      = fg;
  cursor_bg      = bg;

  cursor(c, fg, bg);
}

#ifdef WIN32

#  ifndef IDC_HAND
#    define IDC_HAND	MAKEINTRESOURCE(32649)
#  endif // !IDC_HAND

void Fl_Window::cursor(Fl_Cursor c, Fl_Color c1, Fl_Color c2) {
  if (!shown()) return;
  // the cursor must be set for the top level window, not for subwindows
  Fl_Window *w = window(), *toplevel = this;
  while (w) { toplevel = w; w = w->window(); }
  if (toplevel != this) { toplevel->cursor(c, c1, c2); return; }
  // now set the actual cursor
  if (c == FL_CURSOR_DEFAULT) {
    c = cursor_default;
  }
  if (c > FL_CURSOR_NESW) {
    i->cursor = 0;
  } else if (c == FL_CURSOR_DEFAULT) {
    i->cursor = fl_default_cursor;
  } else {
    LPSTR n;
    switch (c) {
    case FL_CURSOR_ARROW:	n = IDC_ARROW; break;
    case FL_CURSOR_CROSS:	n = IDC_CROSS; break;
    case FL_CURSOR_WAIT:	n = IDC_WAIT; break;
    case FL_CURSOR_INSERT:	n = IDC_IBEAM; break;
    case FL_CURSOR_HELP:	n = IDC_HELP; break;
    case FL_CURSOR_HAND: {
          OSVERSIONINFO osvi;

          // Get the OS version: Windows 98 and 2000 have a standard
	  // hand cursor.
          memset(&osvi, 0, sizeof(OSVERSIONINFO));
          osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
          GetVersionEx(&osvi);

          if (osvi.dwMajorVersion > 4 ||
  	      (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion > 0 &&
  	       osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)) n = IDC_HAND;
          else n = IDC_UPARROW;
	} break;
    case FL_CURSOR_MOVE:	n = IDC_SIZEALL; break;
    case FL_CURSOR_N:
    case FL_CURSOR_S:
    case FL_CURSOR_NS:		n = IDC_SIZENS; break;
    case FL_CURSOR_NE:
    case FL_CURSOR_SW:
    case FL_CURSOR_NESW:	n = IDC_SIZENESW; break;
    case FL_CURSOR_E:
    case FL_CURSOR_W:
    case FL_CURSOR_WE:		n = IDC_SIZEWE; break;
    case FL_CURSOR_SE:
    case FL_CURSOR_NW:
    case FL_CURSOR_NWSE:	n = IDC_SIZENWSE; break;
    default:			n = IDC_NO; break;
    }
    i->cursor = LoadCursor(NULL, n);
  }
  SetCursor(i->cursor);
}

#elif defined(__APPLE__)

#ifdef __BIG_ENDIAN__
# define E(x) x
#elif defined __LITTLE_ENDIAN__
// Don't worry. This will be resolved at compile time
# define E(x) (x>>8)|((x<<8)&0xff00)
#else
# error "Either __LITTLE_ENDIAN__ or __BIG_ENDIAN__ must be defined"
#endif

extern Fl_Offscreen fl_create_offscreen_with_alpha(int w, int h);


CGContextRef Fl_X::help_cursor_image(void)
{
  int w = 20, h = 20;
  Fl_Offscreen off = fl_create_offscreen_with_alpha(w, h);
  fl_begin_offscreen(off);
  CGContextSetRGBFillColor( (CGContextRef)off, 0,0,0,0);
  fl_rectf(0,0,w,h);
  fl_color(FL_BLACK);
  fl_font(FL_COURIER_BOLD, 20);
  fl_draw("?", 1, h-1);
  fl_end_offscreen();
  return (CGContextRef)off;
}

CGContextRef Fl_X::none_cursor_image(void)
{
  int w = 20, h = 20;
  Fl_Offscreen off = fl_create_offscreen_with_alpha(w, h);
  fl_begin_offscreen(off);
  CGContextSetRGBFillColor( (CGContextRef)off, 0,0,0,0);
  fl_rectf(0,0,w,h);
  fl_end_offscreen();
  return (CGContextRef)off;
}

CGContextRef Fl_X::watch_cursor_image(void)
{
  int w, h, r = 5;
  w = 2*r+6;
  h = 4*r;
  Fl_Offscreen off = fl_create_offscreen_with_alpha(w, h);
  fl_begin_offscreen(off);
  CGContextSetRGBFillColor( (CGContextRef)off, 0,0,0,0);
  fl_rectf(0,0,w,h);
  CGContextTranslateCTM( (CGContextRef)off, w/2, h/2);
  fl_color(FL_WHITE);
  fl_circle(0, 0, r+1);
  fl_color(FL_BLACK);
  fl_rectf(int(-r*0.7), int(-r*1.7), int(1.4*r), int(3.4*r));
  fl_rectf(r-1, -1, 3, 3);
  fl_color(FL_WHITE);
  fl_pie(-r, -r, 2*r, 2*r, 0, 360);
  fl_color(FL_BLACK);
  fl_circle(0,0,r);
  fl_xyline(0, 0, int(-r*.7));
  fl_xyline(0, 0, 0, int(-r*.7));
  fl_end_offscreen();
  return (CGContextRef)off;
}

CGContextRef Fl_X::nesw_cursor_image(void)
{
  int c = 7, r = 2*c;
  int w = r, h = r;
  Fl_Offscreen off = fl_create_offscreen_with_alpha(w, h);
  fl_begin_offscreen(off);
  CGContextSetRGBFillColor( (CGContextRef)off, 0,0,0,0);
  fl_rectf(0,0,w,h);
  CGContextTranslateCTM( (CGContextRef)off, 0, h);
  CGContextScaleCTM( (CGContextRef)off, 1, -1);
  fl_color(FL_BLACK);
  fl_polygon(0, 0, c, 0, 0, c);
  fl_polygon(r, r, r, r-c, r-c, r);
  fl_line_style(FL_SOLID, 2, 0);
  fl_line(0,1, r,r+1);
  fl_line_style(FL_SOLID, 0, 0);
  fl_end_offscreen();
  return (CGContextRef)off;
}

CGContextRef Fl_X::nwse_cursor_image(void)
{
  int c = 7, r = 2*c;
  int w = r, h = r;
  Fl_Offscreen off = fl_create_offscreen_with_alpha(w, h);
  fl_begin_offscreen(off);
  CGContextSetRGBFillColor( (CGContextRef)off, 0,0,0,0);
  fl_rectf(0,0,w,h);
  CGContextTranslateCTM( (CGContextRef)off, 0, h);
  CGContextScaleCTM( (CGContextRef)off, 1, -1);
  fl_color(FL_BLACK);
  fl_polygon(r-1, 0, r-1, c, r-1-c, 0);
  fl_polygon(-1, r, c-1, r, -1, r-c);
  fl_line_style(FL_SOLID, 2, 0);
  fl_line(r-1,1, -1,r+1);
  fl_line_style(FL_SOLID, 0, 0);
  fl_end_offscreen();
  return (CGContextRef)off;
}

void Fl_Window::cursor(Fl_Cursor c, Fl_Color, Fl_Color) {
  if (c == FL_CURSOR_DEFAULT) {
    c = cursor_default;
  }
  if (i) i->set_cursor(c);
}

#else

// I like the MSWindows resize cursors, so I duplicate them here:

#define CURSORSIZE 16
#define HOTXY 7
static struct TableEntry {
  uchar bits[CURSORSIZE*CURSORSIZE/8];
  uchar mask[CURSORSIZE*CURSORSIZE/8];
  Cursor cursor;
} table[] = {
  {{	// FL_CURSOR_NS
   0x00, 0x00, 0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0x80, 0x01, 0x80, 0x01,
   0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
   0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01, 0x00, 0x00},
   {
   0x80, 0x01, 0xc0, 0x03, 0xe0, 0x07, 0xf0, 0x0f, 0xf0, 0x0f, 0xc0, 0x03,
   0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xf0, 0x0f,
   0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0x80, 0x01}},
  {{	// FL_CURSOR_EW
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10,
   0x0c, 0x30, 0xfe, 0x7f, 0xfe, 0x7f, 0x0c, 0x30, 0x08, 0x10, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x1c, 0x38,
   0xfe, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7f, 0x1c, 0x38, 0x18, 0x18,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {{	// FL_CURSOR_NWSE
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x00, 0x38, 0x00, 0x78, 0x00,
   0xe8, 0x00, 0xc0, 0x01, 0x80, 0x03, 0x00, 0x17, 0x00, 0x1e, 0x00, 0x1c,
   0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0x7c, 0x00, 0xfc, 0x00,
   0xfc, 0x01, 0xec, 0x03, 0xc0, 0x37, 0x80, 0x3f, 0x00, 0x3f, 0x00, 0x3e,
   0x00, 0x3f, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x00}},
  {{	// FL_CURSOR_NESW
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x1c, 0x00, 0x1e,
   0x00, 0x17, 0x80, 0x03, 0xc0, 0x01, 0xe8, 0x00, 0x78, 0x00, 0x38, 0x00,
   0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
   {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3e, 0x00, 0x3f,
   0x80, 0x3f, 0xc0, 0x37, 0xec, 0x03, 0xfc, 0x01, 0xfc, 0x00, 0x7c, 0x00,
   0xfc, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00}},
  {{0}, {0}} // FL_CURSOR_NONE & unknown
};

void Fl_Window::cursor(Fl_Cursor c, Fl_Color fg, Fl_Color bg) {
  if (!shown()) return;
  Cursor xc;
  int deleteit = 0;
  if (c == FL_CURSOR_DEFAULT) {
    c  = cursor_default;
    fg = cursor_fg;
    bg = cursor_bg;
  }

  if (!c) {
    xc = None;
  } else {
    if (c >= FL_CURSOR_NS) {
      TableEntry *q = (c > FL_CURSOR_NESW) ? table+4 : table+(c-FL_CURSOR_NS);
      if (!(q->cursor)) {
	XColor dummy = { 0 };
	Pixmap p = XCreateBitmapFromData(fl_display,
	  RootWindow(fl_display, fl_screen), (const char*)(q->bits),
	  CURSORSIZE, CURSORSIZE);
	Pixmap m = XCreateBitmapFromData(fl_display,
	  RootWindow(fl_display, fl_screen), (const char*)(q->mask),
	  CURSORSIZE, CURSORSIZE);
	q->cursor = XCreatePixmapCursor(fl_display, p,m,&dummy, &dummy,
					HOTXY, HOTXY);
	XFreePixmap(fl_display, m);
	XFreePixmap(fl_display, p);
      }
      xc = q->cursor;
    } else {
      xc = XCreateFontCursor(fl_display, (c-1)*2);
      deleteit = 1;
    }
    XColor fgc;
    uchar r,g,b;
    Fl::get_color(fg,r,g,b);
    fgc.red = r<<8; fgc.green = g<<8; fgc.blue = b<<8;
    XColor bgc;
    Fl::get_color(bg,r,g,b);
    bgc.red = r<<8; bgc.green = g<<8; bgc.blue = b<<8;
    XRecolorCursor(fl_display, xc, &fgc, &bgc);
  }
  XDefineCursor(fl_display, fl_xid(this), xc);
  if (deleteit) XFreeCursor(fl_display, xc);
}

#endif

//
// End of "$Id: fl_cursor.cxx 8055 2010-12-18 22:31:01Z manolo $".
//
