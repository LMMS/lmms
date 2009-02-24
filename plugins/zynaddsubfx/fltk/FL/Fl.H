//
// "$Id: Fl.H 5848 2007-05-20 16:18:31Z mike $"
//
// Main header file for the Fast Light Tool Kit (FLTK).
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

#ifndef Fl_H
#  define Fl_H

#  include "Enumerations.H"
#  ifndef Fl_Object
#    define Fl_Object Fl_Widget
#  endif

#  ifdef check
#    undef check
#  endif

class Fl_Widget;
class Fl_Window;
class Fl_Image;
struct Fl_Label;
typedef void (Fl_Label_Draw_F)(const Fl_Label*, int,int,int,int, Fl_Align);
typedef void (Fl_Label_Measure_F)(const Fl_Label*, int&, int&);
typedef void (Fl_Box_Draw_F)(int,int,int,int, Fl_Color);

typedef void (*Fl_Timeout_Handler)(void*);
typedef void (*Fl_Awake_Handler)(void*);

class FL_EXPORT Fl {
  Fl() {}; // no constructor!

public: // should be private!

  static int e_number;
  static int e_x;
  static int e_y;
  static int e_x_root;
  static int e_y_root;
  static int e_dx;
  static int e_dy;
  static int e_state;
  static int e_clicks;
  static int e_is_click;
  static int e_keysym;
  static char* e_text;
  static int e_length;
  static Fl_Widget* belowmouse_;
  static Fl_Widget* pushed_;
  static Fl_Widget* focus_;
  static int damage_;
  static Fl_Widget* selection_owner_;
  static Fl_Window* modal_;
  static Fl_Window* grab_;
  static int compose_state;
  static int visible_focus_;
  static int dnd_text_ops_;
  static void damage(int d) {damage_ = d;}

  static void (*idle)();
  static Fl_Awake_Handler *awake_ring_;
  static void **awake_data_;
  static int awake_ring_size_;
  static int awake_ring_head_;
  static int awake_ring_tail_;

  static int add_awake_handler_(Fl_Awake_Handler, void*);
  static int get_awake_handler_(Fl_Awake_Handler&, void*&);

  static const char* scheme_;
  static Fl_Image* scheme_bg_;

  static int e_original_keysym; // late addition

public:

  // API version number
  static double version();

  // argument parsers:
  static int arg(int, char**, int&);
  static int args(int, char**, int&, int (*)(int,char**,int&) = 0);
  static const char* const help;
  static void args(int, char**);

  // things called by initialization:
  static void display(const char*);
  static int visual(int);
  static int gl_visual(int, int *alist=0);
  static void own_colormap();
  static void get_system_colors();
  static void foreground(uchar, uchar, uchar);
  static void background(uchar, uchar, uchar);
  static void background2(uchar, uchar, uchar);

  // schemes:
  static int scheme(const char*);
  static const char* scheme() {return scheme_;}
  static int reload_scheme();
  static int scrollbar_size();
  static void scrollbar_size(int W);

  // execution:
  static int wait();
  static double wait(double time);
  static int check();
  static int ready();
  static int run();
  static Fl_Widget* readqueue();
  static void add_timeout(double t, Fl_Timeout_Handler,void* = 0);
  static void repeat_timeout(double t, Fl_Timeout_Handler,void* = 0);
  static int  has_timeout(Fl_Timeout_Handler, void* = 0);
  static void remove_timeout(Fl_Timeout_Handler, void* = 0);
  static void add_check(Fl_Timeout_Handler, void* = 0);
  static int  has_check(Fl_Timeout_Handler, void* = 0);
  static void remove_check(Fl_Timeout_Handler, void* = 0);
  static void add_fd(int fd, int when, void (*cb)(int,void*),void* =0);
  static void add_fd(int fd, void (*cb)(int, void*), void* = 0);
  static void remove_fd(int, int when);
  static void remove_fd(int);
  static void add_idle(void (*cb)(void*), void* = 0);
  static int  has_idle(void (*cb)(void*), void* = 0);
  static void remove_idle(void (*cb)(void*), void* = 0);
  static int damage() {return damage_;}
  static void redraw();
  static void flush();
  static void (*warning)(const char*, ...);
  static void (*error)(const char*, ...);
  static void (*fatal)(const char*, ...);
  static Fl_Window* first_window();
  static void first_window(Fl_Window*);
  static Fl_Window* next_window(const Fl_Window*);
  static Fl_Window* modal() {return modal_;}
  static Fl_Window* grab() {return grab_;}
  static void grab(Fl_Window*);

  // event information:
  static int event()		{return e_number;}
  static int event_x()	{return e_x;}
  static int event_y()	{return e_y;}
  static int event_x_root()	{return e_x_root;}
  static int event_y_root()	{return e_y_root;}
  static int event_dx()	{return e_dx;}
  static int event_dy()	{return e_dy;}
  static void get_mouse(int &,int &);
  static int event_clicks()	{return e_clicks;}
  static void event_clicks(int i) {e_clicks = i;}
  static int event_is_click()	{return e_is_click;}
  static void event_is_click(int i) {e_is_click = i;} // only 0 works!
  static int event_button()	{return e_keysym-FL_Button;}
  static int event_state()	{return e_state;}
  static int event_state(int i) {return e_state&i;}
  static int event_key()	{return e_keysym;}
  static int event_original_key(){return e_original_keysym;}
  static int event_key(int);
  static int get_key(int);
  static const char* event_text() {return e_text;}
  static int event_length() {return e_length;}
  static int compose(int &del);
  static void compose_reset() {compose_state = 0;}
  static int event_inside(int,int,int,int);
  static int event_inside(const Fl_Widget*);
  static int test_shortcut(int);

  // event destinations:
  static int handle(int, Fl_Window*);
  static Fl_Widget* belowmouse() {return belowmouse_;}
  static void belowmouse(Fl_Widget*);
  static Fl_Widget* pushed()	{return pushed_;}
  static void pushed(Fl_Widget*);
  static Fl_Widget* focus()	{return focus_;}
  static void focus(Fl_Widget*);
  static void add_handler(int (*h)(int));
  static void remove_handler(int (*h)(int));

  // cut/paste:
  static void copy(const char* stuff, int len, int clipboard = 0);
  static void paste(Fl_Widget &receiver, int clipboard /*=0*/);
  static int dnd();
  // These are for back-compatability only:
  static Fl_Widget* selection_owner() {return selection_owner_;}
  static void selection_owner(Fl_Widget*);
  static void selection(Fl_Widget &owner, const char*, int len);
  static void paste(Fl_Widget &receiver);

  // screen size:
  static int x();
  static int y();
  static int w();
  static int h();

  // multi-head support:
  static int screen_count();
  static void screen_xywh(int &X, int &Y, int &W, int &H) {
    screen_xywh(X, Y, W, H, e_x_root, e_y_root);
  }
  static void screen_xywh(int &X, int &Y, int &W, int &H, int mx, int my);
  static void screen_xywh(int &X, int &Y, int &W, int &H, int n);

  // color map:
  static void	set_color(Fl_Color, uchar, uchar, uchar);
  static void	set_color(Fl_Color, unsigned);
  static unsigned get_color(Fl_Color);
  static void	get_color(Fl_Color, uchar&, uchar&, uchar&);
  static void	free_color(Fl_Color, int overlay = 0);

  // fonts:
  static const char* get_font(Fl_Font);
  static const char* get_font_name(Fl_Font, int* attributes = 0);
  static int get_font_sizes(Fl_Font, int*& sizep);
  static void set_font(Fl_Font, const char*);
  static void set_font(Fl_Font, Fl_Font);
  static Fl_Font set_fonts(const char* = 0);

  // labeltypes:
  static void set_labeltype(Fl_Labeltype,Fl_Label_Draw_F*,Fl_Label_Measure_F*);
  static void set_labeltype(Fl_Labeltype, Fl_Labeltype from);

  // boxtypes:
  static Fl_Box_Draw_F *get_boxtype(Fl_Boxtype);
  static void set_boxtype(Fl_Boxtype, Fl_Box_Draw_F*,uchar,uchar,uchar,uchar);
  static void set_boxtype(Fl_Boxtype, Fl_Boxtype from);
  static int box_dx(Fl_Boxtype);
  static int box_dy(Fl_Boxtype);
  static int box_dw(Fl_Boxtype);
  static int box_dh(Fl_Boxtype);
  static int draw_box_active();

  // back compatability:
  static void set_abort(void (*f)(const char*,...)) {fatal = f;}
  static void (*atclose)(Fl_Window*,void*);
  static void default_atclose(Fl_Window*,void*);
  static void set_atclose(void (*f)(Fl_Window*,void*)) {atclose = f;}
  static int event_shift() {return e_state&FL_SHIFT;}
  static int event_ctrl() {return e_state&FL_CTRL;}
  static int event_alt() {return e_state&FL_ALT;}
  static int event_buttons() {return e_state&0x7f000000;}
  static int event_button1() {return e_state&FL_BUTTON1;}
  static int event_button2() {return e_state&FL_BUTTON2;}
  static int event_button3() {return e_state&FL_BUTTON3;}
  static void set_idle(void (*cb)()) {idle = cb;}
  static void grab(Fl_Window&win) {grab(&win);}
  static void release() {grab(0);}

  // Visible focus methods...
  static void visible_focus(int v) { visible_focus_ = v; }
  static int  visible_focus() { return visible_focus_; }

  // Drag-n-drop text operation methods...
  static void dnd_text_ops(int v) { dnd_text_ops_ = v; }
  static int  dnd_text_ops() { return dnd_text_ops_; }

  // Multithreading support:
  static void lock();
  static void unlock();
  static void awake(void* message = 0);
  static int awake(Fl_Awake_Handler cb, void* message = 0);
  static void* thread_message();

  // Widget deletion:
  static void delete_widget(Fl_Widget *w);
  static void do_widget_deletion();
  static void watch_widget_pointer(Fl_Widget *&w);
  static void release_widget_pointer(Fl_Widget *&w);
  static void clear_widget_pointer(Fl_Widget const *w);
};

#endif // !Fl_H

//
// End of "$Id: Fl.H 5848 2007-05-20 16:18:31Z mike $".
//
