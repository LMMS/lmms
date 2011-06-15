//
// "$Id: Fl_PostScript.H 8699 2011-05-20 16:39:06Z manolo $"
//
// Support for graphics output to PostScript file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2010-2011 by Bill Spitzak and others.
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

/**
 \brief PostScript graphical backend.
 *
 PostScript text uses vectorial fonts for the latin alphabet (exactly: all unicode
 characters between U+0020 and U+017F, that is, ASCII, Latin-1 Supplement and Latin Extended-A charts)
 plus a few other characters:
 <table>
 <tr> <th>Char</th><th>Codepoint</th><th>Name</th>  <th>Char</th><th>Codepoint</th><th>Name</th> <th>Char</th><th>Codepoint</th><th>Name</th></tr>
 <tr><td>ƒ</td><td>U+0192</td><td>florin</td><td>‚</td><td>U+201A</td><td>quotesinglbase</td><td>™</td><td>U+2122</td><td>trademark</td></tr>
 <tr><td>ˆ</td><td>U+02C6</td><td>circumflex</td><td>“</td><td>U+201C</td><td>quotedblleft</td><td>∂</td><td>U+2202</td><td>partialdiff</td></tr>
 <tr><td>ˇ</td><td>U+02C7</td><td>caron</td><td>”</td><td>U+201D</td><td>quotedblright</td><td>Δ</td><td>U+2206</td><td>Delta</td></tr>
 <tr><td>˘</td><td>U+02D8</td><td>breve</td><td>„</td><td>U+201E</td><td>quotedblbase</td><td>∑</td><td>U+2211</td><td>summation</td></tr>
 <tr><td>˙</td><td>U+02D9</td><td>dotaccent</td><td>†</td><td>U+2020</td><td>dagger</td><td>√</td><td>U+221A</td><td>radical</td></tr>
 <tr><td>˚</td><td>U+02DA</td><td>ring</td><td>‡</td><td>U+2021</td><td>daggerdbl</td><td>∞</td><td>U+221E</td><td>infinity</td></tr>
 <tr><td>˛</td><td>U+02DB</td><td>ogonek</td><td>•</td><td>U+2022</td><td>bullet</td><td>≠</td><td>U+2260</td><td>notequal</td></tr>
 <tr><td>˜</td><td>U+02DC</td><td>tilde</td><td>…</td><td>U+2026</td><td>ellipsis</td><td>≤</td><td>U+2264</td><td>lessequal</td></tr>
 <tr><td>˝</td><td>U+02DD</td><td>hungarumlaut</td><td>‰</td><td>U+2030</td><td>perthousand</td><td>≥</td><td>U+2265</td><td>greaterequal</td></tr>
 <tr><td>–</td><td>U+2013</td><td>endash</td><td>‹</td><td>U+2039</td><td>guilsinglleft</td><td>◊</td><td>U+25CA</td><td>lozenge</td></tr>
 <tr><td>—</td><td>U+2014</td><td>emdash</td><td>›</td><td>U+203A</td><td>guilsinglright</td><td>ﬁ</td><td>U+FB01</td><td>fi</td></tr>
 <tr><td>‘</td><td>U+2018</td><td>quoteleft</td><td>/</td><td>U+2044</td><td>fraction</td><td>ﬂ</td><td>U+FB02</td><td>fl</td></tr>
 <tr><td>’</td><td>U+2019</td><td>quoteright</td><td>€</td><td>U+20AC</td><td>Euro</td><td></td><td>U+F8FF</td><td>apple (Mac OS only)</td></tr>
 </table>
 <br> All other unicode characters are output as a bitmap.
 <br> FLTK standard fonts are output using PostScript standard fonts: Helvetica, Courier, 
 Times (and their bold, oblique, italic variants).
 */
class FL_EXPORT Fl_PostScript_Graphics_Driver : public Fl_Graphics_Driver {
public: 
  static const char *class_id;
  const char *class_name() {return class_id;};
  Fl_PostScript_Graphics_Driver();
#ifndef FL_DOXYGEN
  enum SHAPE{NONE=0, LINE, LOOP, POLYGON, POINTS};
  
class Clip {
  public:
    int x, y, w, h;
    Clip *prev;
  };
  Clip * clip_;
  
  int lang_level_;
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
  
  uchar bg_r, bg_g, bg_b;
  int start_postscript (int pagecount, enum Fl_Paged_Device::Page_Format format, enum Fl_Paged_Device::Page_Layout layout);
  /*  int alpha_mask(const uchar * data, int w, int h, int D, int LD=0);
   */
  void transformed_draw(const char* s, int n, double x, double y); //precise text placing
  void transformed_draw(const char* s, double x, double y);
  int alpha_mask(const uchar * data, int w, int h, int D, int LD=0);
  
  enum Fl_Paged_Device::Page_Format page_format_;
  char *ps_filename_;
 
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
#endif // FL_DOXYGEN
  
  // implementation of drawing methods
  void color(Fl_Color c);
  void color(uchar r, uchar g, uchar b);
  
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
    
  void draw_image(const uchar* d, int x,int y,int w,int h, int delta=3, int ldelta=0);
  void draw_image_mono(const uchar* d, int x,int y,int w,int h, int delta=1, int ld=0);
  void draw_image(Fl_Draw_Image_Cb call, void* data, int x,int y, int w, int h, int delta=3);
  void draw_image_mono(Fl_Draw_Image_Cb call, void* data, int x,int y, int w, int h, int delta=1);
      
  void draw(const char* s, int n, int x, int y) {transformed_draw(s,n,x,y); };
#ifdef __APPLE__
  void draw(const char* s, int n, float x, float y) {transformed_draw(s,n,x,y); };
#endif
  void draw(int angle, const char *str, int n, int x, int y);
  void rtl_draw(const char* s, int n, int x, int y);
  void font(int face, int size);
  double width(const char *, int);
  void text_extents(const char *c, int n, int &dx, int &dy, int &w, int &h);
  int height();
  int descent();
  void draw(Fl_Pixmap * pxm,int XP, int YP, int WP, int HP, int cx, int cy);
  void draw(Fl_Bitmap * bitmap,int XP, int YP, int WP, int HP, int cx, int cy);
  void draw(Fl_RGB_Image * rgb,int XP, int YP, int WP, int HP, int cx, int cy);
  ~Fl_PostScript_Graphics_Driver();
};

/**
 \brief To send graphical output to a PostScript file.
 */
class FL_EXPORT Fl_PostScript_File_Device : public Fl_Paged_Device {
#ifdef __APPLE__
  Fl_CGContextRef gc;
#endif
protected:
  Fl_PostScript_Graphics_Driver *driver();
public:
  static const char *class_id;
  const char *class_name() {return class_id;};
  Fl_PostScript_File_Device();
  ~Fl_PostScript_File_Device();
  int start_job(int pagecount, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4, 
		enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT);
  int start_job(FILE *ps_output, int pagecount, enum Fl_Paged_Device::Page_Format format = Fl_Paged_Device::A4, 
		enum Fl_Paged_Device::Page_Layout layout = Fl_Paged_Device::PORTRAIT);
  int start_page (void);
  int printable_rect(int *w, int *h);
  void margins(int *left, int *top, int *right, int *bottom);
  void origin(int *x, int *y);
  void origin(int x, int y);
  void scale (float scale_x, float scale_y = 0.);
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
// End of "$Id: Fl_PostScript.H 8699 2011-05-20 16:39:06Z manolo $"
//
