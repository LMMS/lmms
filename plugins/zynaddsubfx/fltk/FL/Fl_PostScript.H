//
// "$Id: Fl_PostScript.H 7622 2010-05-27 17:50:51Z manolo $"
//
// Support for graphics output to PostScript file for the Fast Light Tool Kit (FLTK).
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
/** \file Fl_PostScript.H 
 \brief declaration of classes Fl_PostScript_Graphics_Driver, Fl_PostScript_File_Device.
 */


#ifndef Fl_PostScript_H
#define Fl_PostScript_H

#include <FL/Fl_Paged_Device.H>
#include <FL/fl_draw.H>

#define NO_PAGE_FORMATS 30 /* MSVC6 compilation fix */

/**
 \brief PostScript graphical backend.
 *
 PostScript text output is presently implemented only for the latin character set.
 FLTK's standard fonts are output using PostScript's standard fonts: Helvetica, Courier, 
 Times (and their bold, oblique, italic variants), Symbol, ZapfDingbats.
 */
class Fl_PostScript_Graphics_Driver : public Fl_Graphics_Driver {
public: 
  static const char *device_type;
  /**
   \brief Possible page formats.
   */
  enum Page_Format {
    A0 = 0,
    A1,
    A2,
    A3,
    A4,
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
    LETTER,
    TABLOID,
    ENVELOPE,
    MEDIA = 0x1000
  };
  
  /**
   \brief Possible page layouts.
   */
  enum Page_Layout {PORTRAIT = 0, LANDSCAPE = 0x100, REVERSED = 0x200, ORIENTATION = 0x300};
    
#ifndef FL_DOXYGEN
public:
  enum SHAPE{NONE=0, LINE, LOOP, POLYGON, POINTS};
  
typedef struct page_format {
  int width;
  int height;
  const char *name;
} page_format;
  
class Clip {
  public:
    int x, y, w, h;
    Clip *prev;
  };
  Clip * clip_;
  
  int lang_level_;
  int font_;
  int size_;
  Fl_Color color_;
  int gap_;
  int pages_;
  
  double width_;
  double height_;
  
  int shape_;
  int linewidth_;// need for clipping, lang level 1-2
  int linestyle_;//
  int interpolate_; //interpolation of images
  unsigned char cr_,cg_,cb_;
  char  linedash_[256];//should be enough
  void concat();  // transform ror scalable dradings...
  void reconcat(); //invert
  void recover(); //recovers the state after grestore (such as line styles...)
  void reset();
  
  uchar * mask;
  int mx; // width of mask;
  int my; // mask lines
  //Fl_Color bg_;
  int (*close_cmd_)(FILE *);
  int page_policy_;
  int nPages;
  int orientation_;
  
  float scale_x;
  float scale_y;
  float angle;
  int left_margin;
  int top_margin;
 
  FILE *output;
  double pw_, ph_;
  static const page_format page_formats[NO_PAGE_FORMATS];
  
  uchar bg_r, bg_g, bg_b;
  int start_postscript (int pagecount, enum Page_Format format, enum Page_Layout layout);
  /*  int alpha_mask(const uchar * data, int w, int h, int D, int LD=0);
   */
  void draw(const char* s, int n, int x, int y) {transformed_draw(s,n,x,y); };
  void draw(int angle, const char *str, int n, int x, int y);
  void transformed_draw(const char* s, int n, double x, double y); //precise text placing
  void transformed_draw(const char* s, double x, double y);
  int alpha_mask(const uchar * data, int w, int h, int D, int LD=0);
  void draw_scaled_image(const uchar *data, double x, double y, double w, double h, int iw, int ih, int D=3, int LD=0);
  void draw_scaled_image_mono(const uchar *data, double x, double y, double w, double h, int iw, int ih, int D=3, int LD=0);
  void draw_scaled_image(Fl_Draw_Image_Cb call, void *data, double x, double y, double w, double h, int iw, int ih, int D);
  void draw_scaled_image_mono(Fl_Draw_Image_Cb call, void *data, double x, double y, double w, double h, int iw, int ih, int D);
  
  enum Page_Format page_format_;
  char *ps_filename_;
  // implementation of drawing methods
  void color(Fl_Color c);
  //void bg_color(Fl_Color bg);
  void color(uchar r, uchar g, uchar b);
  Fl_Color color(){return color_;};
  
  void push_clip(int x, int y, int w, int h);
  int clip_box(int x, int y, int w, int h, int &X, int &Y, int &W, int &H);
  int not_clipped(int x, int y, int w, int h);
  void push_no_clip();
  void pop_clip();
  
  void line_style(int style, int width=0, char* dashes=0);
  
  void rect(int x, int y, int w, int h);
  void rectf(int x, int y, int w, int h);
  
  void xyline(int x, int y, int x1);
  void xyline(int x, int y, int x1, int y2);
  void xyline(int x, int y, int x1, int y2, int x3);
  
  void yxline(int x, int y, int y1);
  void yxline(int x, int y, int y1, int x2);
  void yxline(int x, int y, int y1, int x2, int y3);
  
  void line(int x1, int y1, int x2, int y2);
  void line(int x1, int y1, int x2, int y2, int x3, int y3);  
  
  void loop(int x0, int y0, int x1, int y1, int x2, int y2);
  void loop(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  void polygon(int x0, int y0, int x1, int y1, int x2, int y2);
  void polygon(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3);
  void point(int x, int y);
  
  void begin_points();
  void begin_line();
  void begin_loop();
  void begin_polygon();
  void vertex(double x, double y);
  void curve(double x, double y, double x1, double y1, double x2, double y2, double x3, double y3);
  void circle(double x, double y, double r);
  void arc(double x, double y, double r, double start, double a);
  void arc(int x, int y, int w, int h, double a1, double a2);
  void pie(int x, int y, int w, int h, double a1, double a2);
  void end_points();
  void end_line();
  void end_loop();
  void end_polygon();
  void begin_complex_polygon(){begin_polygon();};
  void gap(){gap_=1;};
  void end_complex_polygon(){end_polygon();};
  void transformed_vertex(double x, double y);
  
  void font(int face, int size);
  int font(){return font_;};
  int size(){return size_;};
  double width(unsigned c);
  double width(const char* s, int n);
  int descent();
  int height();
  
  void draw_image(const uchar* d, int x,int y,int w,int h, int delta=3, int ldelta=0){draw_scaled_image(d,x,y,w,h,w,h,delta,ldelta);};
  void draw_image_mono(const uchar* d, int x,int y,int w,int h, int delta=1, int ld=0){draw_scaled_image_mono(d,x,y,w,h,w,h,delta,ld);};
  void draw_image(Fl_Draw_Image_Cb call, void* data, int x,int y, int w, int h, int delta=3){draw_scaled_image(call,data, x, y, w, h, w, h, delta);};
  void draw_image_mono(Fl_Draw_Image_Cb call, void* data, int x,int y, int w, int h, int delta=1){draw_scaled_image_mono(call, data, x, y, w, h, w, h, delta);};
  
  void draw(Fl_Pixmap * pxm,int XP, int YP, int WP, int HP, int cx, int cy);
  void draw(Fl_RGB_Image * rgb,int XP, int YP, int WP, int HP, int cx, int cy);
  void draw(Fl_Bitmap * bitmap,int XP, int YP, int WP, int HP, int cx, int cy);
    
public:
  void page_policy(int p);
  int page_policy(){return page_policy_;};
  void close_command( int (*cmd)(FILE *)){close_cmd_=cmd;};
  FILE * file() {return output;};
  //void orientation (int o);
  //Fl_PostScript_Graphics_Driver(FILE *o, int lang_level, int pages = 0); // ps (also multi-page) constructor
  //Fl_PostScript_Graphics_Driver(FILE *o, int lang_level, int x, int y, int w, int h); //eps constructor
  void interpolate(int i){interpolate_=i;};
  int interpolate(){return interpolate_;}
  
  void page(double pw, double ph, int media = 0);
  void page(int format);
  
  void place(double x, double y, double tx, double ty, double scale = 1);
#endif // FL_DOXYGEN
  Fl_PostScript_Graphics_Driver();
  ~Fl_PostScript_Graphics_Driver();
};

/**
 \brief To send graphical output to a PostScript file.
 */
class Fl_PostScript_File_Device : public Fl_Paged_Device {
#ifdef __APPLE__
  CGContextRef gc;
#endif
protected:
  Fl_PostScript_Graphics_Driver *driver();
public:
  static const char *device_type;
  Fl_PostScript_File_Device();
  ~Fl_PostScript_File_Device();
  int start_job(int pagecount, enum Fl_PostScript_Graphics_Driver::Page_Format format = Fl_PostScript_Graphics_Driver::A4, 
		enum Fl_PostScript_Graphics_Driver::Page_Layout layout = Fl_PostScript_Graphics_Driver::PORTRAIT);
  int start_job(FILE *ps_output, int pagecount, enum Fl_PostScript_Graphics_Driver::Page_Format format = Fl_PostScript_Graphics_Driver::A4, 
		enum Fl_PostScript_Graphics_Driver::Page_Layout layout = Fl_PostScript_Graphics_Driver::PORTRAIT);
  int start_page (void);
  int printable_rect(int *w, int *h);
  void margins(int *left, int *top, int *right, int *bottom);
  void origin(int x, int y);
  void scale (float scale_x, float scale_y);
  void rotate(float angle);
  void translate(int x, int y);
  void untranslate(void);
  int end_page (void);    
  void end_job(void);
#ifdef __APPLE__
  void set_current() { fl_gc = gc; Fl_Paged_Device::set_current(); }
#endif
  
  static const char *file_chooser_title; 
};

#endif // Fl_PostScript_H

//
// End of "$Id: Fl_PostScript.H 7622 2010-05-27 17:50:51Z manolo $"
//
