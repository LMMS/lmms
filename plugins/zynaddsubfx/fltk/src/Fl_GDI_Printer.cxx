//
// "$Id: Fl_GDI_Printer.cxx 8467 2011-02-23 14:36:18Z manolo $"
//
// Support for WIN32 printing for the Fast Light Tool Kit (FLTK).
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

#ifdef WIN32
#include <FL/Fl_Printer.H>


#include <FL/fl_ask.H>
#include <FL/math.h>

extern HWND fl_window;

Fl_System_Printer::Fl_System_Printer(void) : Fl_Paged_Device() {
  hPr = NULL;
  driver(Fl_Display_Device::display_device()->driver());
}

Fl_System_Printer::~Fl_System_Printer(void) {
  if (hPr) end_job();
}

static void WIN_SetupPrinterDeviceContext(HDC prHDC)
{
  if ( !prHDC ) return;
  
  fl_window = 0;
  fl_gc = prHDC;
  SetGraphicsMode(prHDC, GM_ADVANCED); // to allow for rotations
  SetMapMode(prHDC, MM_ANISOTROPIC);
  SetTextAlign(prHDC, TA_BASELINE|TA_LEFT);
  SetBkMode(prHDC, TRANSPARENT);	
  // this matches 720 logical units to the number of device units in 10 inches of paper
  // thus the logical unit is the point (= 1/72 inch)
  SetWindowExtEx(prHDC, 720, 720, NULL);
  SetViewportExtEx(prHDC, 10*GetDeviceCaps(prHDC, LOGPIXELSX), 10*GetDeviceCaps(prHDC, LOGPIXELSY), NULL);
}


int Fl_System_Printer::start_job (int pagecount, int *frompage, int *topage)
// returns 0 iff OK
{
  DWORD       commdlgerr;
  DOCINFO     di;
  char        docName [256];
  int err = 0;
  
  abortPrint = FALSE;
  memset (&pd, 0, sizeof (PRINTDLG));
  pd.lStructSize = sizeof (PRINTDLG);
  pd.hwndOwner = GetForegroundWindow();
  pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION;
  pd.nMinPage = 1;
  pd.nMaxPage = pagecount;
  if (PrintDlg (&pd) != 0) {
    hPr = pd.hDC;
    if (hPr != NULL) {
      strcpy (docName, "FLTK");
      memset(&di, 0, sizeof(DOCINFO));
      di.cbSize = sizeof (DOCINFO);
      di.lpszDocName = (LPCSTR) docName;
      prerr = StartDoc (hPr, &di);
      if (prerr < 1) {
	abortPrint = TRUE;
	//fl_alert ("StartDoc error %d", prerr);
	err = 1;
      }
    } else {
      commdlgerr = CommDlgExtendedError ();
      fl_alert ("Unable to create print context, error %lu",
		(unsigned long) commdlgerr);
      err = 1;
    }
  } else {
    err = 1;
  }
  if(!err) {
    if((pd.Flags & PD_PAGENUMS) != 0 ) {
      if (frompage) *frompage = pd.nFromPage;
      if (topage) *topage = pd.nToPage;
    }
    else {
      if (frompage) *frompage = 1;
      if (topage) *topage = pagecount;
    }
    x_offset = 0;
    y_offset = 0;
    WIN_SetupPrinterDeviceContext (hPr);
    gc = (void *)fl_gc;
    this->set_current();
  }
  return err;
}

void Fl_System_Printer::end_job (void)
{
  Fl_Display_Device::display_device()->set_current();
  if (hPr != NULL) {
    if (! abortPrint) {
      prerr = EndDoc (hPr);
      if (prerr < 0) {
	fl_alert ("EndDoc error %d", prerr);
      }
    }
    DeleteDC (hPr);
    if (pd.hDevMode != NULL) {
      GlobalFree (pd.hDevMode);
    }
    if (pd.hDevNames != NULL) {
      GlobalFree (pd.hDevNames);
    }
  }
  hPr = NULL;
}

void Fl_System_Printer::absolute_printable_rect(int *x, int *y, int *w, int *h)
{
  POINT         physPageSize;
  POINT         pixelsPerInch;
    
  if (hPr == NULL) return;
  SetWindowOrgEx(fl_gc, 0, 0, NULL);
  
  physPageSize.x = GetDeviceCaps(hPr, HORZRES);
  physPageSize.y = GetDeviceCaps(hPr, VERTRES);
  DPtoLP(hPr, &physPageSize, 1);
  *w = physPageSize.x + 1;
  *h = physPageSize.y + 1;
  pixelsPerInch.x = GetDeviceCaps(hPr, LOGPIXELSX);
  pixelsPerInch.y = GetDeviceCaps(hPr, LOGPIXELSY);
  DPtoLP(hPr, &pixelsPerInch, 1);
  left_margin = (pixelsPerInch.x / 4);
  *w -= (pixelsPerInch.x / 2);
  top_margin = (pixelsPerInch.y / 4);
  *h -= (pixelsPerInch.y / 2);
  
  *x = left_margin;
  *y = top_margin;
  origin(x_offset, y_offset);
}

void Fl_System_Printer::margins(int *left, int *top, int *right, int *bottom)
{
  int x, y, w, h;
  absolute_printable_rect(&x, &y, &w, &h);
  if (left) *left = x;
  if (top) *top = y;
  if (right) *right = x;
  if (bottom) *bottom = y;
}

int Fl_System_Printer::printable_rect(int *w, int *h)
{
  int x, y;
  absolute_printable_rect(&x, &y, w, h);
  return 0;
}

int Fl_System_Printer::start_page (void)
{
  int  rsult, w, h;
  
  rsult = 0;
  if (hPr != NULL) {
    WIN_SetupPrinterDeviceContext (hPr);
    prerr = StartPage (hPr);
    if (prerr < 0) {
      fl_alert ("StartPage error %d", prerr);
      rsult = 1;
    }
    printable_rect(&w, &h);
    origin(0, 0);
    fl_clip_region(0);
    gc = (void *)fl_gc;
  }
  return rsult;
}

void Fl_System_Printer::origin (int deltax, int deltay)
{
  SetWindowOrgEx(fl_gc, - left_margin - deltax, - top_margin - deltay, NULL);
  x_offset = deltax;
  y_offset = deltay;
}

void Fl_System_Printer::scale (float scalex, float scaley)
{
  if (scaley == 0.) scaley = scalex;
  int w, h;
  SetWindowExtEx(fl_gc, (int)(720 / scalex + 0.5), (int)(720 / scaley + 0.5), NULL);
  printable_rect(&w, &h);
  origin(0, 0);
}

void Fl_System_Printer::rotate (float rot_angle)
{
  XFORM mat;
  float angle;
  angle = - rot_angle * M_PI / 180.;
  mat.eM11 = cos(angle);
  mat.eM12 = sin(angle);
  mat.eM21 = - mat.eM12;
  mat.eM22 = mat.eM11;
  mat.eDx = mat.eDy = 0;
  SetWorldTransform(fl_gc, &mat);
}

int Fl_System_Printer::end_page (void)
{
  int  rsult;
  
  rsult = 0;
  if (hPr != NULL) {
    prerr = EndPage (hPr);
    if (prerr < 0) {
      abortPrint = TRUE;
      fl_alert ("EndPage error %d", prerr);
      rsult = 1;
    }
  }
  gc = NULL;
  return rsult;
}

static int translate_stack_depth = 0;
const int translate_stack_max = 5;
static int translate_stack_x[translate_stack_max];
static int translate_stack_y[translate_stack_max];

static void do_translate(int x, int y)
{
  XFORM tr;
  tr.eM11 = tr.eM22 = 1;
  tr.eM12 = tr.eM21 = 0;
  tr.eDx =  x;
  tr.eDy =  y;
  ModifyWorldTransform(fl_gc, &tr, MWT_LEFTMULTIPLY);
}

void Fl_System_Printer::translate (int x, int y)
{
  do_translate(x, y);
  if (translate_stack_depth < translate_stack_max) {
    translate_stack_x[translate_stack_depth] = x;
    translate_stack_y[translate_stack_depth] = y;
    translate_stack_depth++;
    }
}

void Fl_System_Printer::untranslate (void)
{
  if (translate_stack_depth > 0) {
    translate_stack_depth--;
    do_translate( - translate_stack_x[translate_stack_depth], - translate_stack_y[translate_stack_depth] );
    }
}

#endif // WIN32

//
// End of "$Id: Fl_GDI_Printer.cxx 8467 2011-02-23 14:36:18Z manolo $".
//
