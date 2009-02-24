//
// "$Id: Fl_Pixmap.H 4288 2005-04-16 00:13:17Z mike $"
//
// Pixmap header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Pixmap_H
#define Fl_Pixmap_H
#  include "Fl_Image.H"

class Fl_Widget;
struct Fl_Menu_Item;

// Older C++ compilers don't support the explicit keyword... :(
#  if defined(__sgi) && !defined(_COMPILER_VERSION)
#    define explicit
#  endif // __sgi && !_COMPILER_VERSION

class FL_EXPORT Fl_Pixmap : public Fl_Image {
  void copy_data();
  void delete_data();
  void set_data(const char * const *p);

  protected:

  void measure();

  public:

  int alloc_data; // Non-zero if data was allocated
#if defined(__APPLE__) || defined(WIN32)
  void *id; // for internal use
  void *mask; // for internal use (mask bitmap)
#else
  unsigned id; // for internal use
  unsigned mask; // for internal use (mask bitmap)
#endif // __APPLE__ || WIN32
  
  explicit Fl_Pixmap(char * const * D) : Fl_Image(-1,0,1), alloc_data(0), id(0), mask(0) {set_data((const char*const*)D); measure();}
  explicit Fl_Pixmap(uchar* const * D) : Fl_Image(-1,0,1), alloc_data(0), id(0), mask(0) {set_data((const char*const*)D); measure();}
  explicit Fl_Pixmap(const char * const * D) : Fl_Image(-1,0,1), alloc_data(0), id(0), mask(0) {set_data((const char*const*)D); measure();}
  explicit Fl_Pixmap(const uchar* const * D) : Fl_Image(-1,0,1), alloc_data(0), id(0), mask(0) {set_data((const char*const*)D); measure();}
  virtual ~Fl_Pixmap();
  virtual Fl_Image *copy(int W, int H);
  Fl_Image *copy() { return copy(w(), h()); }
  virtual void color_average(Fl_Color c, float i);
  virtual void desaturate();
  virtual void draw(int X, int Y, int W, int H, int cx=0, int cy=0);
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);}
  virtual void label(Fl_Widget*w);
  virtual void label(Fl_Menu_Item*m);
  virtual void uncache();
};

#endif

//
// End of "$Id: Fl_Pixmap.H 4288 2005-04-16 00:13:17Z mike $".
//
