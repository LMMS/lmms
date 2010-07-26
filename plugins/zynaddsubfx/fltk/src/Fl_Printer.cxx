//
// "$Id: Fl_Printer.cxx 7617 2010-05-27 17:20:18Z manolo $"
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
#include <src/Fl_Quartz_Printer.mm>
#elif defined(WIN32)
#include <src/Fl_GDI_Printer.cxx>
#endif

#include <src/Fl_PostScript.cxx>

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

const char *Fl_Printer::device_type = "Fl_Printer";

void Fl_Printer::set_current()
{
#ifdef __APPLE__
  fl_gc = (CGContextRef)gc;
#elif defined(WIN32)
  fl_gc = (HDC)gc;
#endif
  this->Fl_Surface_Device::set_current();
}

//
// End of "$Id: Fl_Printer.cxx 7617 2010-05-27 17:20:18Z manolo $".
//
