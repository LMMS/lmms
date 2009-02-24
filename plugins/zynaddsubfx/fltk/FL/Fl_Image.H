//
// "$Id: Fl_Image.H 4288 2005-04-16 00:13:17Z mike $"
//
// Image header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_Image_H
#  define Fl_Image_H

#  include "Enumerations.H"

class Fl_Widget;
struct Fl_Menu_Item;
struct Fl_Label;

class FL_EXPORT Fl_Image {
  int w_, h_, d_, ld_, count_;
  const char * const *data_;

  // Forbid use of copy contructor and assign operator
  Fl_Image & operator=(const Fl_Image &);
  Fl_Image(const Fl_Image &);

  protected:

  void w(int W) {w_ = W;}
  void h(int H) {h_ = H;}
  void d(int D) {d_ = D;}
  void ld(int LD) {ld_ = LD;}
  void data(const char * const *p, int c) {data_ = p; count_ = c;}
  void draw_empty(int X, int Y);

  static void labeltype(const Fl_Label *lo, int lx, int ly, int lw, int lh, Fl_Align la);
  static void measure(const Fl_Label *lo, int &lw, int &lh);

  public:

  int w() const {return w_;}
  int h() const {return h_;}
  int d() const {return d_;}
  int ld() const {return ld_;}
  int count() const {return count_;}
  const char * const *data() const {return data_;}
  
  Fl_Image(int W, int H, int D) {w_ = W; h_ = H; d_ = D; ld_ = 0; count_ = 0; data_ = 0;}
  virtual ~Fl_Image();
  virtual Fl_Image *copy(int W, int H);
  Fl_Image *copy() { return copy(w(), h()); }
  virtual void color_average(Fl_Color c, float i);
  void inactive() { color_average(FL_GRAY, .33f); }
  virtual void desaturate();
  virtual void label(Fl_Widget*w);
  virtual void label(Fl_Menu_Item*m);
  virtual void draw(int X, int Y, int W, int H, int cx=0, int cy=0);
  void draw(int X, int Y) {draw(X, Y, w(), h(), 0, 0);}
  virtual void uncache();
};

class FL_EXPORT Fl_RGB_Image : public Fl_Image {
  public:

  const uchar *array;
  int alloc_array; // Non-zero if array was allocated

#if defined(__APPLE__) || defined(WIN32)
  void *id; // for internal use
  void *mask; // for internal use (mask bitmap)
#else
  unsigned id; // for internal use
  unsigned mask; // for internal use (mask bitmap)
#endif // __APPLE__ || WIN32

  Fl_RGB_Image(const uchar *bits, int W, int H, int D=3, int LD=0) :
    Fl_Image(W,H,D), array(bits), alloc_array(0), id(0), mask(0) {data((const char **)&array, 1); ld(LD);}
  virtual ~Fl_RGB_Image();
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

#endif // !Fl_Image_H

//
// End of "$Id: Fl_Image.H 4288 2005-04-16 00:13:17Z mike $".
//
