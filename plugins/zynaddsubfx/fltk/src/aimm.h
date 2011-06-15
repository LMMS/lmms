//
// "$Id: aimm.h 7913 2010-11-29 18:18:27Z greg.ercolano $"
//
// Standard dialog header file for the UTF-8 Fast Light Tool Kit (FLTK-UTF8).
//
// Copyright 2009-2010 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version
// with exceptions that allow sub-classing and static linking in
// non-LGPL compliant software. These exceptions are subject to
// conditions, see the FLTK License for more details.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the FLTK
// License for more details.
//
// You should have received a copy of the FLTK License along with
// this library; if not, write to  OksiD Software, Jean-Marc Lienher,
// Rue de la Cheminee 1, CH-2065 Savagnier, Switzerland.
//
// Please report all bugs and problems to "oksid@bluewin.ch".
//

#ifndef AIMM_H
#  define AIMM_H
//#  define HANDLE_PTR HANDLE* 
//#  define DWORD_PTR DWORD*
//#  define CLSCTX_INPROC_SERVER 0x1
const GUID IID_IActiveIMMApp = { 0x8c0e040, 0x62d1, 0x11d1, {0x93, 0x26, 0x00, 0x60, 0xb0, 0x67, 0xb8, 0x6e}};
const GUID CLSID_CActiveIMM = { 0x4955dd33, 0xb159, 0x11d0, {0x8f, 0xcf, 0x00, 0xaa, 0x00, 0x6b, 0xcc, 0x59}};
/*
	class IUnknown
    {
    public:
        
        virtual long __stdcall QueryInterface( 
            const GUID & riid,
            void **ppvObject) = 0;
        
        virtual ULONG __stdcall AddRef( void) = 0;
        
        virtual ULONG __stdcall Release( void) = 0;
    };

extern "C" __declspec(dllimport) long __stdcall CoInitialize(void far *pvReserved);
extern "C" __declspec(dllimport) long __stdcall CoCreateInstance(const GUID & rclsid, IUnknown * pUnkOuter,
                    DWORD dwClsContext, const GUID & riid, LPVOID FAR* ppv);

*/

    class IActiveIMMApp : public IUnknown
    {
    public:
        virtual long __stdcall AssociateContext( 
             HWND hWnd,
             HIMC hIME,
             HIMC  *phPrev) = 0;
        
        virtual long __stdcall ConfigureIMEA( 
             HKL hKL,
             HWND hWnd,
             DWORD dwMode,
             void  *pData) = 0;
        
        virtual long __stdcall ConfigureIMEW( 
             HKL hKL,
             HWND hWnd,
             DWORD dwMode,
             void  *pData) = 0;
        
        virtual long __stdcall CreateContext( 
             HIMC  *phIMC) = 0;
        
        virtual long __stdcall DestroyContext( 
             HIMC hIME) = 0;
        
        virtual long __stdcall EnumRegisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szRegister,
             LPVOID pData,
             void  **pEnum) = 0;
        
        virtual long __stdcall EnumRegisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szRegister,
             LPVOID pData,
             void  **pEnum) = 0;
        
        virtual long __stdcall EscapeA( 
             HKL hKL,
             HIMC hIMC,
             UINT uEscape,
            /* [out][in] */ LPVOID pData,
             LRESULT  *plResult) = 0;
        
        virtual long __stdcall EscapeW( 
             HKL hKL,
             HIMC hIMC,
             UINT uEscape,
            /* [out][in] */ LPVOID pData,
             LRESULT  *plResult) = 0;
        
        virtual long __stdcall GetCandidateListA( 
             HIMC hIMC,
             DWORD dwIndex,
             UINT uBufLen,
             void  *pCandList,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetCandidateListW( 
             HIMC hIMC,
             DWORD dwIndex,
             UINT uBufLen,
             void  *pCandList,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetCandidateListCountA( 
             HIMC hIMC,
             DWORD  *pdwListSize,
             DWORD  *pdwBufLen) = 0;
        
        virtual long __stdcall GetCandidateListCountW( 
             HIMC hIMC,
             DWORD  *pdwListSize,
             DWORD  *pdwBufLen) = 0;
        
        virtual long __stdcall GetCandidateWindow( 
             HIMC hIMC,
             DWORD dwIndex,
             void  *pCandidate) = 0;
        
        virtual long __stdcall GetCompositionFontA( 
             HIMC hIMC,
             LOGFONTA  *plf) = 0;
        
        virtual long __stdcall GetCompositionFontW( 
             HIMC hIMC,
             LOGFONTW  *plf) = 0;
        
        virtual long __stdcall GetCompositionStringA( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LONG  *plCopied,
             LPVOID pBuf) = 0;
        
        virtual long __stdcall GetCompositionStringW( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LONG  *plCopied,
             LPVOID pBuf) = 0;
        
        virtual long __stdcall GetCompositionWindow( 
             HIMC hIMC,
             void  *pCompForm) = 0;
        
        virtual long __stdcall GetContext( 
             HWND hWnd,
             HIMC  *phIMC) = 0;
        
        virtual long __stdcall GetConversionListA( 
             HKL hKL,
             HIMC hIMC,
             LPSTR pSrc,
             UINT uBufLen,
             UINT uFlag,
             void  *pDst,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetConversionListW( 
             HKL hKL,
             HIMC hIMC,
             LPWSTR pSrc,
             UINT uBufLen,
             UINT uFlag,
             void  *pDst,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetConversionStatus( 
             HIMC hIMC,
             DWORD  *pfdwConversion,
             DWORD  *pfdwSentence) = 0;
        
        virtual long __stdcall GetDefaultIMEWnd( 
             HWND hWnd,
             HWND  *phDefWnd) = 0;
        
        virtual long __stdcall GetDescriptionA( 
             HKL hKL,
             UINT uBufLen,
             LPSTR szDescription,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetDescriptionW( 
             HKL hKL,
             UINT uBufLen,
             LPWSTR szDescription,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetGuideLineA( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LPSTR pBuf,
             DWORD  *pdwResult) = 0;
        
        virtual long __stdcall GetGuideLineW( 
             HIMC hIMC,
             DWORD dwIndex,
             DWORD dwBufLen,
             LPWSTR pBuf,
             DWORD  *pdwResult) = 0;
        
        virtual long __stdcall GetIMEFileNameA( 
             HKL hKL,
             UINT uBufLen,
             LPSTR szFileName,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetIMEFileNameW( 
             HKL hKL,
             UINT uBufLen,
             LPWSTR szFileName,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetOpenStatus( 
             HIMC hIMC) = 0;
        
        virtual long __stdcall GetProperty( 
             HKL hKL,
             DWORD fdwIndex,
             DWORD  *pdwProperty) = 0;
        
        virtual long __stdcall GetRegisterWordStyleA( 
             HKL hKL,
             UINT nItem,
             STYLEBUFA  *pStyleBuf,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetRegisterWordStyleW( 
             HKL hKL,
             UINT nItem,
             STYLEBUFW  *pStyleBuf,
             UINT  *puCopied) = 0;
        
        virtual long __stdcall GetStatusWindowPos( 
             HIMC hIMC,
             POINT  *pptPos) = 0;
        
        virtual long __stdcall GetVirtualKey( 
             HWND hWnd,
             UINT  *puVirtualKey) = 0;
        
        virtual long __stdcall InstallIMEA( 
             LPSTR szIMEFileName,
             LPSTR szLayoutText,
             HKL  *phKL) = 0;
        
        virtual long __stdcall InstallIMEW( 
             LPWSTR szIMEFileName,
             LPWSTR szLayoutText,
             HKL  *phKL) = 0;
        
        virtual long __stdcall IsIME( 
             HKL hKL) = 0;
        
        virtual long __stdcall IsUIMessageA( 
             HWND hWndIME,
             UINT msg,
             WPARAM wParam,
             LPARAM lParam) = 0;
        
        virtual long __stdcall IsUIMessageW( 
             HWND hWndIME,
             UINT msg,
             WPARAM wParam,
             LPARAM lParam) = 0;
        
        virtual long __stdcall NotifyIME( 
             HIMC hIMC,
             DWORD dwAction,
             DWORD dwIndex,
             DWORD dwValue) = 0;
        
        virtual long __stdcall RegisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szRegister) = 0;
        
        virtual long __stdcall RegisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szRegister) = 0;
        
        virtual long __stdcall ReleaseContext( 
             HWND hWnd,
             HIMC hIMC) = 0;
        
        virtual long __stdcall SetCandidateWindow( 
             HIMC hIMC,
             void  *pCandidate) = 0;
        
        virtual long __stdcall SetCompositionFontA( 
             HIMC hIMC,
             LOGFONTA  *plf) = 0;
        
        virtual long __stdcall SetCompositionFontW( 
             HIMC hIMC,
             LOGFONTW  *plf) = 0;
        
        virtual long __stdcall SetCompositionStringA( 
             HIMC hIMC,
             DWORD dwIndex,
             LPVOID pComp,
             DWORD dwCompLen,
             LPVOID pRead,
             DWORD dwReadLen) = 0;
        
        virtual long __stdcall SetCompositionStringW( 
             HIMC hIMC,
             DWORD dwIndex,
             LPVOID pComp,
             DWORD dwCompLen,
             LPVOID pRead,
             DWORD dwReadLen) = 0;
        
        virtual long __stdcall SetCompositionWindow( 
             HIMC hIMC,
             void  *pCompForm) = 0;
        
        virtual long __stdcall SetConversionStatus( 
             HIMC hIMC,
             DWORD fdwConversion,
             DWORD fdwSentence) = 0;
        
        virtual long __stdcall SetOpenStatus( 
             HIMC hIMC,
             BOOL fOpen) = 0;
        
        virtual long __stdcall SetStatusWindowPos( 
             HIMC hIMC,
             POINT  *pptPos) = 0;
        
        virtual long __stdcall SimulateHotKey( 
             HWND hWnd,
             DWORD dwHotKeyID) = 0;
        
        virtual long __stdcall UnregisterWordA( 
             HKL hKL,
             LPSTR szReading,
             DWORD dwStyle,
             LPSTR szUnregister) = 0;
        
        virtual long __stdcall UnregisterWordW( 
             HKL hKL,
             LPWSTR szReading,
             DWORD dwStyle,
             LPWSTR szUnregister) = 0;
        
        virtual long __stdcall Activate( 
             BOOL fRestoreLayout) = 0;
        
        virtual long __stdcall Deactivate( void) = 0;
        
        virtual long __stdcall OnDefWindowProc( 
             HWND hWnd,
             UINT Msg,
             WPARAM wParam,
             LPARAM lParam,
             LRESULT  *plResult) = 0;
        
        virtual long __stdcall FilterClientWindows( 
             ATOM  *aaClassList,
             UINT uSize) = 0;
        
        virtual long __stdcall GetCodePageA( 
             HKL hKL,
             UINT  *uCodePage) = 0;
        
        virtual long __stdcall GetLangId( 
             HKL hKL,
             WORD  *plid) = 0;
        
        virtual long __stdcall AssociateContextEx( 
             HWND hWnd,
             HIMC hIMC,
             DWORD dwFlags) = 0;
        
        virtual long __stdcall DisableIME( 
             DWORD idThread) = 0;
        
        virtual long __stdcall GetImeMenuItemsA( 
             HIMC hIMC,
             DWORD dwFlags,
             DWORD dwType,
             void  *pImeParentMenu,
             void  *pImeMenu,
             DWORD dwSize,
             DWORD  *pdwResult) = 0;
        
        virtual long __stdcall GetImeMenuItemsW( 
             HIMC hIMC,
             DWORD dwFlags,
             DWORD dwType,
             void  *pImeParentMenu,
            void  *pImeMenu,
             DWORD dwSize,
             DWORD  *pdwResult) = 0;
        
        virtual long __stdcall EnumInputContext( 
             DWORD idThread,
             void  **ppEnum) = 0;
        
    };

#endif

//
// End of "$Id: aimm.h 7913 2010-11-29 18:18:27Z greg.ercolano $".
//
