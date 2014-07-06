/*
 * RemoteVstPlugin.cpp - LMMS VST Support Layer (RemotePlugin client)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * Code partly taken from (X)FST:
 * 		Copyright (c) 2004 Paul Davis
 * 		Copyright (c) 2004 Torben Hohn
 *	 	Copyright (c) 2002 Kjetil S. Matheussen
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

#define BUILD_REMOTE_PLUGIN_CLIENT

#include "RemotePlugin.h"

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef LMMS_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef LMMS_BUILD_LINUX

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif

#include <wine/exception.h>

#endif

#include <windows.h>

#ifdef LMMS_BUILD_WIN32
#ifdef LMMS_BUILD_WIN64
#include "basename.c"
#else
#include <libgen.h>
#endif
#endif


#include <vector>
#include <string>


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

#include "VstSyncData.h"

#ifdef LMMS_BUILD_WIN32
#define USE_QT_SHMEM
#endif

#ifndef USE_QT_SHMEM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

static VstHostLanguages hlang = LanguageEnglish;


class RemoteVstPlugin;

RemoteVstPlugin * __plugin = NULL;

DWORD __GuiThreadID = 0;



class RemoteVstPlugin : public RemotePluginClient
{
public:
	RemoteVstPlugin( key_t _shm_in, key_t _shm_out );
	virtual ~RemoteVstPlugin();

	virtual bool processMessage( const message & _m );

	void init( const std::string & _plugin_file );
	void initEditor();

	virtual void process( const sampleFrame * _in, sampleFrame * _out );


	virtual void processMidiEvent( const MidiEvent& event, const f_cnt_t offset );

	// set given sample-rate for plugin
	virtual void updateSampleRate()
	{
		pluginDispatch( effSetSampleRate, 0, 0,
						NULL, (float) sampleRate() );
	}

	// set given buffer-size for plugin
	virtual void updateBufferSize()
	{
		pluginDispatch( effSetBlockSize, 0, bufferSize() );
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

	// send name of current program back to host
	void sendCurrentProgramName();

	// do a complete parameter-dump and post it
	void getParameterDump();

	// read parameter-dump and set it for plugin
	void setParameterDump( const message & _m );

	// post properties of specified parameter
	void getParameterProperties( const int _idx );

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
	void updateInOutCount();

	inline void lock()
	{
		pthread_mutex_lock( &m_pluginLock );
	}

	inline void unlock()
	{
		pthread_mutex_unlock( &m_pluginLock );
	}

	static DWORD WINAPI processingThread( LPVOID _param );
	static DWORD WINAPI guiEventLoop( LPVOID _param );


private:
	enum GuiThreadMessages
	{
		None,
		ProcessPluginMessage,
		GiveIdle,
		ClosePlugin
	} ;

	// callback used by plugin for being able to communicate with it's host
	static intptr_t hostCallback( AEffect * _effect, int32_t _opcode,
					int32_t _index, intptr_t _value,
					void * _ptr, float _opt );


	bool load( const std::string & _plugin_file );

	// thread-safe dispatching of plugin
	int pluginDispatch( int cmd, int param1 = 0, int param2 = 0,
					void * p = NULL, float f = 0 )
	{
		int ret = 0;
		lock();
		if( m_plugin )
		{
			ret = m_plugin->dispatcher( m_plugin, cmd, param1, param2, p, f );
		}
		unlock();
		return ret;
	}

	// thread-safe dispatching of plugin
	int pluginDispatchNoLocking( int cmd, int param1 = 0, int param2 = 0, void * p = NULL, float f = 0 )
	{
		if( m_plugin )
		{
			return m_plugin->dispatcher( m_plugin, cmd, param1, param2, p, f );
		}
		return 0;
	}


	std::string m_shortName;

	HINSTANCE m_libInst;

	AEffect * m_plugin;
	HWND m_window;
	intptr_t m_windowID;
	int m_windowWidth;
	int m_windowHeight;

	bool m_initialized;

	pthread_mutex_t m_pluginLock;


	float * * m_inputs;
	float * * m_outputs;

	typedef std::vector<VstMidiEvent> VstMidiEventList;
	VstMidiEventList m_midiEvents;

	bpm_t m_bpm;
	double m_currentSamplePos;
	int m_currentProgram;

	// host to plugin synchronisation data structure
	struct in
	{
		float lastppqPos;
		float m_Timestamp;
	} ;

	in * m_in;

	int m_shmID;
	VstSyncData* m_vstSyncData;

} ;




RemoteVstPlugin::RemoteVstPlugin( key_t _shm_in, key_t _shm_out ) :
	RemotePluginClient( _shm_in, _shm_out ),
	m_shortName( "" ),
	m_libInst( NULL ),
	m_plugin( NULL ),
	m_window( NULL ),
	m_windowID( 0 ),
	m_windowWidth( 0 ),
	m_windowHeight( 0 ),
	m_initialized( false ),
	m_pluginLock(),
	m_inputs( NULL ),
	m_outputs( NULL ),
	m_midiEvents(),
	m_bpm( 0 ),
	m_currentSamplePos( 0 ),
	m_currentProgram( -1 ),
	m_in( NULL ),
	m_shmID( -1 ),
	m_vstSyncData( NULL )

{
	pthread_mutex_init( &m_pluginLock, NULL );

	__plugin = this;

#ifndef USE_QT_SHMEM
	key_t key;
	if( ( key = ftok( VST_SNC_SHM_KEY_FILE, 'R' ) ) == -1 )
	{
		perror( "RemoteVstPlugin.cpp::ftok" );
	}
	else
	{	// connect to shared memory segment
		if( ( m_shmID = shmget( key, 0, 0 ) ) == -1 )
		{
			perror( "RemoteVstPlugin.cpp::shmget" );
		}
		else
		{	// attach segment
			m_vstSyncData = (VstSyncData *)shmat(m_shmID, 0, 0);
			if( m_vstSyncData == (VstSyncData *)( -1 ) )
			{
				perror( "RemoteVstPlugin.cpp::shmat" );
			}
		}
	}
#else
	m_vstSyncData = RemotePluginClient::getQtVSTshm();
#endif
	if( m_vstSyncData == NULL )
	{
		fprintf(stderr, "RemoteVstPlugin.cpp: "
			"Failed to initialize shared memory for VST synchronization.\n"
			" (VST-host synchronization will be disabled)\n");
		m_vstSyncData = (VstSyncData*) malloc( sizeof( VstSyncData ) );
		m_vstSyncData->isPlaying = true;
		m_vstSyncData->timeSigNumer = 4;
		m_vstSyncData->timeSigDenom = 4;
		m_vstSyncData->ppqPos = 0;
		m_vstSyncData->isCycle = false;
		m_vstSyncData->hasSHM = false;
		m_vstSyncData->m_sampleRate = sampleRate();
	}

	m_in = ( in* ) new char[ sizeof( in ) ];
	m_in->lastppqPos = 0;
	m_in->m_Timestamp = -1;

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
	if( m_window != NULL )
	{
		pluginDispatch( effEditClose );
#ifdef LMMS_BUILD_LINUX
		CloseWindow( m_window );
#endif
		m_window = NULL;
	}
	pluginDispatch( effMainsChanged, 0, 0 );
	pluginDispatch( effClose );
#ifndef USE_QT_SHMEM
	// detach shared memory segment
	if( shmdt( m_vstSyncData ) == -1)
	{
		if( __plugin->m_vstSyncData->hasSHM )
		{
			perror( "~RemoteVstPlugin::shmdt" );
		}
		if( m_vstSyncData != NULL )
		{
			delete m_vstSyncData;
			m_vstSyncData = NULL;
		}
	}
#endif

	if( m_libInst != NULL )
	{
		FreeLibrary( m_libInst );
		m_libInst = NULL;
	}

	delete[] m_inputs;
	delete[] m_outputs;

	pthread_mutex_destroy( &m_pluginLock );
}




bool RemoteVstPlugin::processMessage( const message & _m )
{
	switch( _m.id )
	{
		case IdVstLoadPlugin:
			init( _m.getString() );
			break;

#ifdef LMMS_BUILD_WIN32
		case IdVstPluginWindowInformation:
		{
			HWND top = FindWindowEx( NULL, NULL, NULL,
						_m.getString().c_str() );
			m_window = FindWindowEx( top, NULL, NULL, NULL );
			break;
		}
#endif

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

		case IdVstGetParameterProperties:
			getParameterProperties( _m.getInt() );
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
			lock();
			m_plugin->setParameter( m_plugin, _m.getInt( 0 ), _m.getFloat( 1 ) );
			unlock();
			//sendMessage( IdVstSetParameter );
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

	// some plugins have to set samplerate during init
	if( m_vstSyncData->hasSHM )
	{
		updateSampleRate();
	}

	/* set program to zero */
	/* i comment this out because it breaks dfx Geometer
	 * looks like we cant set programs for it
	 *
	pluginDispatch( effSetProgram, 0, 0 ); */
	// request rate and blocksize

	pluginDispatch( effMainsChanged, 0, 1 );

	debugMessage( "creating editor\n" );
	initEditor();
	debugMessage( "editor successfully created\n" );


	// now post some information about our plugin
	sendMessage( message( IdVstPluginWindowID ).addInt( m_windowID ) );

	sendMessage( message( IdVstPluginEditorGeometry ).
						addInt( m_windowWidth ).
						addInt( m_windowHeight ) );

	sendMessage( message( IdVstPluginName ).addString( pluginName() ) );
	sendMessage( message( IdVstPluginVersion ).addInt( pluginVersion() ) );
	sendMessage( message( IdVstPluginVendorString ).
					addString( pluginVendorString() ) );
	sendMessage( message( IdVstPluginProductString ).
					addString( pluginProductString() ) );
	sendMessage( message( IdVstParameterCount ).
					addInt( m_plugin->numParams ) );

	sendMessage( IdInitDone );

	m_initialized = true;
}




void RemoteVstPlugin::initEditor()
{
	if( !( m_plugin->flags & effFlagsHasEditor ) )
	{
		return;
	}


	HMODULE hInst = GetModuleHandle( NULL );
	if( hInst == NULL )
	{
		debugMessage( "initEditor(): can't get module handle\n" );
		return;
	}


	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DefWindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "LVSL";

	if( !RegisterClass( &wc ) )
	{
		return;
	}

#ifdef LMMS_BUILD_LINUX
	//m_window = CreateWindowEx( 0, "LVSL", m_shortName.c_str(),
	//		       ( WS_OVERLAPPEDWINDOW | WS_THICKFRAME ) & ~WS_MAXIMIZEBOX,
	//		       0, 0, 10, 10, NULL, NULL, hInst, NULL );

	m_window = CreateWindowEx( 0 , "LVSL", m_shortName.c_str(),
	   WS_POPUP | WS_SYSMENU | WS_BORDER , 0, 0, 10, 10, NULL, NULL, hInst, NULL);
#else
	m_windowID = 1;	// arbitrary value on win32 to signal
			// vstPlugin-class that we have an editor

	m_window = CreateWindowEx( 0, "LVSL", m_shortName.c_str(),
					WS_CHILD, 0, 0, 10, 10,
					m_window, NULL, hInst, NULL );
#endif
	if( m_window == NULL )
	{
		debugMessage( "initEditor(): cannot create editor window\n" );
		return;
	}


	pluginDispatch( effEditOpen, 0, 0, m_window );

	ERect * er;
	pluginDispatch( effEditGetRect, 0, 0, &er );

	m_windowWidth = er->right - er->left;
	m_windowHeight = er->bottom - er->top;

	SetWindowPos( m_window, 0, 0, 0, m_windowWidth + 8,
			m_windowHeight + 26, SWP_NOACTIVATE |
						SWP_NOMOVE | SWP_NOZORDER );
	pluginDispatch( effEditTop );

	ShowWindow( m_window, SW_SHOWNORMAL );
	UpdateWindow( m_window );

#ifdef LMMS_BUILD_LINUX
	m_windowID = (intptr_t) GetProp( m_window, "__wine_x11_whole_window" );
#endif
}




bool RemoteVstPlugin::load( const std::string & _plugin_file )
{
	if( ( m_libInst = LoadLibrary( _plugin_file.c_str() ) ) == NULL )
	{
		// give VstPlugin class a chance to start 32 bit version of RemoteVstPlugin
		if( GetLastError() == ERROR_BAD_EXE_FORMAT )
		{
			sendMessage( IdVstBadDllFormat );
		}
		return false;
	}

	char * tmp = strdup( _plugin_file.c_str() );
	m_shortName = basename( tmp );
	free( tmp );

	typedef AEffect * ( __stdcall * mainEntryPointer )
						( audioMasterCallback );
	mainEntryPointer mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "VSTPluginMain" );
	if( mainEntry == NULL )
	{
		mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "VstPluginMain" );
	}
	if( mainEntry == NULL )
	{
		mainEntry = (mainEntryPointer)
				GetProcAddress( m_libInst, "main" );
	}
	if( mainEntry == NULL )
	{
		debugMessage( "could not find entry point\n" );
		return false;
	}

	m_plugin = mainEntry( hostCallback );
	if( m_plugin == NULL )
	{
		debugMessage( "mainEntry prodecure returned NULL\n" );
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

	for( int i = 0; i < inputCount(); ++i )
	{
		m_inputs[i] = &((float *) _in)[i * bufferSize()];
	}

	for( int i = 0; i < outputCount(); ++i )
	{
		m_outputs[i] = &((float *) _out)[i * bufferSize()];
		memset( m_outputs[i], 0, bufferSize() * sizeof( float ) );
	}

	lock();

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

	unlock();

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



void RemoteVstPlugin::sendCurrentProgramName()
{
	char presName[64];
	sprintf( presName, "%d/%d: %s", pluginDispatch( effGetProgram ) + 1, m_plugin->numPrograms, programName() );

	sendMessage( message( IdVstCurrentProgramName ).addString( presName ) );
}



void RemoteVstPlugin::getParameterDump()
{
	lock();

	message m( IdVstParameterDump );
	m.addInt( m_plugin->numParams );

	for( int i = 0; i < m_plugin->numParams; ++i )
	{
		char paramName[32];
		memset( paramName, 0, sizeof( paramName ) );
		pluginDispatchNoLocking( effGetParamName, i, 0, paramName );
		paramName[sizeof(paramName)-1] = 0;

		m.addInt( i );
		m.addString( paramName );
		m.addFloat( m_plugin->getParameter( m_plugin, i ) );
	}

	unlock();

	sendMessage( m );
}




void RemoteVstPlugin::setParameterDump( const message & _m )
{
	lock();
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
	unlock();
}




void RemoteVstPlugin::getParameterProperties( const int _idx )
{
	VstParameterProperties p;
	pluginDispatch( effGetParameterProperties, _idx, 0, &p );
	message m( IdVstParameterProperties );
	m.addString( p.label );
	m.addString( p.shortLabel );
	m.addString(
#if kVstVersion > 2
			p.categoryLabel
#else
			""
#endif
					);
	m.addFloat( p.minInteger );
	m.addFloat( p.maxInteger );
	m.addFloat( ( p.flags & kVstParameterUsesFloatStep ) ?
						p.stepFloat : p.stepInteger );
	m.addInt(
#if kVstVersion > 2
			p.category
#else
			0
#endif
				);
	sendMessage( m );
}




void RemoteVstPlugin::saveChunkToFile( const std::string & _file )
{
	if( m_plugin->flags & 32 )
	{
		void * chunk = NULL;
		const int len = pluginDispatch( 23, 0, 0, &chunk );
		if( len > 0 )
		{
			int fd = open( _file.c_str(), O_WRONLY | O_BINARY );
			write( fd, chunk, len );
			close( fd );
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
	char* data = NULL;
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
			lock();
			for ( int i = 0; i < m_plugin->numParams; i++ )
			{
				float value = m_plugin->getParameter( m_plugin, i );
				unsigned int * pValue = ( unsigned int * ) &value;
				toUIntArray[ i ] = endian_swap( *pValue );
			}
			unlock();
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

	FILE * stream = fopen( _file.c_str(), "w" );
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
			lock();
			for ( int i = 0; i < m_plugin->numParams; i++ )
			{
				value = m_plugin->getParameter( m_plugin, i );
				pValue = ( unsigned int * ) &value;
				toUIntArray[ i ] = endian_swap( *pValue );
			}
			unlock();
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
	void * chunk = NULL;
	unsigned int * pLen = new unsigned int[ 1 ];
	unsigned int len = 0;
	sBank * pBank = (sBank*) new char[ sizeof( sBank ) ];
	FILE * stream = fopen( _file.c_str(), "r" );
	fread ( pBank, 1, 56, stream );
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
			fread (pLen, 1, 4, stream);
			chunk = new char[len = endian_swap(*pLen)];
		} else chunk = new char[len = sizeof(float)*pBank->numPrograms];
		fread (chunk, len, 1, stream);
		fclose( stream );
	}

	if(_file.substr(_file.find_last_of(".") + 1) == "fxp") {
		pBank->prgName[23] = 0;
		pluginDispatch( 4, 0, 0, pBank->prgName );
		if(pBank->fxMagic != 0x6B437846)
			pluginDispatch( 24, 1, len, chunk );
		else
		{
			lock();
			unsigned int* toUIntArray = reinterpret_cast<unsigned int*>( chunk );
			for (int i = 0; i < pBank->numPrograms; i++ )
			{
				toUInt = endian_swap( toUIntArray[ i ] );
				pFloat = ( float* ) &toUInt;
				m_plugin->setParameter( m_plugin, i, *pFloat );
			}
			unlock();
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
			lock();
			for (int i =0; i < numPrograms; i++) {
				fread (pBank, 1, 56, stream);
				fread (chunk, len, 1, stream);
				pluginDispatch( effSetProgram, 0, i );
				pBank->prgName[23] = 0;
				pluginDispatch( 4, 0, 0, pBank->prgName );
				for (int j = 0; j < m_plugin->numParams; j++ ) {
					toUInt = endian_swap( toUIntArray[ j ] );
					pFloat = ( float* ) &toUInt;
					m_plugin->setParameter( m_plugin, j, *pFloat );
				}
			}
			unlock();
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
	char * buf = NULL;

	void * chunk = NULL;
	// various plugins need this in order to not crash when setting
	// chunk (also we let the plugin allocate "safe" memory this way)
	const int actualLen = pluginDispatch( 23, 0, 0, &chunk );

	// allocated buffer big enough?
	if( _len > actualLen )
	{
		// no, then manually allocate a buffer
		buf = new char[_len];
		chunk = buf;
	}

	const int fd = open( _file.c_str(), O_RDONLY | O_BINARY );
	read( fd, chunk, _len );
	close( fd );
	pluginDispatch( 24, 0, _len, chunk );

	delete[] buf;
}




void RemoteVstPlugin::updateInOutCount()
{
	delete[] m_inputs;
	delete[] m_outputs;

	m_inputs = NULL;
	m_outputs = NULL;

	setInputCount( inputCount() );
	setOutputCount( outputCount() );

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
	if( __plugin && __plugin->m_plugin == NULL )
	{
		__plugin->m_plugin = _effect;
	}

	switch( _opcode )
	{
		case audioMasterAutomate:
			SHOW_CALLBACK( "amc: audioMasterAutomate\n" );
			// index, value, returns 0
			_effect->setParameter( _effect, _index, _opt );
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
			PostThreadMessage( __GuiThreadID,
						WM_USER, GiveIdle, 0 );
			return 0;

		case audioMasterPinConnected:
			SHOW_CALLBACK( "amc: audioMasterPinConnected\n" );
			// inquire if an input or output is beeing connected;
			// index enumerates input or output counting from zero:
			// value is 0 for input and != 0 otherwise. note: the
			// return value is 0 for <true> such that older versions
			// will always return true.
			return 1;

		case audioMasterGetTime:
			SHOW_CALLBACK( "amc: audioMasterGetTime\n" );
			// returns const VstTimeInfo* (or 0 if not supported)
			// <value> should contain a mask indicating which
			// fields are required (see valid masks above), as some
			// items may require extensive conversions

			// Shared memory was initialised? - see song.cpp
			//assert( __plugin->m_vstSyncData != NULL );

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
				_timeInfo.flags |= kVstTransportChanged;
				__plugin->m_in->lastppqPos = __plugin->m_vstSyncData->ppqPos;
				__plugin->m_in->m_Timestamp = __plugin->m_vstSyncData->ppqPos;
			}
			else if( __plugin->m_vstSyncData->isPlaying )
			{
				__plugin->m_in->lastppqPos += (
							__plugin->m_vstSyncData->hasSHM ?
							__plugin->m_vstSyncData->m_bpm :
							__plugin->m_bpm ) / (float)10340;
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

#ifdef LMMS_BUILD_WIN64
			return (long long) &_timeInfo;
#else
			return (long) &_timeInfo;
#endif

		case audioMasterProcessEvents:
			SHOW_CALLBACK( "amc: audioMasterProcessEvents\n" );
			// VstEvents* in <ptr>
			return 0;

		case audioMasterIOChanged:
			__plugin->updateInOutCount();
			SHOW_CALLBACK( "amc: audioMasterIOChanged\n" );
			// numInputs and/or numOutputs has changed
			return 0;

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
			SHOW_CALLBACK( "amc: audioMasterSizeWindow\n" );
			if( __plugin->m_window == 0 )
			{
				return 0;
			}
			__plugin->m_windowWidth = _index;
			__plugin->m_windowHeight = _value;
			SetWindowPos( __plugin->m_window, 0, 0, 0,
					_index + 8, _value + 26,
					SWP_NOACTIVATE | SWP_NOMOVE |
					SWP_NOOWNERZORDER | SWP_NOZORDER );
			__plugin->sendMessage(
				message( IdVstPluginEditorGeometry ).
					addInt( __plugin->m_windowWidth ).
					addInt( __plugin->m_windowHeight ) );
			return 1;

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
			PostThreadMessage( __GuiThreadID,
						WM_USER, GiveIdle, 0 );
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




DWORD WINAPI RemoteVstPlugin::processingThread( LPVOID _param )
{
	RemoteVstPlugin * _this = static_cast<RemoteVstPlugin *>( _param );

	RemotePluginClient::message m;
	while( ( m = _this->receiveMessage() ).id != IdQuit )
        {
		if( m.id == IdStartProcessing || m.id == IdMidiEvent )
		{
			_this->processMessage( m );
		}
		else
		{
			PostThreadMessage( __GuiThreadID,
					WM_USER,
					ProcessPluginMessage,
					(LPARAM) new message( m ) );
		}
	}

	// notify GUI thread about shutdown
	PostThreadMessage( __GuiThreadID, WM_USER, ClosePlugin, 0 );

	return 0;
}




DWORD WINAPI RemoteVstPlugin::guiEventLoop( LPVOID _param )
{
	RemoteVstPlugin * _this = static_cast<RemoteVstPlugin *>( _param );

	HMODULE hInst = GetModuleHandle( NULL );
	if( hInst == NULL )
	{
		_this->debugMessage( "guiEventLoop(): can't get "
							"module handle\n" );
		return -1;
	}

	HWND timerWindow = CreateWindowEx( 0, "LVSL", "dummy",
						0, 0, 0, 0, 0, NULL, NULL,
								hInst, NULL );
	// install GUI update timer
	SetTimer( timerWindow, 1000, 50, NULL );

	MSG msg;

	bool quit = false;
	while( quit == false && GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );

		if( msg.message == WM_TIMER && _this->isInitialized() )
		{
			// give plugin some idle-time for GUI-update
			_this->pluginDispatch( effEditIdle );
		}
		else if( msg.message == WM_USER )
		{
			switch( msg.wParam )
			{
				case ProcessPluginMessage:
				{
					message * m = (message *) msg.lParam;
					_this->processMessage( *m );
					delete m;
					break;
				}

				case GiveIdle:
					_this->pluginDispatch( effEditIdle );
					break;

				case ClosePlugin:
					quit = true;
					break;

				default:
					break;
			}
		}
	}

	return 0;
}




int main( int _argc, char * * _argv )
{
	if( _argc < 3 )
	{
		fprintf( stderr, "not enough arguments\n" );
		return -1;
	}

#ifdef LMMS_BUILD_WIN32
#ifndef __WINPTHREADS_VERSION
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif
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

	// constructor automatically will process messages until it receives
	// a IdVstLoadPlugin message and processes it
	__plugin = new RemoteVstPlugin( atoi( _argv[1] ), atoi( _argv[2] ) );

	if( __plugin->isInitialized() )
	{
		__GuiThreadID = GetCurrentThreadId();
		if( CreateThread( NULL, 0, RemoteVstPlugin::processingThread,
						__plugin, 0, NULL ) == NULL )
		{
			__plugin->debugMessage( "could not create "
							"processingThread\n" );
			return -1;
		}
		RemoteVstPlugin::guiEventLoop( __plugin );
	}


	delete __plugin;


#ifdef LMMS_BUILD_WIN32
#ifndef __WINPTHREADS_VERSION
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif
#endif

	return 0;

}

