//
// "$Id: Fl_Paged_Device.H 8699 2011-05-20 16:39:06Z manolo $"
//
// Printing support for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010 by Bill Spitzak and others.
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

/** \file Fl_Paged_Device.H 
 \brief declaration of class Fl_Paged_Device.
 */

#ifndef Fl_Paged_Device_H
#define Fl_Paged_Device_H

#include <FL/Fl_Device.H>
#include <FL/Fl_Window.H>

/** \brief Number of elements in enum Page_Format */
#define NO_PAGE_FORMATS 30 /* MSVC6 compilation fix */

/**
 \brief Represents page-structured drawing surfaces.
 *
 This class has no public constructor: don't instantiate it; use Fl_Printer 
 or Fl_PostScript_File_Device instead.
 */
class FL_EXPORT Fl_Paged_Device : public Fl_Surface_Device {
public:
  /**
   \brief Possible page formats.
   
   All paper formats with pre-defined width and height.
   */
  enum Page_Format {
    A0 = 0, /**<  A0 format */
    A1,
    A2,
    A3,
    A4, /**<  A4 format */
    A5,
    A6,
    A7,
    A8,
    A9,
    B0,
    B1,
    B2,
    B3,
    B4,
    B5,
    B6,
    B7,
    B8,
    B9,
    B10,
    C5E,
    DLE,
    EXECUTIVE,
    FOLIO,
    LEDGER,
    LEGAL,
    LETTER, /**<  Letter format */
    TABLOID,
    ENVELOPE,
    MEDIA = 0x1000
  };
  /**
   \brief Possible page layouts.
   */
  enum Page_Layout {
    PORTRAIT = 0,  /**< Portrait orientation */
    LANDSCAPE = 0x100,   /**< Landscape orientation */
    REVERSED = 0x200,  /**< Reversed orientation */
    ORIENTATION = 0x300 /**<  orientation */
  };

  /** \brief width, height and name of a page format
 */
  typedef struct {
    /** \brief width in points */
    int width; 
    /** \brief height in points */
    int height; 
    /** \brief format name */
    const char *name; 
  } page_format;
  /** \brief width, height and name of all elements of the enum \ref Page_Format.
   */
  static const page_format page_formats[NO_PAGE_FORMATS];  
private:
  void traverse(Fl_Widget *widget); // finds subwindows of widget and prints them
protected:
  /** \brief horizontal offset to the origin of graphics coordinates */
  int x_offset;
  /** \brief vertical offset to the origin of graphics coordinates */
  int y_offset;
  /** \brief The constructor */
  Fl_Paged_Device() : Fl_Surface_Device(NULL) {};
  /** \brief The destructor */
  virtual ~Fl_Paged_Device() {};
public:
  static const char *class_id;
  const char *class_name() {return class_id;};
  virtual int start_job(int pagecount, int *frompage = NULL, int *topage = NULL);
  virtual int start_page(void);
  virtual int printable_rect(int *w, int *h);
  virtual void margins(int *left, int *top, int *right, int *bottom);
  virtual void origin(int x, int y);
  virtual void origin(int *x, int *y);
  virtual void scale(float scale_x, float scale_y = 0.);
  virtual void rotate(float angle);
  virtual void translate(int x, int y);
  virtual void untranslate(void);
  virtual void print_widget(Fl_Widget* widget, int delta_x = 0, int delta_y = 0);
  /** Prints a window with its title bar and frame if any.
   
   \p x_offset and \p y_offset are optional coordinates of where to position the window top left.
   Equivalent to print_widget() if \p win is a subwindow or has no border.
   Use Fl_Window::decorated_w() and Fl_Window::decorated_h() to get the size of the
   printed window.
   */
  void print_window(Fl_Window *win, int x_offset = 0, int y_offset = 0);
  virtual void print_window_part(Fl_Window *win, int x, int y, int w, int h, int delta_x = 0, int delta_y = 0);
  virtual int end_page (void);
  virtual void end_job (void);
};

#endif // Fl_Paged_Device_H

//
// End of "$Id: Fl_Paged_Device.H 8699 2011-05-20 16:39:06Z manolo $"
//

