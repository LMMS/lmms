//
// "$Id: Fl_Paged_Device.cxx 8621 2011-04-23 15:46:30Z AlbrechtS $"
//
// implementation of Fl_Paged_Device class for the Fast Light Tool Kit (FLTK).
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
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//
/** \file Fl_Paged_Device.cxx
 \brief implementation of class Fl_Paged_Device.
 */

#include <FL/Fl_Paged_Device.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>

const char *Fl_Paged_Device::class_id = "Fl_Paged_Device";


/**
 @brief Draws the widget on the printed page.
 *
 The widget's position on the printed page is determined by the last call to origin()
 and by the optional delta_x and delta_y arguments.
 Its dimensions are in points unless there was a previous call to scale().
 @param[in] widget Any FLTK widget (e.g., standard, custom, window).
 @param[in] delta_x Optional horizontal offset for positioning the widget relatively
 to the current origin of graphics functions.
 @param[in] delta_y Same as above, vertically.
 */
void Fl_Paged_Device::print_widget(Fl_Widget* widget, int delta_x, int delta_y) 
{ 
  int old_x, old_y, new_x, new_y, is_window;
  if ( ! widget->visible() ) return;
  is_window = (widget->as_window() != NULL);
  widget->damage(FL_DAMAGE_ALL);
  // set origin to the desired top-left position of the widget
  origin(&old_x, &old_y);
  new_x = old_x + delta_x;
  new_y = old_y + delta_y;
  if (!is_window) {
    new_x -= widget->x();
    new_y -= widget->y();
  }
  if (new_x != old_x || new_y != old_y) {
    translate(new_x - old_x, new_y - old_y );
  }
  // if widget is a window, clip all drawings to the window area
  if (is_window) fl_push_clip(0, 0, widget->w(), widget->h() );
  // we do some trickery to recognize OpenGL windows and draw them via a plugin
  int drawn_by_plugin = 0;
  if (widget->as_gl_window()) {
    Fl_Plugin_Manager pm("fltk:device");  
    Fl_Device_Plugin *pi = (Fl_Device_Plugin*)pm.plugin("opengl.device.fltk.org");
    if (pi) {
      int width, height;
      this->printable_rect(&width, &height);
      drawn_by_plugin = pi->print(widget, 0, 0, height);
    }
  }
  if (!drawn_by_plugin) {
    widget->draw();
  }
  if (is_window) fl_pop_clip();
  // find subwindows of widget and print them
  traverse(widget);
  // reset origin to where it was
  if (new_x != old_x || new_y != old_y) {
    untranslate();
  }
}


void Fl_Paged_Device::traverse(Fl_Widget *widget)
{
  Fl_Group *g = widget->as_group();
  if (!g) return;
  int n = g->children();
  for (int i = 0; i < n; i++) {
    Fl_Widget *c = g->child(i);
    if ( !c->visible() ) continue;
    if ( c->as_window() ) {
      print_widget(c, c->x(), c->y());
    }
    else traverse(c);
  }
}

/**
 @brief Computes the page coordinates of the current origin of graphics functions.
 *
 @param[out] x If non-null, *x is set to the horizontal page offset of graphics origin.
 @param[out] y Same as above, vertically.
 */
void Fl_Paged_Device::origin(int *x, int *y)
{
  if (x) *x = x_offset;
  if (y) *y = y_offset;
}

/**
 @brief Prints a rectangular part of an on-screen window.

 @param win The window from where to capture.
 @param x The rectangle left
 @param y The rectangle top
 @param w The rectangle width
 @param h The rectangle height
 @param delta_x Optional horizontal offset from current graphics origin where to print the captured rectangle.
 @param delta_y As above, vertically.
 */
void Fl_Paged_Device::print_window_part(Fl_Window *win, int x, int y, int w, int h, int delta_x, int delta_y)
{
  Fl_Surface_Device *current = Fl_Surface_Device::surface();
  Fl_Display_Device::display_device()->set_current();
  Fl_Window *save_front = Fl::first_window();
  win->show();
  fl_gc = NULL;
  Fl::check();
  win->make_current();
  uchar *image_data;
  image_data = fl_read_image(NULL, x, y, w, h);
  if (save_front != win) save_front->show();
  current->set_current();
  fl_draw_image(image_data, delta_x, delta_y, w, h, 3);
  delete[] image_data;
#ifdef WIN32
  fl_gc = GetDC(fl_xid(win));
  ReleaseDC(fl_xid(win), fl_gc);
#endif
}

/**
 @brief Starts a print job.

 @param[in] pagecount the total number of pages of the job
 @param[out] frompage if non-null, *frompage is set to the first page the user wants printed
 @param[out] topage if non-null, *topage is set to the last page the user wants printed
 @return 0 if OK, non-zero if any error
 */
int Fl_Paged_Device::start_job(int pagecount, int *frompage, int *topage) {return 1;}

/**
 @brief Starts a new printed page

 The page coordinates are initially in points, i.e., 1/72 inch, 
 and with origin at the top left of the printable page area.
 @return 0 if OK, non-zero if any error
 */
int Fl_Paged_Device::start_page (void) {return 1;}

/**
 @brief Computes the width and height of the printable area of the page.

 Values are in the same unit as that used by FLTK drawing functions,
 are unchanged by calls to origin(), but are changed by scale() calls.
 Values account for the user-selected paper type and print orientation.
 @return 0 if OK, non-zero if any error
 */
int Fl_Paged_Device::printable_rect(int *w, int *h) {return 1;}

/**
 @brief Computes the dimensions of margins that lie between the printable page area and
 the full page.

 Values are in the same unit as that used by FLTK drawing functions. They are changed
 by scale() calls.
 @param[out] left If non-null, *left is set to the left margin size.
 @param[out] top If non-null, *top is set to the top margin size.
 @param[out] right If non-null, *right is set to the right margin size.
 @param[out] bottom If non-null, *bottom is set to the bottom margin size.
 */
void Fl_Paged_Device::margins(int *left, int *top, int *right, int *bottom) {}

/**
 @brief Sets the position in page coordinates of the origin of graphics functions.

 Arguments should be expressed relatively to the result of a previous printable_rect() call.
 That is, <tt>printable_rect(&w, &h); origin(w/2, 0);</tt> sets the graphics origin at the
 top center of the page printable area.
 Origin() calls are not affected by rotate() calls.
 Successive origin() calls don't combine their effects.
 @param[in] x Horizontal position in page coordinates of the desired origin of graphics functions.
 @param[in] y Same as above, vertically.
 */
void Fl_Paged_Device::origin(int x, int y) {}

/**
 @brief Changes the scaling of page coordinates.

 This function also resets the origin of graphics functions at top left of printable page area.
 After a scale() call, do a printable_rect() call to get the new dimensions of the printable page area.
 Successive scale() calls don't combine their effects.
 @param scale_x Horizontal dimensions of plot are multiplied by this quantity.
 @param scale_y Same as above, vertically. 
  The value 0. is equivalent to setting \p scale_y = \p scale_x. Thus, scale(factor);
  is equivalent to scale(factor, factor);
 */
void Fl_Paged_Device::scale (float scale_x, float scale_y) {}

/**
 @brief Rotates the graphics operations relatively to paper.

 The rotation is centered on the current graphics origin. 
 Successive rotate() calls don't combine their effects.
 @param angle Rotation angle in counter-clockwise degrees.
 */
void Fl_Paged_Device::rotate(float angle) {}

/**
 @brief To be called at the end of each page.

 @return 0 if OK, non-zero if any error.
 */
int Fl_Paged_Device::end_page (void) {return 1;}

/**
 @brief To be called at the end of a print job.
 */
void Fl_Paged_Device::end_job (void) {}

/**
 @brief Translates the current graphics origin accounting for the current rotation.

 This function is only useful after a rotate() call. 
 Each translate() call must be matched by an untranslate() call.
 Successive translate() calls add up their effects.
 */
void Fl_Paged_Device::translate(int x, int y) {}

/**
 @brief Undoes the effect of a previous translate() call.
 */
void Fl_Paged_Device::untranslate(void) {}

const Fl_Paged_Device::page_format Fl_Paged_Device::page_formats[NO_PAGE_FORMATS] = { 
  // order of enum Page_Format
  // comes from appendix B of 5003.PPD_Spec_v4.3.pdf
  
  // A* // index(Ai) = i
  {2384, 3370, "A0"},
  {1684, 2384, "A1"},
  {1191, 1684, "A2"},
  { 842, 1191, "A3"},
  { 595,  842, "A4"},
  { 420,  595, "A5"},
  { 297,  420, "A6"},
  { 210,  297, "A7"},
  { 148,  210, "A8"},
  { 105,  148, "A9"},
  
  // B* // index(Bi) = i+10
  {2920, 4127, "B0"},
  {2064, 2920, "B1"},
  {1460, 2064, "B2"},
  {1032, 1460, "B3"},
  { 729, 1032, "B4"},
  { 516,  729, "B5"},
  { 363,  516, "B6"},
  { 258,  363, "B7"},
  { 181,  258, "B8"},
  { 127,  181, "B9"},
  {  91,  127, "B10"},
  
  // others
  { 459,  649, "EnvC5"}, // envelope
  { 312,  624, "EnvDL"}, // envelope
  { 522,  756, "Executive"},
  { 595,  935, "Folio"},
  {1224,  792, "Ledger"}, // landscape
  { 612, 1008, "Legal"},
  { 612,  792, "Letter"},
  { 792, 1224, "Tabloid"},
  { 297,  684, "Env10"} // envelope
};

//
// End of "$Id: Fl_Paged_Device.cxx 8621 2011-04-23 15:46:30Z AlbrechtS $".
//
