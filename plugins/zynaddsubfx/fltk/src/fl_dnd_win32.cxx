//
// "$Id: fl_dnd_win32.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Drag & Drop code for the Fast Light Tool Kit (FLTK).
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
// Please report all bugs and problems to "fltk-bugs@fltk.org

// This file contains win32-specific code for fltk which is always linked
// in.  Search other files for "WIN32" or filenames ending in _win32.cxx
// for other system-specific code.

#include <FL/Fl.H>
#include <FL/x.H>
#include <FL/Fl_Window.H>
#include "flstring.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#if defined(__CYGWIN__)
#include <sys/time.h>
#include <unistd.h>
#else
#include <winsock.h>
#endif

extern char *fl_selection_buffer[2];
extern int fl_selection_length[2];
extern int fl_selection_buffer_length[2];
extern char fl_i_own_selection[2];

Fl_Window *fl_dnd_target_window = 0;

// All of the following code requires GCC 3.x or a non-GNU compiler...
#if !defined(__GNUC__) || __GNUC__ >= 3

#include <ole2.h>
#include <shellapi.h>

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

    // fill currDrag* with ASCII data, if available
    FORMATETC fmt = { 0 };
    STGMEDIUM medium = { 0 };
    fmt.tymed = TYMED_HGLOBAL;
    fmt.dwAspect = DVASPECT_CONTENT;
    fmt.lindex = -1;
    fmt.cfFormat = CF_TEXT;
    // if it is ASCII text, return a copy of it
    if ( data->GetData( &fmt, &medium )==S_OK )
    {
      void *stuff = GlobalLock( medium.hGlobal );
      Fl::e_length = strlen((char*)stuff);
      Fl::e_text = strdup((char*)stuff);
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
    // if it is a pathname list, send an FL_PASTE with a \n seperated list of filepaths
    if ( data->GetData( &fmt, &medium )==S_OK )
    {
      HDROP hdrop = (HDROP)medium.hGlobal;
      int i, n, nn = 0, nf = DragQueryFile( hdrop, (UINT)-1, 0, 0 );
      for ( i=0; i<nf; i++ ) nn += DragQueryFile( hdrop, i, 0, 0 );
      nn += nf;
      Fl::e_length = nn-1;
      char *dst = Fl::e_text = (char*)malloc(nn+1);
      for ( i=0; i<nf; i++ ) {
	n = DragQueryFile( hdrop, i, dst, nn );
	dst += n;
	if ( i<nf-1 ) *dst++ = '\n';
      }
      *dst = 0;
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

/**
 * this is the actual object that FLTK can drop somewhere
 * - the implementation is minimal, but it should work with all decent Win32 drop targets
 */
class FLDataObject : public IDataObject
{
  DWORD m_cRefCount;
public:
  FLDataObject() { m_cRefCount = 1; }
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
  // GetData currently allows ASCII text through Global Memory only
  HRESULT STDMETHODCALLTYPE GetData( FORMATETC *pformatetcIn, STGMEDIUM *pmedium ) {
    if ((pformatetcIn->dwAspect & DVASPECT_CONTENT) &&
        (pformatetcIn->tymed & TYMED_HGLOBAL) &&
        (pformatetcIn->cfFormat == CF_TEXT))
    {
      HGLOBAL gh = GlobalAlloc( GHND, fl_selection_length[0]+1 );
      char *pMem = (char*)GlobalLock( gh );
      memmove( pMem, fl_selection_buffer[0], fl_selection_length[0] );
      pMem[ fl_selection_length[0] ] = 0;      
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
        (pformatetc->cfFormat == CF_TEXT))
      return S_OK;
    return DV_E_FORMATETC;	
  }  
  // all the following methods are not really needed for a DnD object
  HRESULT STDMETHODCALLTYPE GetDataHere( FORMATETC* /*pformatetcIn*/, STGMEDIUM* /*pmedium*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc( FORMATETC* /*in*/, FORMATETC* /*out*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE SetData( FORMATETC* /*pformatetc*/, STGMEDIUM* /*pmedium*/, BOOL /*fRelease*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE EnumFormatEtc( DWORD /*dir*/, IEnumFORMATETC** /*ppenumFormatEtc*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE DAdvise( FORMATETC* /*pformatetc*/, DWORD /*advf*/,
      IAdviseSink* /*pAdvSink*/, DWORD* /*pdwConnection*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE DUnadvise( DWORD /*dwConnection*/) { return E_NOTIMPL; }
  HRESULT STDMETHODCALLTYPE EnumDAdvise( IEnumSTATDATA** /*ppenumAdvise*/) { return E_NOTIMPL; }
};


/**
 * drag and drop whatever is in the cut-copy-paste buffer
 * - create a selection first using: 
 *     Fl::copy(const char *stuff, int len, 0)
 */
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
#else
int Fl::dnd()
{
  // Always indicate DnD failed when using GCC < 3...
  return 1;
}
#endif // !__GNUC__ || __GNUC__ >= 3


//
// End of "$Id: fl_dnd_win32.cxx 5190 2006-06-09 16:16:34Z mike $".
//
