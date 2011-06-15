//
// "$Id: fl_dnd_win32.cxx 8028 2010-12-14 19:46:55Z AlbrechtS $"
//
// Drag & Drop code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2010 by Bill Spitzak and others.
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

// This file contains win32-specific code for fltk which is always linked
// in.  Search other files for "WIN32" or filenames ending in _win32.cxx
// for other system-specific code.

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include <FL/fl_utf8.h>
#include "flstring.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <objidl.h>
#include <time.h>
#if defined(__CYGWIN__)
#include <sys/time.h>
#include <unistd.h>
#endif

extern char *fl_selection_buffer[2];
extern int fl_selection_length[2];
extern int fl_selection_buffer_length[2];
extern char fl_i_own_selection[2];
extern char *fl_locale2utf8(const char *s, UINT codepage = 0);
extern unsigned int fl_codepage;

Fl_Window *fl_dnd_target_window = 0;

#include <ole2.h>
#include <shellapi.h>
#include <shlobj.h>


/**
 * subclass the IDropTarget to receive data from DnD operations
 */
class FLDropTarget : public IDropTarget
{
  DWORD m_cRefCount;
  DWORD lastEffect;
  int px, py;
public:
  FLDropTarget() : m_cRefCount(0) { } // initialize
  HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, LPVOID *ppvObject ) {
    if (IID_IUnknown==riid || IID_IDropTarget==riid)
    {
      *ppvObject=this;
      ((LPUNKNOWN)*ppvObject)->AddRef();
      return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() { return ++m_cRefCount; }
  ULONG STDMETHODCALLTYPE Release() {
    long nTemp;
    nTemp = --m_cRefCount;
    if(nTemp==0)
      delete this;
    return nTemp;
  }
  HRESULT STDMETHODCALLTYPE DragEnter( IDataObject *pDataObj, DWORD /*grfKeyState*/, POINTL pt, DWORD *pdwEffect) {
    if( !pDataObj ) return E_INVALIDARG;
    // set e_modifiers here from grfKeyState, set e_x and e_root_x
    // check if FLTK handles this drag and return if it can't (i.e. BMP drag without filename)
    POINT ppt;
    Fl::e_x_root = ppt.x = pt.x;
    Fl::e_y_root = ppt.y = pt.y;
    HWND hWnd = WindowFromPoint( ppt );
    Fl_Window *target = fl_find( hWnd );
    if (target) {
      Fl::e_x = Fl::e_x_root-target->x();
      Fl::e_y = Fl::e_y_root-target->y();
    }
    fl_dnd_target_window = target;
    px = pt.x; py = pt.y;
    if (fillCurrentDragData(pDataObj)) {
      // FLTK has no mechanism yet for the different drop effects, so we allow move and copy
      if ( target && Fl::handle( FL_DND_ENTER, target ) )
        *pdwEffect = DROPEFFECT_MOVE|DROPEFFECT_COPY; //|DROPEFFECT_LINK;
      else
        *pdwEffect = DROPEFFECT_NONE;
    } else {
      *pdwEffect = DROPEFFECT_NONE;
    }
    lastEffect = *pdwEffect;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE DragOver( DWORD /*grfKeyState*/, POINTL pt, DWORD *pdwEffect) {
    if ( px==pt.x && py==pt.y )
    {
      *pdwEffect = lastEffect;
      return S_OK;
    }
    if ( !fl_dnd_target_window )
    {
      *pdwEffect = lastEffect = DROPEFFECT_NONE;
      return S_OK;
    }
    // set e_modifiers here from grfKeyState, set e_x and e_root_x
    Fl::e_x_root = pt.x;
    Fl::e_y_root = pt.y;
    if (fl_dnd_target_window) {
      Fl::e_x = Fl::e_x_root-fl_dnd_target_window->x();
      Fl::e_y = Fl::e_y_root-fl_dnd_target_window->y();
    }
    if (fillCurrentDragData(0)) {
      // Fl_Group will change DND_DRAG into DND_ENTER and DND_LEAVE if needed
      if ( Fl::handle( FL_DND_DRAG, fl_dnd_target_window ) )
        *pdwEffect = DROPEFFECT_MOVE|DROPEFFECT_COPY; //|DROPEFFECT_LINK;
      else
        *pdwEffect = DROPEFFECT_NONE;
    } else {
      *pdwEffect = DROPEFFECT_NONE;
    }
    px = pt.x; py = pt.y;
    lastEffect = *pdwEffect;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE DragLeave() {
    if ( fl_dnd_target_window && fillCurrentDragData(0))
    {
      Fl::handle( FL_DND_LEAVE, fl_dnd_target_window );
      fl_dnd_target_window = 0;
      clearCurrentDragData();
    }
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Drop( IDataObject *data, DWORD /*grfKeyState*/, POINTL pt, DWORD* /*pdwEffect*/) {
    if ( !fl_dnd_target_window )
      return S_OK;
    Fl_Window *target = fl_dnd_target_window;
    fl_dnd_target_window = 0;
    Fl::e_x_root = pt.x;
    Fl::e_y_root = pt.y;
    if (target) {
      Fl::e_x = Fl::e_x_root-target->x();
      Fl::e_y = Fl::e_y_root-target->y();
    }
    // tell FLTK that the user released an object on this widget
    if ( !Fl::handle( FL_DND_RELEASE, target ) )
      return S_OK;

    Fl_Widget *w = target;
    while (w->parent()) w = w->window();
    HWND hwnd = fl_xid( (Fl_Window*)w );
    if (fillCurrentDragData(data)) {
      int old_event = Fl::e_number;
      char *a, *b;
      a = b = currDragData;
      while (*a) { // strip the CRLF pairs
	if (*a == '\r' && a[1] == '\n') a++;
	else *b++ = *a++;
      }
      *b = 0;
      Fl::e_text = currDragData;
      Fl::e_length = b - currDragData;
      Fl::belowmouse()->handle(Fl::e_number = FL_PASTE); // e_text will be invalid after this call
      Fl::e_number = old_event;
      SetForegroundWindow( hwnd );
      clearCurrentDragData();
      return S_OK;
    }
    return S_OK;
  }
private:

  static IDataObject *currDragRef;
  static char *currDragData;
  static int currDragSize;
  static char currDragResult;

  static void clearCurrentDragData() {
    currDragRef = 0;
    if (currDragData) free(currDragData);
    currDragData = 0;
    currDragSize = 0;
    currDragResult = 0;
  }
  static char fillCurrentDragData(IDataObject *data) {
    // shortcut through this whole procedure if there is no fresh data
    if (!data) 
      return currDragResult;
    // shortcut through this whole procedure if this is still the same drag event
    // (* this is safe, because 'currDragRef' is cleared on Leave and Drop events)
    if (data==currDragRef)
      return currDragResult;

    // clear currDrag* for a new drag event
    clearCurrentDragData();
    
    currDragRef = data;
    // fill currDrag* with UTF-8 data, if available
    FORMATETC fmt = { 0 };
    STGMEDIUM medium = { 0 };
    fmt.tymed = TYMED_HGLOBAL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.cfFormat = CF_UNICODETEXT;
    // if it is UNICODE text, return a UTF-8-converted copy of it
    if ( data->GetData( &fmt, &medium )==S_OK )
    {
      void *stuff = GlobalLock( medium.hGlobal );
      unsigned srclen = 0;
      const wchar_t *wstuff = (const wchar_t *)stuff;
      while (*wstuff++) srclen++;
      wstuff = (const wchar_t *)stuff;
      unsigned utf8len = fl_utf8fromwc(NULL, 0, wstuff, srclen);
      currDragSize = utf8len;
      currDragData = (char*)malloc(utf8len + 1);
      fl_utf8fromwc(currDragData, currDragSize+1, wstuff, srclen+1); // include null-byte
      GlobalUnlock( medium.hGlobal );
      ReleaseStgMedium( &medium );
      currDragResult = 1;
      return currDragResult;
    }
    fmt.cfFormat = CF_TEXT;
    // if it is CP1252 text, return a UTF-8-converted copy of it
    if ( data->GetData( &fmt, &medium )==S_OK )
    {
      int len;
      char *p, *q, *last;
      unsigned u;
      void *stuff = GlobalLock( medium.hGlobal );
      currDragData = (char*)malloc(3 * strlen((char*)stuff) + 10);
      p = (char*)stuff; 
      last = p + strlen(p);
      q = currDragData;
      while (p < last) {
	u = fl_utf8decode(p, last, &len);
	p += len;
	len = fl_utf8encode(u, q);
	q += len;
	}
      *q = 0;
      currDragSize = q - currDragData;
      currDragData = (char*)realloc(currDragData, currDragSize + 1);
      GlobalUnlock( medium.hGlobal );
      ReleaseStgMedium( &medium );
      currDragResult = 1;
      return currDragResult;
    }
    // else fill currDrag* with filenames, if possible
    memset(&fmt, 0, sizeof(fmt));
    fmt.tymed = TYMED_HGLOBAL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.cfFormat = CF_HDROP;
    // if it is a pathname list, send an FL_PASTE with a \n separated list of filepaths
    if ( data->GetData( &fmt, &medium )==S_OK )
    {
      HDROP hdrop = (HDROP)medium.hGlobal;
      int i, n, nn = 0, nf = DragQueryFileW( hdrop, (UINT)-1, 0, 0 );
        for ( i=0; i<nf; i++ ) nn += DragQueryFileW( hdrop, i, 0, 0 );
      nn += nf;
        xchar *dst = (xchar *)malloc(nn * sizeof(xchar));
        xchar *bu = dst;
      for ( i=0; i<nf; i++ ) {
          n = DragQueryFileW( hdrop, i, (WCHAR*)dst, nn );
          dst += n;
          if ( i<nf-1 ) {
            *dst++ = L'\n';
          }
        }
         *dst=0;

        currDragData = (char*) malloc(nn * 5 + 1);
//      Fl::e_length = fl_unicode2utf(bu, nn, Fl::e_text);
        currDragSize = fl_utf8fromwc(currDragData, (nn*5+1), bu, nn);
        currDragData[currDragSize] = 0;
        free(bu);

//    Fl::belowmouse()->handle(FL_DROP);
//      free( Fl::e_text );
      ReleaseStgMedium( &medium );
      currDragResult = 1;
      return currDragResult;
    }
    currDragResult = 0;
    return currDragResult;
  }
} flDropTarget;

IDropTarget *flIDropTarget = &flDropTarget;

IDataObject *FLDropTarget::currDragRef = 0;
char *FLDropTarget::currDragData = 0;
int FLDropTarget::currDragSize = 0;
char FLDropTarget::currDragResult = 0;

/**
 * this class is needed to allow FLTK apps to be a DnD source
 */
class FLDropSource : public IDropSource
{
  DWORD m_cRefCount;
public:
  FLDropSource() { m_cRefCount = 0; }
  HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, LPVOID *ppvObject ) {
    if (IID_IUnknown==riid || IID_IDropSource==riid)
    {
      *ppvObject=this;
      ((LPUNKNOWN)*ppvObject)->AddRef();
      return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() { return ++m_cRefCount; }
  ULONG STDMETHODCALLTYPE Release() {
    long nTemp;
    nTemp = --m_cRefCount;
    if(nTemp==0)
      delete this;
    return nTemp;
  }
  STDMETHODIMP GiveFeedback( ulong ) { return DRAGDROP_S_USEDEFAULTCURSORS; }
  STDMETHODIMP QueryContinueDrag( BOOL esc, DWORD keyState ) {
    if ( esc )
      return DRAGDROP_S_CANCEL;
    if ( !(keyState & (MK_LBUTTON|MK_MBUTTON|MK_RBUTTON)) )
      return DRAGDROP_S_DROP;
    return S_OK;
  }
};
class FLEnum : public IEnumFORMATETC
{
public:
  int n;
  LONG m_lRefCount;

  ULONG __stdcall AddRef(void) {
    return InterlockedIncrement(&m_lRefCount);
  }

  ULONG __stdcall Release(void) {
    LONG count = InterlockedDecrement(&m_lRefCount);
    if(count == 0) {
      delete this;
      return 0;
    } else {
      return count;
    }
  }


  HRESULT __stdcall QueryInterface(REFIID iid, void **ppvObject) {
    if(iid == IID_IEnumFORMATETC || iid == IID_IUnknown) {
       AddRef();
       *ppvObject = this;
       return S_OK;
    } else {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
  }

  HRESULT __stdcall Next(ULONG celt, FORMATETC * rgelt, ULONG *pceltFetched) {
    if (n > 0) return S_FALSE;
    for (ULONG i = 0; i < celt; i++) {
      n++;
      rgelt->cfFormat = CF_HDROP;
      rgelt->dwAspect = DVASPECT_CONTENT;
      rgelt->lindex = -1;
      rgelt->ptd = NULL;
      rgelt->tymed = TYMED_HGLOBAL;
    }
    if (pceltFetched) *pceltFetched = celt;
    return S_OK;
  }

  HRESULT __stdcall Skip(ULONG celt) {
    n += celt;
    return  (n == 0) ? S_OK : S_FALSE;
  }

  HRESULT __stdcall Reset(void) {
	n = 0;
	return S_OK;
  }

  HRESULT __stdcall Clone(IEnumFORMATETC  **ppenum){
    *ppenum = new FLEnum();
    return S_OK;
  }

  FLEnum(void) {
    m_lRefCount   = 1;
    n = 0;
  }

  virtual ~FLEnum(void) {
    n = 0;
  }
};


/**
  This is the actual object that FLTK can drop somewhere.

  The implementation is minimal, but it should work with all decent Win32 drop targets
*/
class FLDataObject : public IDataObject
{
  DWORD m_cRefCount;
  FLEnum *m_EnumF;
public:
  FLDataObject() { m_cRefCount = 1; }// m_EnumF = new FLEnum();}
  HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, LPVOID *ppvObject ) {
    if (IID_IUnknown==riid || IID_IDataObject==riid)
    {
      *ppvObject=this;
      ((LPUNKNOWN)*ppvObject)->AddRef();
      return S_OK;
    }
    *ppvObject = NULL;
    return E_NOINTERFACE;
  }
  ULONG STDMETHODCALLTYPE AddRef() { return ++m_cRefCount; }
  ULONG STDMETHODCALLTYPE Release() {
    long nTemp;
    nTemp = --m_cRefCount;
    if(nTemp==0)
      delete this;
    return nTemp;
  }
  // GetData currently allows UNICODE text through Global Memory only
  HRESULT STDMETHODCALLTYPE GetData( FORMATETC *pformatetcIn, STGMEDIUM *pmedium ) {
    if ((pformatetcIn->dwAspect & DVASPECT_CONTENT) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL) &&
        (pformatetcIn->cfFormat == CF_UNICODETEXT))
    {
      int utf16_len = fl_utf8toUtf16(fl_selection_buffer[0], fl_selection_length[0], 0, 0);
      HGLOBAL gh = GlobalAlloc( GHND, utf16_len * 2 + 2 );
      char *pMem = (char*)GlobalLock( gh );
      fl_utf8toUtf16(fl_selection_buffer[0], fl_selection_length[0], (unsigned short*)pMem, utf16_len + 1);
//      HGLOBAL gh = GlobalAlloc( GHND| GMEM_SHARE,
//                            (fl_selection_length[0]+4) * sizeof(short)
//                            + sizeof(DROPFILES));
//      unsigned char *pMem = (unsigned char*)GlobalLock( gh );
//      if (!pMem) {
//        GlobalFree(gh);
//        return DV_E_FORMATETC;
//      }
//      DROPFILES *df =(DROPFILES*) pMem;
//      int l;
//      df->pFiles = sizeof(DROPFILES);
//      df->pt.x = 0;
//      df->pt.y = 0;
//      df->fNC = FALSE;
//      for (int i = 0; i < fl_selection_length[0]; i++) {
//        if (fl_selection_buffer[0][i] == '\n') {
//          fl_selection_buffer[0][i] = '\0';
//        }
//      }
//
//        df->fWide = TRUE;
//        l = fl_utf2unicode((unsigned char*)fl_selection_buffer[0],
//                             fl_selection_length[0], (xchar*)(((char*)pMem)
//                              + sizeof(DROPFILES)));
//
//      pMem[l * sizeof(WCHAR) + sizeof(DROPFILES)] = 0;
//      pMem[l * sizeof(WCHAR) + 1 + sizeof(DROPFILES)] = 0;
//      pMem[l * sizeof(WCHAR) + 2 + sizeof(DROPFILES)] = 0;
//      pMem[l * sizeof(WCHAR) + 3 + sizeof(DROPFILES)] = 0;
      pmedium->tymed	      = TYMED_HGLOBAL;
      pmedium->hGlobal	      = gh;
      pmedium->pUnkForRelease = NULL;
      GlobalUnlock( gh );
      return S_OK;
    }
    return DV_E_FORMATETC;
  }
  HRESULT STDMETHODCALLTYPE QueryGetData( FORMATETC *pformatetc )
  {
    if ((pformatetc->dwAspect & DVASPECT_CONTENT) &&
        (pformatetc->tymed & TYMED_HGLOBAL) &&
        (pformatetc->cfFormat == CF_UNICODETEXT))
      return S_OK;
    return DV_E_FORMATETC;
  }
//  HRESULT STDMETHODCALLTYPE EnumFormatEtc( DWORD dir, IEnumFORMATETC** ppenumFormatEtc) {
//	*ppenumFormatEtc = m_EnumF;
//	return S_OK;
//  }

  // all the following methods are not really needed for a DnD object
  HRESULT STDMETHODCALLTYPE GetDataHere( FORMATETC* /*pformatetcIn*/, STGMEDIUM* /*pmedium*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc( FORMATETC* /*in*/, FORMATETC* /*out*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE SetData( FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/, BOOL /*fRelease*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE EnumFormatEtc( DWORD /*dir*/, IEnumFORMATETC** /*ppenumFormatEtc*/) { return E_NOTIMPL; }
//  HRESULT STDMETHODCALLTYPE EnumFormatEtc( DWORD dir, IEnumFORMATETC** ppenumFormatEtc) {*ppenumFormatEtc = m_EnumF; return S_OK;}
  HRESULT STDMETHODCALLTYPE DAdvise( FORMATETC* /*pformatetc*/, DWORD /*advf*/,
      IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE DUnadvise( DWORD /*dwConnection*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE EnumDAdvise( IEnumSTATDATA** /*ppenumAdvise*/) { return E_NOTIMPL; }
};


int Fl::dnd()
{
  DWORD dropEffect;
  ReleaseCapture();

  FLDataObject *fdo = new FLDataObject;
  fdo->AddRef();
  FLDropSource *fds = new FLDropSource;
  fds->AddRef();

  HRESULT ret = DoDragDrop( fdo, fds, DROPEFFECT_MOVE|DROPEFFECT_LINK|DROPEFFECT_COPY, &dropEffect );

  fdo->Release();
  fds->Release();

  Fl_Widget *w = Fl::pushed();
  if ( w )
  {
    int old_event = Fl::e_number;
    w->handle(Fl::e_number = FL_RELEASE);
    Fl::e_number = old_event;
    Fl::pushed( 0 );
  }
  if ( ret==DRAGDROP_S_DROP ) return 1; // or DD_S_CANCEL
  return 0;
}

//
// End of "$Id: fl_dnd_win32.cxx 8028 2010-12-14 19:46:55Z AlbrechtS $".
//
