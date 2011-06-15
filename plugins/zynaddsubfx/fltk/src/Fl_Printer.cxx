//
// "$Id: Fl_Printer.cxx 8565 2011-04-06 13:43:09Z manolo $"
//
// Encompasses platform-specific printing-support code and 
// PostScript output code for the Fast Light Tool Kit (FLTK).
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
// Please report all bugs and problems to:
//
//     http://www.fltk.org/str.php
//

#include <FL/Fl_Printer.H>

#ifdef __APPLE__
//#include "Fl_Quartz_Printer.mm"
#elif defined(WIN32)
#include "Fl_GDI_Printer.cxx"
#endif

#include "Fl_PostScript.cxx"

// print dialog customization strings
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_title = "Print";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_printer = "Printer:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_range = "Print Range";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_copies = "Copies";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_all = "All";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_pages = "Pages";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_from = "From:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_to = "To:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_properties = "Properties...";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_copyNo = "# Copies:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_print_button = "Print";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_cancel_button = "Cancel";
/** [this text may be customized at run-time] */
const char *Fl_Printer::dialog_print_to_file = "Print To File";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_title = "Printer Properties";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_pagesize = "Page Size:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_mode = "Output Mode:";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_use = "Use";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_save = "Save";
/** [this text may be customized at run-time] */
const char *Fl_Printer::property_cancel = "Cancel";

const char *Fl_Printer::class_id = "Fl_Printer";
#if defined(__APPLE__) || defined(WIN32) || defined(FL_DOXYGEN)
const char *Fl_System_Printer::class_id = Fl_Printer::class_id;
#endif
#if !( defined(__APPLE__) || defined(WIN32) )
const char *Fl_PostScript_Printer::class_id = Fl_Printer::class_id;
#endif

#if defined(__APPLE__) || defined(WIN32)
void Fl_System_Printer::set_current(void)
{
#ifdef __APPLE__
  fl_gc = (CGContextRef)gc;
#elif defined(WIN32)
  fl_gc = (HDC)gc;
#endif
  this->Fl_Surface_Device::set_current();
}

void Fl_System_Printer::origin(int *x, int *y)
{
  Fl_Paged_Device::origin(x, y);
}

#endif

Fl_Printer::Fl_Printer(void) {
#if defined(WIN32) || defined(__APPLE__)
  printer = new Fl_System_Printer();
#else
  printer = new Fl_PostScript_Printer();
#endif
}

int Fl_Printer::start_job(int pagecount, int *frompage, int *topage)
{
  return printer->start_job(pagecount, frompage, topage);
}

int Fl_Printer::start_page(void)
{
  return printer->start_page();
}

int Fl_Printer::printable_rect(int *w, int *h)
{
  return printer->printable_rect(w, h);
}

void Fl_Printer::margins(int *left, int *top, int *right, int *bottom)
{
  printer->margins(left, top, right, bottom);
}

void Fl_Printer::origin(int *x, int *y)
{
  printer->origin(x, y);
}

void Fl_Printer::origin(int x, int y)
{
  printer->origin(x, y);
}

void Fl_Printer::scale(float scale_x, float scale_y)
{
  printer->scale(scale_x, scale_y);
}

void Fl_Printer::rotate(float angle)
{
  printer->rotate(angle);
}

void Fl_Printer::translate(int x, int y)
{
  printer->translate(x, y);
}

void Fl_Printer::untranslate(void)
{
  printer->untranslate();
}

int Fl_Printer::end_page (void)
{
  return printer->end_page();
}

void Fl_Printer::end_job (void)
{
  printer->end_job();
}

void Fl_Printer::print_widget(Fl_Widget* widget, int delta_x, int delta_y)
{
  printer->print_widget(widget, delta_x, delta_y);
}

void Fl_Printer::print_window_part(Fl_Window *win, int x, int y, int w, int h, int delta_x, int delta_y)
{
  printer->print_window_part(win, x, y, w, h, delta_x, delta_y);
}

void Fl_Printer::set_current(void)
{
  printer->set_current();
}

Fl_Graphics_Driver* Fl_Printer::driver(void)
{
  return printer->driver();
}

Fl_Printer::~Fl_Printer(void)
{
  delete printer;
}


//
// End of "$Id: Fl_Printer.cxx 8565 2011-04-06 13:43:09Z manolo $".
//
