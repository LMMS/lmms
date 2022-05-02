/*
 * RemoteVstPlugin.cpp - LMMS VST Support Layer (RemotePlugin client)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * Code partly taken from (X)FST:
 * 		Copyright (c) 2004 Paul Davis
 * 		Copyright (c) 2004 Torben Hohn
 *	 	Copyright (c) 2002 Kjetil S. Matheussen
 *
 * X11 code partly taken from https://github.com/ekenberg/vstminihost:
 *      Copyright (c) 2012 Johan Ekenberg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "lmmsconfig.h"

#include "RemotePluginClient.h"

#ifdef LMMS_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef LMMS_BUILD_LINUX

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif

#ifndef NATIVE_LINUX_VST
#include <wine/exception.h>
#endif

#endif

#ifndef NATIVE_LINUX_VST
#define USE_WS_PREFIX
#include <windows.h>
#else
#include <dlfcn.h>
#include <pthread.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <sstream>
#include <time.h>

// https://stackoverflow.com/questions/22476110/c-compiling-error-including-x11-x-h-x11-xlib-h
#undef Bool
#undef CursorShape
#undef Expose
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef None
#undef Status
#undef Unsorted
#endif

#ifdef USE_MINGW_THREADS_REPLACEMENT
#	include <mingw.mutex.h>
#else
#	include <mutex>
#endif

#include <algorithm>
#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <string>
#include <cstring>

#include <aeffectx.h>

#if kVstVersion < 2400

#define OLD_VST_SDK

struct ERect
{
    short top;
    short left;
    short bottom;
    short right;
} ;

#endif


#include "lmms_basics.h"
#include "Midi.h"
#include "communication.h"
#include "IoHelper.h"

#include "VstSyncData.h"

using namespace std;

static VstHostLanguages hlang = LanguageEnglish;

static bool EMBED = false;
static bool EMBED_X11 = false;
static bool EMBED_WIN32 = false;
static bool HEADLESS = false;

class RemoteVstPlugin;

RemoteVstPlugin * __plugin = nullptr;

#ifndef NATIVE_LINUX_VST
HWND __MessageHwnd = nullptr;
DWORD __processingThreadId = 0;
#else
pthread_t __processingThreadId = 0;
#endif

#ifdef _WIN32
//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetErrorAsString(DWORD errorMessageID)
{
	//Get the error message, if any.
	if(errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
								 nullptr, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}
#endif

class RemoteVstPlugin : public RemotePluginClient
{
public:
#ifdef SYNC_WITH_SHM_FIFO
	RemoteVstPlugin( const std::string& _shm_in, const std::string& _shm_out );
#else
	RemoteVstPlugin( const char * socketPath );
#endif
	virtual ~RemoteVstPlugin();

	virtual bool processMessage( const message & _m );

	void init( const std::string & _plugin_file );
	void initEditor();
	void showEditor();
	void hideEditor();
	void destroyEditor();

	virtual void process( const sampleFrame * _in, sampleFrame * _out );


	virtual void processMidiEvent( const MidiEvent& event, const f_cnt_t offset );

	// set given sample-rate for plugin
	virtual void updateSampleRate()
	{
		SuspendPlugin suspend( this );
		pluginDispatch( effSetSampleRate, 0, 0,
						nullptr, (float) sampleRate() );
	}

	// set given buffer-size for plugin
	virtual void updateBufferSize()
	{
		SuspendPlugin suspend( this );
		pluginDispatch( effSetBlockSize, 0, bufferSize() );
	}

	void setResumed( bool resumed )
	{
		m_resumed = resumed;
		pluginDispatch( effMainsChanged, 0, resumed ? 1 : 0 );
	}

	inline bool isResumed() const
	{
		return m_resumed;
	}

	inline bool isInitialized() const
	{
		return m_initialized;
	}


	// set given tempo
	void setBPM( const bpm_t _bpm )
	{
		m_bpm = _bpm;
	}

	// determine VST-version the plugin uses
	inline int pluginVersion()
	{
		return pluginDispatch( effGetVendorVersion );
	}

	// determine name of plugin
	const char * pluginName();

	// determine vendor of plugin
	const char * pluginVendorString();

	// determine product-string of plugin
	const char * pluginProductString();

	// determine name of current program
	const char * programName();

	void getParameterDisplays();

	void getParameterLabels();

	// send name of current program back to host
	void sendCurrentProgramName();

	// do a complete parameter-dump and post it
	void getParameterDump();

	// read parameter-dump and set it for plugin
	void setParameterDump( const message & _m );

	// save settings chunk of plugin into file
	void saveChunkToFile( const std::string & _file );

	// restore settings chunk of plugin from file
	void loadChunkFromFile( const std::string & _file, int _len );

	// restore settings chunk of plugin from file
	void loadPresetFile( const std::string & _file );

	// sets given program index
	void setProgram( int index );

	// rotate current program by given offset
	void rotateProgram( int offset );

	// Load names of presets/programs
	void getProgramNames();

	// Save presets/programs
	void savePreset( const std::string & _file );

	// number of inputs
	virtual int inputCount() const
	{
		if( m_plugin )
		{
			return m_plugin->numInputs;
		}
		return 0;
	}

	// number of outputs
	virtual int outputCount() const
	{
		if( m_plugin )
		{
			return m_plugin->numOutputs;
		}
		return 0;
	}

	// has to be called as soon as input- or output-count changes
	int updateInOutCount();

	inline void lockShm()
	{
		m_shmLock.lock();
	}

	inline bool tryLockShm()
	{
		return m_shmLock.try_lock();
	}

	inline void unlockShm()
	{
		m_shmLock.unlock();
	}

	inline bool isShmValid()
	{
		return m_shmValid;
	}

	inline void setShmIsValid( bool valid )
	{
		m_shmValid = valid;
	}

	inline bool isProcessing() const
	{
		return m_processing;
	}

	inline void setProcessing( bool processing )
	{
		m_processing = processing;
	}

	inline void queueMessage( const message & m ) {
#ifdef NATIVE_LINUX_VST
		pthread_mutex_lock(&message_mutex);
#endif
		m_messageList.push( m );
#ifdef NATIVE_LINUX_VST
		pthread_mutex_unlock(&message_mutex);
#endif
	}

	inline bool shouldGiveIdle() const
	{
		return m_shouldGiveIdle;
	}

	inline void setShouldGiveIdle( bool shouldGiveIdle )
	{
		m_shouldGiveIdle = shouldGiveIdle;
	}

	void idle();
	void processUIThreadMessages();
#ifdef NATIVE_LINUX_VST
	void sendX11Idle();
#endif

#ifndef NATIVE_LINUX_VST
	static DWORD WINAPI processingThread( LPVOID _param );
#else
	static void * processingThread( void * _param );
#endif
	
	static bool setupMessageWindow();
	
#ifndef NATIVE_LINUX_VST
	static DWORD WINAPI guiEventLoop();
#else
	void guiEventLoop();
#endif
	
#ifndef NATIVE_LINUX_VST
	static LRESULT CALLBACK wndProc( HWND hwnd, UINT uMsg,
					WPARAM wParam, LPARAM lParam );
#endif

private:
	enum GuiThreadMessages
	{
		None,
		ProcessPluginMessage,
		GiveIdle,
		ClosePlugin
	} ;

	struct SuspendPlugin {
		SuspendPlugin( RemoteVstPlugin * plugin ) :
			m_plugin( plugin ),
			m_resumed( plugin->isResumed() )
		{
			if( m_resumed ) { m_plugin->setResumed( false ); }
		}

		~SuspendPlugin()
		{
			if( m_resumed ) { m_plugin->setResumed( true ); }
		}

	private:
		RemoteVstPlugin * m_plugin;
		bool m_resumed;
	};

	// callback used by plugin for being able to communicate with it's host
	static intptr_t VST_CALL_CONV hostCallback( AEffect * _effect, int32_t _opcode,
					int32_t _index, intptr_t _value,
					void * _ptr, float _opt );


	bool load( const std::string & _plugin_file );

	int pluginDispatch( int cmd, int param1 = 0, int param2 = 0,
					void * p = nullptr, float f = 0 )
	{
		if( m_plugin )
		{
			return m_plugin->dispatcher( m_plugin, cmd, param1, param2, p, f );
		}
		return 0;
	}


	std::string m_shortName;

#ifndef NATIVE_LINUX_VST
	HINSTANCE m_libInst;
#else
	void* m_libInst = nullptr;
#endif

	AEffect * m_plugin;
#ifndef NATIVE_LINUX_VST
	HWND m_window = nullptr;
#else
	Window m_window = 0;
	Display* m_display = nullptr;
	Atom m_wmDeleteMessage;
	bool m_x11WindowVisible = false;
#endif
	intptr_t m_windowID;
	int m_windowWidth;
	int m_windowHeight;

	bool m_initialized;
	bool m_resumed;

	bool m_processing;

#ifdef NATIVE_LINUX_VST
	pthread_mutex_t message_mutex = PTHREAD_MUTEX_INITIALIZER;
	bool m_shouldQuit = false;
#endif
	std::queue<message> m_messageList;
	bool m_shouldGiveIdle;


	float * * m_inputs;
	float * * m_outputs;

	std::mutex m_shmLock;
	bool m_shmValid;

	typedef std::vector<VstMidiEvent> VstMidiEventList;
	VstMidiEventList m_midiEvents;

	bpm_t m_bpm;
	double m_currentSamplePos;
	int m_currentProgram;

	// host to plugin synchronisation data structure
	struct in
	{
		double lastppqPos;
		double m_Timestamp;
		int32_t m_lastFlags;
	} ;

	in * m_in;

	const VstSyncData* m_vstSyncData;

} ;




#ifdef SYNC_WITH_SHM_FIFO
RemoteVstPlugin::RemoteVstPlugin( const std::string& _shm_in, const std::string& _shm_out ) :
	RemotePluginClient( _shm_in, _shm_out ),
#else
RemoteVstPlugin::RemoteVstPlugin( const char * socketPath ) :
	RemotePluginClient( socketPath ),
#endif
	m_libInst( nullptr ),
	m_plugin( nullptr ),
	m_windowID( 0 ),
	m_windowWidth( 0 ),
	m_windowHeight( 0 ),
	m_initialized( false ),
	m_resumed( false ),
	m_processing( false ),
	m_messageList(),
	m_shouldGiveIdle( false ),
	m_inputs( nullptr ),
	m_outputs( nullptr ),
	m_shmValid( false ),
	m_midiEvents(),
	m_bpm( 0 ),
	m_currentSamplePos( 0 ),
	m_currentProgram( -1 ),
	m_in( nullptr ),
	m_vstSyncData( nullptr )
{
	__plugin = this;

	m_vstSyncData = RemotePluginClient::getVstSyncData();
	if( m_vstSyncData == nullptr )
	{
		fprintf(stderr, "RemoteVstPlugin.cpp: "
			"Failed to initialize shared memory for VST synchronization.\n"
			" (VST-host synchronization will be disabled)\n");
		const auto vstSyncData = (VstSyncData*) malloc( sizeof( VstSyncData ) );
		vstSyncData->isPlaying = true;
		vstSyncData->timeSigNumer = 4;
		vstSyncData->timeSigDenom = 4;
		vstSyncData->ppqPos = 0;
		vstSyncData->isCycle = false;
		vstSyncData->hasSHM = false;
		vstSyncData->m_playbackJumped = false;
		vstSyncData->m_sampleRate = sampleRate();
		m_vstSyncData = vstSyncData;
	}

	m_in = ( in* ) new char[ sizeof( in ) ];
	m_in->lastppqPos = 0;
	m_in->m_Timestamp = -1;
	m_in->m_lastFlags = 0;

	// process until we have loaded the plugin
	while( 1 )
	{
		message m = receiveMessage();
		processMessage( m );
		if( m.id == IdVstLoadPlugin || m.id == IdQuit )
		{
			break;
		}
	}
}




RemoteVstPlugin::~RemoteVstPlugin()
{
	destroyEditor();
	setResumed( false );
	pluginDispatch( effClose );

	if (!m_vstSyncData->hasSHM)
	{
		delete m_vstSyncData;
		m_vstSyncData = nullptr;
	}

	if( m_libInst != nullptr )
	{
#ifndef NATIVE_LINUX_VST
		FreeLibrary( m_libInst );
#else
		dlclose(m_libInst);
#endif
		m_libInst = nullptr;
	}

	delete[] m_inputs;
	delete[] m_outputs;
}




bool RemoteVstPlugin::processMessage( const message & _m )
{
	if (! EMBED)
	{
		switch( _m.id )
		{
		case IdShowUI:
			showEditor();
			return true;

		case IdHideUI:
			hideEditor();
			return true;

		case IdToggleUI:
#ifndef NATIVE_LINUX_VST
			if( m_window && IsWindowVisible( m_window ) )
#else
			if (m_window && m_x11WindowVisible)
#endif
			{
				hideEditor();
			}
			else
			{
				showEditor();
			}

			return true;

		case IdIsUIVisible:
#ifndef NATIVE_LINUX_VST
			bool visible = m_window && IsWindowVisible( m_window );
#else
			bool visible = m_window && m_x11WindowVisible;
#endif
			sendMessage( message( IdIsUIVisible )
						 .addInt( visible ? 1 : 0 ) );
			return true;
		}
	}
	else if (EMBED && _m.id == IdShowUI)
	{
#ifndef NATIVE_LINUX_VST
		ShowWindow( m_window, SW_SHOWNORMAL );
		UpdateWindow( m_window );
#endif
		return true;
	}

	switch( _m.id )
	{
		case IdVstLoadPlugin:
			init( _m.getString() );
			break;

		case IdVstSetTempo:
			setBPM( _m.getInt() );
			break;

		case IdVstSetLanguage:
			hlang = static_cast<VstHostLanguages>( _m.getInt() );
			break;

		case IdVstGetParameterDump:
			getParameterDump();
			break;

		case IdVstSetParameterDump:
			setParameterDump( _m );
			break;

		case IdSaveSettingsToFile:
			saveChunkToFile( _m.getString() );
			sendMessage( IdSaveSettingsToFile );
			break;

		case IdLoadSettingsFromFile:
			loadChunkFromFile( _m.getString( 0 ), _m.getInt( 1 ) );
			sendMessage( IdLoadSettingsFromFile );
			break;

		case IdLoadPresetFile:
			loadPresetFile( _m.getString( 0 ) );
			sendMessage( IdLoadPresetFile );
			break;

		case IdVstSetProgram:
			setProgram( _m.getInt( 0 ) );
			sendMessage( IdVstSetProgram );
			break;

		case IdVstCurrentProgram:
			sendMessage( message( IdVstCurrentProgram ).addInt( m_currentProgram ) );
			break;

		case IdVstRotateProgram:
			rotateProgram( _m.getInt( 0 ) );
			sendMessage( IdVstRotateProgram );
			break;

		case IdVstProgramNames:
			getProgramNames();
			break;

		case IdSavePresetFile:
			savePreset( _m.getString( 0 ) );
			sendMessage( IdSavePresetFile );
			break;

		case IdVstSetParameter:
			m_plugin->setParameter( m_plugin, _m.getInt( 0 ), _m.getFloat( 1 ) );
			//sendMessage( IdVstSetParameter );
			break;

		case IdVstParameterDisplays:
			getParameterDisplays();
			break;

		case IdVstParameterLabels:
			getParameterLabels();
			break;


		case IdVstIdleUpdate:
		{
			int newCurrentProgram = pluginDispatch( effGetProgram );
			if( newCurrentProgram != m_currentProgram )
			{
				m_currentProgram = newCurrentProgram;
				sendCurrentProgramName();
			}

			break;
		}

		case IdIdle:
		{
#ifdef NATIVE_LINUX_VST
			idle();
#endif
			break;
		}
		
#ifdef NATIVE_LINUX_VST
		case IdQuit:
		{
			m_shouldQuit = true;
			break;
		}
#endif
		default:
			return RemotePluginClient::processMessage( _m );
	}
	return true;
}




void RemoteVstPlugin::init( const std::string & _plugin_file )
{
	if( load( _plugin_file ) == false )
	{
		sendMessage( IdVstFailedLoadingPlugin );
		return;
	}

	updateInOutCount();
	updateBufferSize();
	updateSampleRate();

	/* set program to zero */
	/* i comment this out because it breaks dfx Geometer
	 * looks like we cant set programs for it
	 *
	pluginDispatch( effSetProgram, 0, 0 ); */
	// request rate and blocksize

	setResumed( true );

	debugMessage( "creating editor\n" );
	initEditor();
	debugMessage( "editor successfully created\n" );


	// now post some information about our plugin
	sendMessage( message( IdVstPluginWindowID ).addInt( m_windowID ) );

	sendMessage( message( IdVstPluginEditorGeometry ).
						addInt( m_windowWidth ).
						addInt( m_windowHeight ) );
	sendMessage( message( IdVstPluginName ).addString( pluginName() ) );
	debugMessage( std::string("plugin name: ") + pluginName() + "\n" );
	sendMessage( message( IdVstPluginVersion ).addInt( pluginVersion() ) );
	sendMessage( message( IdVstPluginVendorString ).
					addString( pluginVendorString() ) );
	sendMessage( message( IdVstPluginProductString ).
					addString( pluginProductString() ) );
	sendMessage( message( IdVstParameterCount ).
					addInt( m_plugin->numParams ) );

	sendMessage( IdInitDone );

	debugMessage( "initialization done\n" );
	m_initialized = true;
}




static void close_check( FILE* fp )
{
	if (!fp) {return;}
	if( fclose( fp ) )
	{
		perror( "fclose" );
	}
}




void RemoteVstPlugin::initEditor()
{
	if( HEADLESS || m_window || !( m_plugin->flags & effFlagsHasEditor ) )
	{
		return;
	}

#ifndef NATIVE_LINUX_VST
	HMODULE hInst = GetModuleHandle( nullptr );
	if( hInst == nullptr )
	{
		debugMessage( "initEditor(): can't get module handle\n" );
		return;
	}


	DWORD dwStyle;
	if (EMBED) {
		dwStyle = WS_POPUP | WS_SYSMENU | WS_BORDER;
	} else {
		dwStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
	}

	m_window = CreateWindowEx( WS_EX_APPWINDOW, "LVSL", pluginName(),
		dwStyle,
		0, 0, 10, 10, nullptr, nullptr, hInst, nullptr );
	if( m_window == nullptr )
	{
		debugMessage( "initEditor(): cannot create editor window\n" );
		return;
	}


	pluginDispatch( effEditOpen, 0, 0, m_window );

	ERect * er;
	pluginDispatch( effEditGetRect, 0, 0, &er );

	m_windowWidth = er->right - er->left;
	m_windowHeight = er->bottom - er->top;

	RECT windowSize = { 0, 0, m_windowWidth, m_windowHeight };
	AdjustWindowRect( &windowSize, dwStyle, false );
	SetWindowPos( m_window, 0, 0, 0, windowSize.right - windowSize.left,
			windowSize.bottom - windowSize.top, SWP_NOACTIVATE |
						SWP_NOMOVE | SWP_NOZORDER );
	pluginDispatch( effEditTop );

#ifdef LMMS_BUILD_LINUX
	m_windowID = (intptr_t) GetProp( m_window, "__wine_x11_whole_window" );
#else
	// 64-bit versions of Windows use 32-bit handles for interoperability
	m_windowID = (intptr_t) m_window;
#endif

#else
	XEvent e;
	Atom prop_atom, val_atom;
	
	if (m_display == nullptr)
	{
		m_display = XOpenDisplay(nullptr);
	}
	m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display), 0, 0, 400, 400, 0, 0, 0);
	
	m_wmDeleteMessage = XInternAtom(m_display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(m_display, m_window, &m_wmDeleteMessage, 1);
	
	// make tool window
	prop_atom = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE", False);
	val_atom = XInternAtom(m_display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	XChangeProperty(m_display, m_window, prop_atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&val_atom, 1);
	
	// change name
	XStoreName(m_display, m_window, pluginName());
	
	ERect * er;
	pluginDispatch(effEditGetRect, 0, 0, &er);

	m_windowWidth = er->right - er->left;
	m_windowHeight = er->bottom - er->top;
	XResizeWindow(m_display, m_window, m_windowWidth, m_windowHeight);
	
	XMapWindow(m_display, m_window);
	XFlush(m_display);
	
	pluginDispatch(effEditOpen, 0, (intptr_t) m_display, (void*) m_window);
	
	XSelectInput(m_display, m_window, SubstructureNotifyMask | ButtonPressMask | ButtonReleaseMask
			| ButtonMotionMask | ExposureMask | KeyPressMask);
	
	pluginDispatch(effEditTop);
	m_x11WindowVisible = true;
#endif
}




void RemoteVstPlugin::showEditor() {
	if( !EMBED && !HEADLESS && m_window )
	{
#ifndef NATIVE_LINUX_VST
		ShowWindow( m_window, SW_SHOWNORMAL );
#else
		if (!m_x11WindowVisible)
		{
			XMapWindow(m_display, m_window);
			XFlush(m_display);
			m_x11WindowVisible = true;
		}
#endif
	}
}




void RemoteVstPlugin::hideEditor() {
	if( !EMBED && !HEADLESS && m_window )
	{
#ifndef NATIVE_LINUX_VST
		ShowWindow( m_window, SW_HIDE );
#else
		if (m_x11WindowVisible)
		{
			XUnmapWindow(m_display, m_window);
			XFlush(m_display);
			m_x11WindowVisible = false;
		}
#endif
	}
}




void RemoteVstPlugin::destroyEditor()
{
	if( !m_window )
	{
		return;
	}

	pluginDispatch( effEditClose );
#ifndef NATIVE_LINUX_VST
	// Destroying the window takes some time in Wine 1.8.5
	DestroyWindow( m_window );
	m_window = nullptr;
#else
	if (m_display)
	{
		XCloseDisplay(m_display);
	}
	m_display = nullptr;
	m_window = 0;
#endif
}




bool RemoteVstPlugin::load( const std::string & _plugin_file )
{
#ifndef NATIVE_LINUX_VST
	if( ( m_libInst = LoadLibraryW( toWString(_plugin_file).c_str() ) ) == nullptr )
	{
		DWORD error = GetLastError();
		debugMessage( "LoadLibrary failed: " + GetErrorAsString(error) );
		return false;
	}
#else
	m_libInst = dlopen(_plugin_file.c_str(), RTLD_LAZY);
	if ( m_libInst == nullptr) {
		debugMessage( std::string("LoadLibrary failed: ") + dlerror() );
		return false;
	}
#endif

	typedef AEffect * ( VST_CALL_CONV * mainEntryPointer )
						( audioMasterCallback );
#ifndef NATIVE_LINUX_VST
	mainEntryPointer mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "VSTPluginMain" );
	if( mainEntry == nullptr )
	{
		mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "VstPluginMain" );
	}
	if( mainEntry == nullptr )
	{
		mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "main" );
	}
#else
	mainEntryPointer mainEntry = (mainEntryPointer) dlsym(m_libInst, "VSTPluginMain");
	if( mainEntry == nullptr )
		mainEntry = (mainEntryPointer) dlsym(m_libInst, "VstPluginMain");
	if( mainEntry == nullptr )
		mainEntry = (mainEntryPointer) dlsym(m_libInst, "main");
#endif

	if( mainEntry == nullptr )
	{
		debugMessage( "could not find entry point\n" );
		return false;
	}

	m_plugin = mainEntry( hostCallback );
	if( m_plugin == nullptr )
	{
		debugMessage( "mainEntry procedure returned nullptr\n" );
		return false;
	}

	if( m_plugin->magic != kEffectMagic )
	{
		debugMessage( "File is not a VST plugin\n" );
		return false;
	}


	char id[5];
	sprintf( id, "%c%c%c%c", ((char *)&m_plugin->uniqueID)[3],
					 ((char *)&m_plugin->uniqueID)[2],
					 ((char *)&m_plugin->uniqueID)[1],
					 ((char *)&m_plugin->uniqueID)[0] );
	id[4] = 0;
	sendMessage( message( IdVstPluginUniqueID ).addString( id ) );

	pluginDispatch( effOpen );

	return true;
}




void RemoteVstPlugin::process( const sampleFrame * _in, sampleFrame * _out )
{
	// first we gonna post all MIDI-events we enqueued so far
	if( m_midiEvents.size() )
	{
		// since MIDI-events are not received immediately, we
		// have to have them stored somewhere even after
		// dispatcher-call, so we create static copies of the
		// data and post them
#define MIDI_EVENT_BUFFER_COUNT		1024
		static char eventsBuffer[sizeof( VstEvents ) + sizeof( VstMidiEvent * ) * MIDI_EVENT_BUFFER_COUNT];
		static VstMidiEvent vme[MIDI_EVENT_BUFFER_COUNT];

		// first sort events chronologically, since some plugins
		// (e.g. Sinnah) can hang if they're out of order
		std::stable_sort( m_midiEvents.begin(), m_midiEvents.end(),
				[]( const VstMidiEvent &a, const VstMidiEvent &b )
				{
					return a.deltaFrames < b.deltaFrames;
				} );

		VstEvents* events = (VstEvents *) eventsBuffer;
		events->reserved = 0;
		events->numEvents = m_midiEvents.size();

		int idx = 0;
		for( VstMidiEventList::iterator it = m_midiEvents.begin(); it != m_midiEvents.end(); ++it, ++idx )
		{
			memcpy( &vme[idx], &*it, sizeof( VstMidiEvent ) );
			events->events[idx] = (VstEvent *) &vme[idx];
		}

		m_midiEvents.clear();
		pluginDispatch( effProcessEvents, 0, 0, events );
	}

	// now we're ready to fetch sound from VST-plugin

	if( !tryLockShm() )
	{
		return;
	}

	if( !isShmValid() )
	{
		unlockShm();
		return;
	}

	for( int i = 0; i < inputCount(); ++i )
	{
		m_inputs[i] = &((float *) _in)[i * bufferSize()];
	}

	for( int i = 0; i < outputCount(); ++i )
	{
		m_outputs[i] = &((float *) _out)[i * bufferSize()];
		memset( m_outputs[i], 0, bufferSize() * sizeof( float ) );
	}

#ifdef OLD_VST_SDK
	if( m_plugin->flags & effFlagsCanReplacing )
	{
#endif
		m_plugin->processReplacing( m_plugin, m_inputs, m_outputs,
								bufferSize() );
#ifdef OLD_VST_SDK
	}
	else
	{
		m_plugin->process( m_plugin, m_inputs, m_outputs,
								bufferSize() );
	}
#endif

	unlockShm();

	m_currentSamplePos += bufferSize();
}




void RemoteVstPlugin::processMidiEvent( const MidiEvent& event, const f_cnt_t offset )
{
	VstMidiEvent vme;

	vme.type = kVstMidiType;
	vme.byteSize = 24;
	vme.deltaFrames = offset;
	vme.flags = 0;
	vme.detune = 0;
	vme.noteLength = 0;
	vme.noteOffset = 0;
	vme.noteOffVelocity = 0;
	vme.reserved1 = 0;
	vme.reserved2 = 0;
	vme.midiData[0] = event.type() + event.channel();

	switch( event.type() )
	{
		case MidiPitchBend:
			vme.midiData[1] = event.pitchBend() & 0x7f;
			vme.midiData[2] = event.pitchBend() >> 7;
			break;
		// TODO: handle more special cases
		default:
			vme.midiData[1] = event.key();
			vme.midiData[2] = event.velocity();
			break;
	}
	vme.midiData[3] = 0;

	m_midiEvents.push_back( vme );
}




const char * RemoteVstPlugin::pluginName()
{
	static char buf[32];
	buf[0] = 0;
	pluginDispatch( effGetEffectName, 0, 0, buf );
	buf[31] = 0;
	return buf;
}




const char * RemoteVstPlugin::pluginVendorString()
{
	static char buf[64];
	buf[0] = 0;
	pluginDispatch( effGetVendorString, 0, 0, buf );
	buf[63] = 0;
	return buf;
}




const char * RemoteVstPlugin::pluginProductString()
{
	static char buf[64];
	buf[0] = 0;
	pluginDispatch( effGetProductString, 0, 0, buf );
	buf[63] = 0;
	return buf;
}




const char * RemoteVstPlugin::programName()
{
	static char buf[24];

	memset( buf, 0, sizeof( buf ) );

	pluginDispatch( effGetProgramName, 0, 0, buf );

	buf[23] = 0;

	return buf;
}



// join the ParameterDisplays (stringified values without units) and send them to host
void RemoteVstPlugin::getParameterDisplays()
{
	std::string paramDisplays;
	static char buf[9]; // buffer for getting string
	for (int i=0; i< m_plugin->numParams; ++i)
	{
		memset( buf, 0, sizeof( buf ) ); // fill with '\0' because got string may not to be ended with '\0'
		pluginDispatch( effGetParamDisplay, i, 0, buf );
		buf[8] = 0;

		// each field shaped like: [length:number][content:string]
		paramDisplays += '0' + strlen(buf); // add length descriptor (length is up to 8)
		paramDisplays += buf;
	}

	sendMessage( message( IdVstParameterDisplays ).addString( paramDisplays.c_str() ) );
}



// join the ParameterLabels (units) and send them to host
void RemoteVstPlugin::getParameterLabels()
{
	std::string paramLabels;
	static char buf[9]; // buffer for getting string
	for (int i=0; i< m_plugin->numParams; ++i)
	{
		memset( buf, 0, sizeof( buf ) ); // fill with '\0' because got string may not to be ended with '\0'
		pluginDispatch( effGetParamLabel, i, 0, buf );
		buf[8] = 0;

		// each field shaped like: [length:number][content:string]
		paramLabels += '0' + strlen(buf); // add length descriptor (length is up to 8)
		paramLabels += buf;
	}

	sendMessage( message( IdVstParameterLabels ).addString( paramLabels.c_str() ) );
}




void RemoteVstPlugin::sendCurrentProgramName()
{
	char presName[64];
	sprintf( presName, "%d/%d: %s", pluginDispatch( effGetProgram ) + 1, m_plugin->numPrograms, programName() );

	sendMessage( message( IdVstCurrentProgramName ).addString( presName ) );
}



void RemoteVstPlugin::getParameterDump()
{
	message m( IdVstParameterDump );
	m.addInt( m_plugin->numParams );

	for( int i = 0; i < m_plugin->numParams; ++i )
	{
		char paramName[256];
		memset( paramName, 0, sizeof( paramName ) );
		pluginDispatch( effGetParamName, i, 0, paramName );
		paramName[sizeof(paramName)-1] = 0;

		m.addInt( i );
		m.addString( paramName );
		m.addFloat( m_plugin->getParameter( m_plugin, i ) );
	}

	sendMessage( m );
}




void RemoteVstPlugin::setParameterDump( const message & _m )
{
	const int n = _m.getInt( 0 );
	const int params = ( n > m_plugin->numParams ) ?
					m_plugin->numParams : n;
	int p = 0;
	for( int i = 0; i < params; ++i )
	{
		VstParameterDumpItem item;
		item.index = _m.getInt( ++p );
		item.shortLabel = _m.getString( ++p );
		item.value = _m.getFloat( ++p );
		m_plugin->setParameter( m_plugin, item.index, item.value );
	}
}




void RemoteVstPlugin::saveChunkToFile( const std::string & _file )
{
	if( m_plugin->flags & 32 )
	{
		void * chunk = nullptr;
		const int len = pluginDispatch( 23, 0, 0, &chunk );
		if( len > 0 )
		{
			FILE* fp = F_OPEN_UTF8( _file, "wb" );
			if (!fp)
			{
				fprintf( stderr,
					"Error opening file for saving chunk.\n" );
				return;
			}
			if ( fwrite( chunk, 1, len, fp ) != len )
			{
				fprintf( stderr,
					"Error saving chunk to file.\n" );
			}
			close_check( fp );
		}
	}
}




void RemoteVstPlugin::setProgram( int program )
{
	if( isInitialized() == false )
	{
		return;
	}

	if( program < 0 )
	{
		program = 0;
	}
	else if( program >= m_plugin->numPrograms )
	{
		program = m_plugin->numPrograms - 1;
	}
	pluginDispatch( effSetProgram, 0, program );

	sendCurrentProgramName();
}




void RemoteVstPlugin::rotateProgram( int offset )
{
	if( isInitialized() == false )
	{
		return;
	}

	int newProgram = pluginDispatch( effGetProgram ) + offset;

	if( newProgram < 0 )
	{
		newProgram = 0;
	}
	else if( newProgram >= m_plugin->numPrograms )
	{
		newProgram = m_plugin->numPrograms - 1;
	}
	pluginDispatch( effSetProgram, 0, newProgram );

	sendCurrentProgramName();
}




void RemoteVstPlugin::getProgramNames()
{
	char presName[1024+256*30];
	char curProgName[30];
	if (isInitialized() == false) return;
	bool progNameIndexed = ( pluginDispatch( 29, 0, -1, curProgName ) == 1 );

	if (m_plugin->numPrograms > 1) {
		if (progNameIndexed) {
			for (int i = 0; i< (m_plugin->numPrograms >= 256?256:m_plugin->numPrograms); i++)
			{
				pluginDispatch( 29, i, -1, curProgName );
				if (i == 0) 	sprintf( presName, "%s", curProgName );
				else		sprintf( presName + strlen(presName), "|%s", curProgName );
			}
		}
		else
		{
			int currProgram = pluginDispatch( effGetProgram );
			for (int i = 0; i< (m_plugin->numPrograms >= 256?256:m_plugin->numPrograms); i++)
			{
				pluginDispatch( effSetProgram, 0, i );
				if (i == 0) 	sprintf( presName, "%s", programName() );
				else		sprintf( presName + strlen(presName), "|%s", programName() );
			}
			pluginDispatch( effSetProgram, 0, currProgram );
		}
	} else sprintf( presName, "%s", programName() );

	presName[sizeof(presName)-1] = 0;

	sendMessage( message( IdVstProgramNames ).addString( presName ) );
}




inline unsigned int endian_swap(unsigned int& x)
{
    return (x>>24) | ((x<<8) & 0x00FF0000) | ((x>>8) & 0x0000FF00) | (x<<24);
}

struct sBank
{
	unsigned int chunkMagic;
	unsigned int byteSize;
	unsigned int fxMagic;
	unsigned int version;
	unsigned int fxID;
	unsigned int fxVersion;
	unsigned int numPrograms;
	char prgName[28];
};

void RemoteVstPlugin::savePreset( const std::string & _file )
{
	unsigned int chunk_size = 0;
	sBank * pBank = ( sBank* ) new char[ sizeof( sBank ) ];
	char progName[ 128 ] = { 0 };
	char* data = nullptr;
	const bool chunky = ( m_plugin->flags & ( 1 << 5 ) ) != 0;
	bool isPreset = _file.substr( _file.find_last_of( "." ) + 1 )  == "fxp";
	int presNameLen = _file.find_last_of( "/" ) + _file.find_last_of( "\\" ) + 2;

	if (isPreset)
	{
		for (size_t i = 0; i < _file.length() - 4 - presNameLen; i++)
			progName[i] = i < 23 ? _file[presNameLen + i] : 0;
		pluginDispatch( 4, 0, 0, progName );
	}
	if ( chunky )
		chunk_size = pluginDispatch( 23, isPreset, 0, &data );
	else {
		if (isPreset) {
			chunk_size = m_plugin->numParams * sizeof( float );
			data = new char[ chunk_size ];
			unsigned int* toUIntArray = reinterpret_cast<unsigned int*>( data );
			for ( int i = 0; i < m_plugin->numParams; i++ )
			{
				float value = m_plugin->getParameter( m_plugin, i );
				unsigned int * pValue = ( unsigned int * ) &value;
				toUIntArray[ i ] = endian_swap( *pValue );
			}
		} else chunk_size = (((m_plugin->numParams * sizeof( float )) + 56)*m_plugin->numPrograms);
	}

	pBank->chunkMagic = 0x4B6E6343;
	pBank->byteSize = chunk_size + ( chunky ? sizeof( int ) : 0 ) + 48;
	if (!isPreset) pBank->byteSize += 100;
	pBank->byteSize = endian_swap( pBank->byteSize );
	pBank->fxMagic = chunky ? 0x68435046 : 0x6B437846;
	if (!isPreset && chunky) pBank->fxMagic = 0x68434246;
	if (!isPreset &&!chunky) pBank->fxMagic = 0x6B427846;

	pBank->version = 0x01000000;
	unsigned int uIntToFile = (unsigned int) m_plugin->uniqueID;
	pBank->fxID = endian_swap( uIntToFile );
	uIntToFile = (unsigned int) pluginVersion();
	pBank->fxVersion = endian_swap( uIntToFile );
	uIntToFile = (unsigned int) chunky ? m_plugin->numPrograms : m_plugin->numParams;
	if (!isPreset &&!chunky) uIntToFile = (unsigned int) m_plugin->numPrograms;
	pBank->numPrograms = endian_swap( uIntToFile );

	FILE * stream = F_OPEN_UTF8( _file, "wb" );
	if (!stream)
	{
		fprintf( stderr,
			"Error opening file for saving preset.\n" );
		return;
	}
	fwrite ( pBank, 1, 28, stream );
	fwrite ( progName, 1, isPreset ? 28 : 128, stream );
	if ( chunky ) {
		uIntToFile = endian_swap( chunk_size );
		fwrite ( &uIntToFile, 1, 4, stream );
	}
	if (pBank->fxMagic != 0x6B427846 )
		fwrite ( data, 1, chunk_size, stream );
	else {
		int numPrograms = m_plugin->numPrograms;
		int currProgram = pluginDispatch( effGetProgram );
		chunk_size = (m_plugin->numParams * sizeof( float ));
		pBank->byteSize = chunk_size + 48;
		pBank->byteSize = endian_swap( pBank->byteSize );
		pBank->fxMagic = 0x6B437846;
		uIntToFile = (unsigned int) m_plugin->numParams;
		pBank->numPrograms = endian_swap( uIntToFile );
		data = new char[ chunk_size ];
		unsigned int* pValue,* toUIntArray = reinterpret_cast<unsigned int*>( data );
		float value;
		for (int j = 0; j < numPrograms; j++) {
			pluginDispatch( effSetProgram, 0, j );
			pluginDispatch( effGetProgramName, 0, 0, pBank->prgName );
			fwrite ( pBank, 1, 56, stream );
			for ( int i = 0; i < m_plugin->numParams; i++ )
			{
				value = m_plugin->getParameter( m_plugin, i );
				pValue = ( unsigned int * ) &value;
				toUIntArray[ i ] = endian_swap( *pValue );
			}
			fwrite ( data, 1, chunk_size, stream );
		}
		pluginDispatch( effSetProgram, 0, currProgram );
	}
	fclose( stream );

	if ( !chunky ) 
		delete[] data;
	delete[] (sBank*)pBank;

}




void RemoteVstPlugin::loadPresetFile( const std::string & _file )
{
	void * chunk = nullptr;
	unsigned int * pLen = new unsigned int[ 1 ];
	unsigned int len = 0;
	sBank * pBank = (sBank*) new char[ sizeof( sBank ) ];
	FILE * stream = F_OPEN_UTF8( _file, "rb" );
	if (!stream)
	{
		fprintf( stderr,
			"Error opening file for loading preset.\n" );
		return;
	}
	if ( fread ( pBank, 1, 56, stream ) != 56 )
	{
		fprintf( stderr, "Error loading preset file.\n" );
	}
        pBank->fxID = endian_swap( pBank->fxID );
	pBank->numPrograms = endian_swap( pBank->numPrograms );
	unsigned int toUInt;
	float * pFloat;

	if (m_plugin->uniqueID != pBank->fxID) {
		sendMessage( message( IdVstCurrentProgramName ).
					addString( "Error: Plugin UniqID not match" ) );
		fclose( stream );
		delete[] (unsigned int*)pLen;
		delete[] (sBank*)pBank;
		return;
	}

	if( _file.substr( _file.find_last_of( "." ) + 1 ) != "fxp" )
		fseek ( stream , 156 , SEEK_SET );

	if(pBank->fxMagic != 0x6B427846) {
		if(pBank->fxMagic != 0x6B437846) {
			if ( fread (pLen, 1, 4, stream) != 4 )
			{
				fprintf( stderr,
					"Error loading preset file.\n" );
			}
			chunk = new char[len = endian_swap(*pLen)];
		} else chunk = new char[len = sizeof(float)*pBank->numPrograms];
		if ( fread (chunk, len, 1, stream) != 1 )
		{
			fprintf( stderr, "Error loading preset file.\n" );
		}
		fclose( stream );
	}

	if(_file.substr(_file.find_last_of(".") + 1) == "fxp") {
		pBank->prgName[23] = 0;
		pluginDispatch( 4, 0, 0, pBank->prgName );
		if(pBank->fxMagic != 0x6B437846)
			pluginDispatch( 24, 1, len, chunk );
		else
		{
			unsigned int* toUIntArray = reinterpret_cast<unsigned int*>( chunk );
			for (int i = 0; i < pBank->numPrograms; i++ )
			{
				toUInt = endian_swap( toUIntArray[ i ] );
				pFloat = ( float* ) &toUInt;
				m_plugin->setParameter( m_plugin, i, *pFloat );
			}
		}
	} else {
		if(pBank->fxMagic != 0x6B427846) {
			pluginDispatch( 24, 0, len, chunk );
		} else {
			int numPrograms = pBank->numPrograms;
			unsigned int * toUIntArray;
			int currProgram = pluginDispatch( effGetProgram );
			chunk = new char[ len = sizeof(float)*m_plugin->numParams ];
			toUIntArray = reinterpret_cast<unsigned int *>( chunk );
			for (int i =0; i < numPrograms; i++) {
				if ( fread (pBank, 1, 56, stream) != 56 )
				{
					fprintf( stderr,
					"Error loading preset file.\n" );
				}
				if ( fread (chunk, len, 1, stream) != 1 )
				{
					fprintf( stderr,
					"Error loading preset file.\n" );
				}
				pluginDispatch( effSetProgram, 0, i );
				pBank->prgName[23] = 0;
				pluginDispatch( 4, 0, 0, pBank->prgName );
				for (int j = 0; j < m_plugin->numParams; j++ ) {
					toUInt = endian_swap( toUIntArray[ j ] );
					pFloat = ( float* ) &toUInt;
					m_plugin->setParameter( m_plugin, j, *pFloat );
				}
			}
			pluginDispatch( effSetProgram, 0, currProgram );
			fclose( stream );
		}
	}

	sendCurrentProgramName();

	delete[] (unsigned int*)pLen;
	delete[] (sBank*)pBank;
	delete[] (char*)chunk;
}




void RemoteVstPlugin::loadChunkFromFile( const std::string & _file, int _len )
{
	char * chunk = new char[_len];

	FILE* fp = F_OPEN_UTF8( _file, "rb" );
	if (!fp)
	{
		fprintf( stderr,
			"Error opening file for loading chunk.\n" );
		return;
	}
	if ( fread( chunk, 1, _len, fp ) != _len )
	{
		fprintf( stderr, "Error loading chunk from file.\n" );
	}
	close_check( fp );

	pluginDispatch( effSetChunk, 0, _len, chunk );

	delete[] chunk;
}




int RemoteVstPlugin::updateInOutCount()
{
	if( inputCount() == RemotePluginClient::inputCount() &&
		outputCount() == RemotePluginClient::outputCount() )
	{
		return 1;
	}
	
#ifndef NATIVE_LINUX_VST
	if( GetCurrentThreadId() == __processingThreadId )
#else
	if( pthread_equal(pthread_self(), __processingThreadId) )
#endif
	{
		debugMessage( "Plugin requested I/O change from processing "
			"thread. Request denied; stability may suffer.\n" );
		return 0;
	}

	lockShm();

	setShmIsValid( false );

	unlockShm();

	delete[] m_inputs;
	delete[] m_outputs;

	m_inputs = nullptr;
	m_outputs = nullptr;

	setInputOutputCount( inputCount(), outputCount() );

	char buf[64];
	sprintf( buf, "inputs: %d  output: %d\n", inputCount(), outputCount() );
	debugMessage( buf );

	if( inputCount() > 0 )
	{
		m_inputs = new float * [inputCount()];
	}

	if( outputCount() > 0 )
	{
		m_outputs = new float * [outputCount()];
	}

	return 1;
}



//#define DEBUG_CALLBACKS
#ifdef DEBUG_CALLBACKS
#define SHOW_CALLBACK __plugin->debugMessage
#else
#define SHOW_CALLBACK(...)
#endif


/* TODO:
 * - complete audioMasterGetTime-handling (bars etc.)
 * - implement audioMasterProcessEvents
 * - audioMasterGetVendorVersion: return LMMS-version (config.h!)
 * - audioMasterGetDirectory: return either VST-plugin-dir or LMMS-workingdir
 * - audioMasterOpenFileSelector: show QFileDialog?
 */
intptr_t RemoteVstPlugin::hostCallback( AEffect * _effect, int32_t _opcode,
					int32_t _index, intptr_t _value,
						void * _ptr, float _opt )
{
	static VstTimeInfo _timeInfo;
#ifdef DEBUG_CALLBACKS
	char buf[64];
	sprintf( buf, "host-callback, opcode = %d\n", (int) _opcode );
	SHOW_CALLBACK( buf );
#endif

	// workaround for early callbacks by some plugins
	if( __plugin && __plugin->m_plugin == nullptr )
	{
		__plugin->m_plugin = _effect;
	}

	switch( _opcode )
	{
		case audioMasterAutomate:
			SHOW_CALLBACK( "amc: audioMasterAutomate\n" );
			// index, value, returns 0
			return 0;

		case audioMasterVersion:
			SHOW_CALLBACK( "amc: audioMasterVersion\n" );
			return 2300;

		case audioMasterCurrentId:
			SHOW_CALLBACK( "amc: audioMasterCurrentId\n" );
			// returns the unique id of a plug that's currently
			// loading
			return 0;

		case audioMasterIdle:
			SHOW_CALLBACK ("amc: audioMasterIdle\n" );
			// call application idle routine (this will
			// call effEditIdle for all open editors too)
#ifndef NATIVE_LINUX_VST
			PostMessage( __MessageHwnd, WM_USER, GiveIdle, 0 );
#else
			__plugin->sendX11Idle();
#endif
			return 0;

		case audioMasterPinConnected:
			SHOW_CALLBACK( "amc: audioMasterPinConnected\n" );
			// inquire if an input or output is beeing connected;
			// index enumerates input or output counting from zero:
			// value is 0 for input and != 0 otherwise. note: the
			// return value is 0 for <true> such that older versions
			// will always return true.
			return 0;

		case audioMasterGetTime:
			SHOW_CALLBACK( "amc: audioMasterGetTime\n" );
			// returns const VstTimeInfo* (or 0 if not supported)
			// <value> should contain a mask indicating which
			// fields are required (see valid masks above), as some
			// items may require extensive conversions

			// Shared memory was initialised? - see song.cpp
			//assert( __plugin->m_vstSyncData != nullptr );

			memset( &_timeInfo, 0, sizeof( _timeInfo ) );
			_timeInfo.samplePos = __plugin->m_currentSamplePos;
			_timeInfo.sampleRate = __plugin->m_vstSyncData->hasSHM ?
							__plugin->m_vstSyncData->m_sampleRate :
							__plugin->sampleRate();
			_timeInfo.flags = 0;
			_timeInfo.tempo = __plugin->m_vstSyncData->hasSHM ?
							__plugin->m_vstSyncData->m_bpm :
							__plugin->m_bpm;
			_timeInfo.timeSigNumerator = __plugin->m_vstSyncData->timeSigNumer;
			_timeInfo.timeSigDenominator = __plugin->m_vstSyncData->timeSigDenom;
			_timeInfo.flags |= kVstTempoValid;
			_timeInfo.flags |= kVstTimeSigValid;

			if( __plugin->m_vstSyncData->isCycle )
			{
				_timeInfo.cycleStartPos = __plugin->m_vstSyncData->cycleStart;
				_timeInfo.cycleEndPos = __plugin->m_vstSyncData->cycleEnd;
				_timeInfo.flags |= kVstCyclePosValid;
				_timeInfo.flags |= kVstTransportCycleActive;
			}

			if( __plugin->m_vstSyncData->ppqPos != 
							__plugin->m_in->m_Timestamp )
			{
				_timeInfo.ppqPos = __plugin->m_vstSyncData->ppqPos;
				__plugin->m_in->lastppqPos = __plugin->m_vstSyncData->ppqPos;
				__plugin->m_in->m_Timestamp = __plugin->m_vstSyncData->ppqPos;
			}
			else if( __plugin->m_vstSyncData->isPlaying )
			{
				if( __plugin->m_vstSyncData->hasSHM )
				{
					__plugin->m_in->lastppqPos +=
						__plugin->m_vstSyncData->m_bpm / 60.0
						* __plugin->m_vstSyncData->m_bufferSize
						/ __plugin->m_vstSyncData->m_sampleRate;
				}
				else
				{
					__plugin->m_in->lastppqPos +=
						__plugin->m_bpm / 60.0
						* __plugin->bufferSize()
						/ __plugin->sampleRate();
				}
				_timeInfo.ppqPos = __plugin->m_in->lastppqPos;
			}
//			_timeInfo.ppqPos = __plugin->m_vstSyncData->ppqPos;
			_timeInfo.flags |= kVstPpqPosValid;

			if( __plugin->m_vstSyncData->isPlaying )
			{
				_timeInfo.flags |= kVstTransportPlaying;
			}
			_timeInfo.barStartPos = ( (int) ( _timeInfo.ppqPos / 
				( 4 *__plugin->m_vstSyncData->timeSigNumer
				/ (float) __plugin->m_vstSyncData->timeSigDenom ) ) ) *
				( 4 * __plugin->m_vstSyncData->timeSigNumer
				/ (float) __plugin->m_vstSyncData->timeSigDenom );

			_timeInfo.flags |= kVstBarsValid;

			if( ( _timeInfo.flags & ( kVstTransportPlaying | kVstTransportCycleActive ) ) !=
				( __plugin->m_in->m_lastFlags & ( kVstTransportPlaying | kVstTransportCycleActive ) )
				|| __plugin->m_vstSyncData->m_playbackJumped )
			{
				_timeInfo.flags |= kVstTransportChanged;
			}
			__plugin->m_in->m_lastFlags = _timeInfo.flags;

			return (intptr_t) &_timeInfo;

		case audioMasterProcessEvents:
			SHOW_CALLBACK( "amc: audioMasterProcessEvents\n" );
			// VstEvents* in <ptr>
			return 0;

		case audioMasterIOChanged:
			SHOW_CALLBACK( "amc: audioMasterIOChanged\n" );
			// numInputs, numOutputs, and/or latency has changed
			return __plugin->updateInOutCount();

#ifdef OLD_VST_SDK
		case audioMasterWantMidi:
			SHOW_CALLBACK( "amc: audioMasterWantMidi\n" );
			// <value> is a filter which is currently ignored
			return 1;

		case audioMasterSetTime:
			SHOW_CALLBACK( "amc: audioMasterSetTime\n" );
			// VstTimenfo* in <ptr>, filter in <value>, not
			// supported
			return 0;

		case audioMasterTempoAt:
			SHOW_CALLBACK( "amc: audioMasterTempoAt\n" );
			return __plugin->m_bpm * 10000;

		case audioMasterGetNumAutomatableParameters:
			SHOW_CALLBACK( "amc: audioMasterGetNumAutomatable"
							"Parameters\n" );
			return 5000;

		case audioMasterGetParameterQuantization:
			SHOW_CALLBACK( "amc: audioMasterGetParameter\n"
							"Quantization\n" );
			// returns the integer value for +1.0 representation,
			// or 1 if full single float precision is maintained
			// in automation. parameter index in <value> (-1: all,
			// any)
			return 1;

		case audioMasterNeedIdle:
			SHOW_CALLBACK( "amc: audioMasterNeedIdle\n" );
			// plug needs idle calls (outside its editor window)
			return 1;

		case audioMasterGetPreviousPlug:
			SHOW_CALLBACK( "amc: audioMasterGetPreviousPlug\n" );
			// input pin in <value> (-1: first to come), returns
			// cEffect*
			return 0;

		case audioMasterGetNextPlug:
			SHOW_CALLBACK( "amc: audioMasterGetNextPlug\n" );
			// output pin in <value> (-1: first to come), returns
			// cEffect*
			return 0;

		case audioMasterWillReplaceOrAccumulate:
			SHOW_CALLBACK( "amc: audioMasterWillReplaceOr"
							"Accumulate\n" );
			// returns: 0: not supported, 1: replace, 2: accumulate
			return 1;

		case audioMasterGetSpeakerArrangement:
			SHOW_CALLBACK( "amc: audioMasterGetSpeaker"
							"Arrangement\n" );
			// (long)input in <value>, output in <ptr>
			return 0;

		case audioMasterSetOutputSampleRate:
			SHOW_CALLBACK( "amc: audioMasterSetOutputSample"
								"Rate\n" );
			// for variable i/o, sample rate in <opt>
			return 0;

		case audioMasterSetIcon:
			SHOW_CALLBACK( "amc: audioMasterSetIcon\n" );
			// TODO
			// void* in <ptr>, format not defined yet
			return 0;

		case audioMasterOpenWindow:
			SHOW_CALLBACK( "amc: audioMasterOpenWindow\n" );
			// TODO
			// returns platform specific ptr
			return 0;

		case audioMasterCloseWindow:
			SHOW_CALLBACK( "amc: audioMasterCloseWindow\n" );
			// TODO
			// close window, platform specific handle in <ptr>
			return 0;
#endif

		case audioMasterSizeWindow:
		{
			SHOW_CALLBACK( "amc: audioMasterSizeWindow\n" );
			if( __plugin->m_window == 0 )
			{
				return 0;
			}
			__plugin->m_windowWidth = _index;
			__plugin->m_windowHeight = _value;
#ifndef NATIVE_LINUX_VST
			HWND window = __plugin->m_window;
			DWORD dwStyle = GetWindowLongPtr( window, GWL_STYLE );
			RECT windowSize = { 0, 0, (int) _index, (int) _value };
			AdjustWindowRect( &windowSize, dwStyle, false );
			SetWindowPos( window, 0, 0, 0,
					windowSize.right - windowSize.left,
					windowSize.bottom - windowSize.top,
					SWP_NOACTIVATE | SWP_NOMOVE |
					SWP_NOOWNERZORDER | SWP_NOZORDER );
#else
			XResizeWindow(__plugin->m_display, __plugin->m_window, (int) _index, (int) _value);
			XFlush(__plugin->m_display);
#endif
			__plugin->sendMessage(
				message( IdVstPluginEditorGeometry ).
					addInt( __plugin->m_windowWidth ).
					addInt( __plugin->m_windowHeight ) );
			return 1;
		}

		case audioMasterGetSampleRate:
			SHOW_CALLBACK( "amc: audioMasterGetSampleRate\n" );
			return __plugin->sampleRate();

		case audioMasterGetBlockSize:
			SHOW_CALLBACK( "amc: audioMasterGetBlockSize\n" );

			return __plugin->bufferSize();

		case audioMasterGetInputLatency:
			SHOW_CALLBACK( "amc: audioMasterGetInputLatency\n" );
			return __plugin->bufferSize();

		case audioMasterGetOutputLatency:
			SHOW_CALLBACK( "amc: audioMasterGetOutputLatency\n" );
			return __plugin->bufferSize();

		case audioMasterGetCurrentProcessLevel:
			SHOW_CALLBACK( "amc: audioMasterGetCurrentProcess"
								"Level\n" );
			// returns: 0: not supported,
			// 1: currently in user thread (gui)
			// 2: currently in audio thread (where process is
			//    called)
			// 3: currently in 'sequencer' thread (midi, timer etc)
			// 4: currently offline processing and thus in user
			//    thread
			// other: not defined, but probably pre-empting user
			//        thread.
			return 0;

		case audioMasterGetAutomationState:
			SHOW_CALLBACK( "amc: audioMasterGetAutomationState\n" );
			// returns 0: not supported, 1: off, 2:read, 3:write,
			// 4:read/write offline
			return 0;

		case audioMasterOfflineStart:
			SHOW_CALLBACK( "amc: audioMasterOfflineStart\n" );
			return 0;

		case audioMasterOfflineRead:
			SHOW_CALLBACK( "amc: audioMasterOfflineRead\n" );
			// ptr points to offline structure, see below.
			// return 0: error, 1 ok
			return 0;

		case audioMasterOfflineWrite:
			SHOW_CALLBACK( "amc: audioMasterOfflineWrite\n" );
			// same as read
			return 0;

		case audioMasterOfflineGetCurrentPass:
			SHOW_CALLBACK( "amc: audioMasterOfflineGetCurrent"
								"Pass\n" );
			return 0;

		case audioMasterOfflineGetCurrentMetaPass:
			SHOW_CALLBACK( "amc: audioMasterOfflineGetCurrentMeta"
								"Pass\n");
			return 0;

		case audioMasterGetVendorString:
			SHOW_CALLBACK( "amc: audioMasterGetVendorString\n" );
			// fills <ptr> with a string identifying the vendor
			// (max 64 char)
			strcpy( (char *) _ptr, "Tobias Doerffel" );
			return 1;

		case audioMasterGetProductString:
			SHOW_CALLBACK( "amc: audioMasterGetProductString\n" );
			// fills <ptr> with a string with product name
			// (max 64 char)
			strcpy( (char *) _ptr,
					"LMMS VST Support Layer (LVSL)" );
			return 1;

		case audioMasterGetVendorVersion:
			SHOW_CALLBACK( "amc: audioMasterGetVendorVersion\n" );
			// returns vendor-specific version
			return 1000;

		case audioMasterVendorSpecific:
			SHOW_CALLBACK( "amc: audioMasterVendorSpecific\n" );
			// no definition, vendor specific handling
			return 0;

		case audioMasterCanDo:
			SHOW_CALLBACK( "amc: audioMasterCanDo\n" );
			return !strcmp( (char *) _ptr, "sendVstEvents" ) ||
				!strcmp( (char *) _ptr, "sendVstMidiEvent" ) ||
				!strcmp( (char *) _ptr, "sendVstTimeInfo" ) ||
				!strcmp( (char *) _ptr, "sizeWindow" ) ||
				!strcmp( (char *) _ptr, "supplyIdle" );

		case audioMasterGetLanguage:
			SHOW_CALLBACK( "amc: audioMasterGetLanguage\n" );
			return hlang;

		case audioMasterGetDirectory:
			SHOW_CALLBACK( "amc: audioMasterGetDirectory\n" );
			// get plug directory, FSSpec on MAC, else char*
			return 0;

		case audioMasterUpdateDisplay:
			SHOW_CALLBACK( "amc: audioMasterUpdateDisplay\n" );
			// something has changed, update 'multi-fx' display
#ifndef NATIVE_LINUX_VST
			PostMessage( __MessageHwnd, WM_USER, GiveIdle, 0 );
#else
			__plugin->sendX11Idle();
#endif
			return 0;

#if kVstVersion > 2
		case audioMasterBeginEdit:
			SHOW_CALLBACK( "amc: audioMasterBeginEdit\n" );
			// begin of automation session (when mouse down),
			// parameter index in <index>
			return 0;

		case audioMasterEndEdit:
			SHOW_CALLBACK( "amc: audioMasterEndEdit\n" );
			// end of automation session (when mouse up),
			// parameter index in <index>
			return 0;

		case audioMasterOpenFileSelector:
			SHOW_CALLBACK( "amc: audioMasterOpenFileSelector\n" );
			// open a fileselector window with VstFileSelect*
			// in <ptr>
			return 0;
#endif
		default:
			SHOW_CALLBACK( "amd: not handled" );
			break;
	}

	return 0;
}




void RemoteVstPlugin::idle()
{
	if( isProcessing() )
	{
		setShouldGiveIdle( true );
		return;
	}
	setProcessing( true );
	pluginDispatch( effEditIdle );
	setShouldGiveIdle( false );
	setProcessing( false );
	// We might have received a message whilst idling
	processUIThreadMessages();
}




void RemoteVstPlugin::processUIThreadMessages()
{
	setProcessing( true );
#ifdef NATIVE_LINUX_VST
	pthread_mutex_lock(&message_mutex);
#endif
	
	size_t size = m_messageList.size();
	
#ifdef NATIVE_LINUX_VST
	pthread_mutex_unlock(&message_mutex);
#endif

	while( size )
	{
		
#ifdef NATIVE_LINUX_VST
		pthread_mutex_lock(&message_mutex);
#endif

		message m = m_messageList.front();

#ifdef NATIVE_LINUX_VST
		pthread_mutex_unlock(&message_mutex);
#endif
		
		processMessage( m );
		
#ifdef NATIVE_LINUX_VST
		pthread_mutex_lock(&message_mutex);
#endif

		m_messageList.pop();
#ifdef NATIVE_LINUX_VST
		pthread_mutex_unlock(&message_mutex);
#endif
		if( shouldGiveIdle() )
		{
			pluginDispatch( effEditIdle );
			setShouldGiveIdle( false );
		}
#ifdef NATIVE_LINUX_VST
		pthread_mutex_lock(&message_mutex);
#endif

		size = m_messageList.size();
#ifdef NATIVE_LINUX_VST
		pthread_mutex_unlock(&message_mutex);
#endif
	}

	setProcessing( false );
}

#ifdef NATIVE_LINUX_VST
void RemoteVstPlugin::sendX11Idle()
{
	queueMessage(message( IdIdle ));
}
#endif


#ifndef NATIVE_LINUX_VST
DWORD WINAPI RemoteVstPlugin::processingThread( LPVOID _param )
#else
void * RemoteVstPlugin::processingThread(void * _param)
#endif
{
#ifndef NATIVE_LINUX_VST
	__processingThreadId = GetCurrentThreadId();
#else
	__processingThreadId = pthread_self();
#endif

	RemoteVstPlugin * _this = static_cast<RemoteVstPlugin *>( _param );

	RemotePluginClient::message m;
	while( ( m = _this->receiveMessage() ).id != IdQuit )
	{
		
		if( m.id == IdStartProcessing
			|| m.id == IdMidiEvent
			|| m.id == IdVstSetParameter
			|| m.id == IdVstSetTempo)
		{
			_this->processMessage( m );
		}
		else if( m.id == IdChangeSharedMemoryKey )
		{
			_this->processMessage( m );
			_this->setShmIsValid( true );
		}
		else
		{
#ifndef NATIVE_LINUX_VST
			PostMessage( __MessageHwnd,
					WM_USER,
					ProcessPluginMessage,
					(LPARAM) new message( m ) );
#else
		_this->queueMessage( m );
#endif
		}
	}

	// notify GUI thread about shutdown
#ifndef NATIVE_LINUX_VST
	PostMessage( __MessageHwnd, WM_USER, ClosePlugin, 0 );

	return 0;
#else
	if (m.id == IdQuit)
	{
		_this->queueMessage( m );
	}

	return nullptr;
#endif
}


bool RemoteVstPlugin::setupMessageWindow()
{
#ifndef NATIVE_LINUX_VST
	HMODULE hInst = GetModuleHandle( nullptr );
	if( hInst == nullptr )
	{
		__plugin->debugMessage( "setupMessageWindow(): can't get "
							"module handle\n" );
		return false;
	}

	__MessageHwnd = CreateWindowEx( 0, "LVSL", "dummy",
						0, 0, 0, 0, 0, nullptr, nullptr,
								hInst, nullptr );
	// install GUI update timer
	SetTimer( __MessageHwnd, 1000, 50, nullptr );
#endif

	return true;
}



#ifndef NATIVE_LINUX_VST
DWORD WINAPI RemoteVstPlugin::guiEventLoop()
{
	MSG msg;
	while( GetMessage( &msg, nullptr, 0, 0 ) > 0 )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return 0;
}
#else
void RemoteVstPlugin::guiEventLoop()
{
	struct timespec tim, tim2;
	tim.tv_sec = 0;
	tim.tv_nsec = 5000000;

	XEvent e;
	while(true)
	{
		//if (XQLength(m_display) > 0)
		if (m_display && XPending(m_display) > 0)
		{
			XNextEvent(m_display, &e);
		
			if (e.type == ClientMessage && e.xclient.data.l[0] == m_wmDeleteMessage)
			{
				hideEditor();
			}
		}

		// needed by ZynAddSubFX UI
		if (__plugin->isInitialized())
		{
			__plugin->idle();
		}
		
		if(__plugin->isInitialized() && !__plugin->isProcessing() )
		{
			__plugin->processUIThreadMessages();
		}
		
		nanosleep(&tim, &tim2);
		
		if (m_shouldQuit) 
		{
			__plugin->hideEditor();
			break;
		}
	}
}
#endif


#ifndef NATIVE_LINUX_VST
LRESULT CALLBACK RemoteVstPlugin::wndProc( HWND hwnd, UINT uMsg,
						WPARAM wParam, LPARAM lParam )
{
	if( uMsg == WM_TIMER && __plugin->isInitialized() )
	{
		// give plugin some idle-time for GUI-update
		__plugin->idle();
		return 0;
	}
	else if( uMsg == WM_USER )
	{
		switch( wParam )
		{
			case ProcessPluginMessage:
			{
				message * m = (message *) lParam;
				__plugin->queueMessage( *m );
				delete m;
				if( !__plugin->isProcessing() )
				{
					__plugin->processUIThreadMessages();
				}
				return 0;
			}

			case GiveIdle:
				__plugin->idle();
				return 0;

			case ClosePlugin:
				PostQuitMessage(0);
				return 0;

			default:
				break;
		}
	}
	else if( uMsg == WM_SYSCOMMAND && (wParam & 0xfff0) == SC_CLOSE )
	{
		__plugin->hideEditor();
		return 0;
	}

	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}
#endif


int main( int _argc, char * * _argv )
{
#ifdef SYNC_WITH_SHM_FIFO
	if( _argc < 4 )
#else
	if( _argc < 3 )
#endif
	{
		fprintf( stderr, "not enough arguments\n" );
		return -1;
	}

#ifndef LMMS_BUILD_WIN32
	const auto pollParentThread = PollParentThread{};
#endif

#ifndef NATIVE_LINUX_VST
	OleInitialize(nullptr);
#else
	XInitThreads();
#endif
#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_SCHED_H
	// try to set realtime-priority
	struct sched_param sparam;
	sparam.sched_priority = ( sched_get_priority_max( SCHED_FIFO ) +
				sched_get_priority_min( SCHED_FIFO ) ) / 2;
	sched_setscheduler( 0, SCHED_FIFO, &sparam );
#endif
#endif

#ifdef LMMS_BUILD_WIN32
	if( !SetPriorityClass( GetCurrentProcess(), HIGH_PRIORITY_CLASS ) )
	{
		printf( "Notice: could not set high priority.\n" );
	}
#endif

#ifndef NATIVE_LINUX_VST
	HMODULE hInst = GetModuleHandle( nullptr );
	if( hInst == nullptr )
	{
		return -1;
	}

	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = RemoteVstPlugin::wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon( nullptr, IDI_APPLICATION );
	wc.hCursor = LoadCursor( nullptr, IDC_ARROW );
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = "LVSL";

	if( !RegisterClass( &wc ) )
	{
		return -1;
	}

#endif
	{
	#ifdef SYNC_WITH_SHM_FIFO
		int embedMethodIndex = 3;
	#else
		int embedMethodIndex = 2;
	#endif
		std::string embedMethod = _argv[embedMethodIndex];

		if ( embedMethod == "none" )
		{
			cerr << "Starting detached." << endl;
			EMBED = EMBED_X11 = EMBED_WIN32 = HEADLESS = false;
		}
		else if ( embedMethod == "win32" )
		{
			cerr << "Starting using Win32-native embedding." << endl;
			EMBED = EMBED_WIN32 = true; EMBED_X11 = HEADLESS = false;
		}
		else if ( embedMethod == "qt" )
		{
			cerr << "Starting using Qt-native embedding." << endl;
			EMBED = true; EMBED_X11 = EMBED_WIN32 = HEADLESS = false;
		}
		else if ( embedMethod == "xembed" )
		{
			cerr << "Starting using X11Embed protocol." << endl;
			EMBED = EMBED_X11 = true; EMBED_WIN32 = HEADLESS = false;
		}
		else if ( embedMethod == "headless" )
		{
			cerr << "Starting without UI." << endl;
			HEADLESS = true; EMBED = EMBED_X11 = EMBED_WIN32 = false;
		}
		else
		{
			cerr << "Unknown embed method " << embedMethod << ". Starting detached instead." << endl;
			EMBED = EMBED_X11 = EMBED_WIN32 = HEADLESS = false;
		}
	}

#ifdef NATIVE_LINUX_VST
	if (EMBED)
	{
		cerr << "Native linux VST works only without embedding." << endl;
	}
#endif
	
	// constructor automatically will process messages until it receives
	// a IdVstLoadPlugin message and processes it
#ifdef SYNC_WITH_SHM_FIFO
	__plugin = new RemoteVstPlugin( _argv[1], _argv[2] );
#else
	__plugin = new RemoteVstPlugin( _argv[1] );
#endif

	if( __plugin->isInitialized() )
	{
		if( RemoteVstPlugin::setupMessageWindow() == false )
		{
			return -1;
		}
#ifndef NATIVE_LINUX_VST
		if( CreateThread( nullptr, 0, RemoteVstPlugin::processingThread,
						__plugin, 0, nullptr ) == nullptr )
#else
		int err = 0;
		err = pthread_create(&__processingThreadId, nullptr, &RemoteVstPlugin::processingThread, __plugin);
		if (err != 0)
#endif
		{
			__plugin->debugMessage( "could not create "
							"processingThread\n" );
			return -1;
		}

		__plugin->guiEventLoop();
#ifdef NATIVE_LINUX_VST
		pthread_join(__processingThreadId, nullptr);
#endif
	}

	delete __plugin;
#ifndef NATIVE_LINUX_VST
	OleUninitialize();
#endif
	return 0;
}

