/*
 * RemoteVstPlugin.cpp - LMMS VST Support Layer (RemotePlugin client)
 *
 * Copyright (c) 2005-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include "midi.h"
#include "communication.h"



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


	virtual void processMidiEvent( const midiEvent & _event,
							const f_cnt_t _offset );

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
			ret = m_plugin->dispatcher( m_plugin, cmd, param1,
							param2, p, f );
		}
		unlock();
		return ret;
	}

	std::string m_shortName;

	HINSTANCE m_libInst;

	AEffect * m_plugin;
	HWND m_window;
	Sint32 m_windowID;
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
	m_currentSamplePos( 0 )
{
	pthread_mutex_init( &m_pluginLock, NULL );

	__plugin = this;

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
	m_window = CreateWindowEx( 0, "LVSL", m_shortName.c_str(),
			       ( WS_OVERLAPPEDWINDOW | WS_THICKFRAME ) & ~WS_MAXIMIZEBOX,
			       0, 0, 10, 10, NULL, NULL, hInst, NULL );

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
	m_windowID = (Sint32) GetProp( m_window, "__wine_x11_whole_window" );
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

	m_plugin->user = this;

	if( m_plugin->magic != kEffectMagic )
	{
		char buf[256];
		sprintf( buf, "%s is not a VST plugin\n",
							_plugin_file.c_str() );
		debugMessage( buf );
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
		static char event_buf[sizeof( VstMidiEvent * ) *
						MIDI_EVENT_BUFFER_COUNT +
							sizeof( VstEvents )];
		static VstMidiEvent vme[MIDI_EVENT_BUFFER_COUNT];
		VstEvents * events = (VstEvents *) event_buf;
		events->reserved = 0;
		events->numEvents = m_midiEvents.size();
		int idx = 0;
		for( VstMidiEventList::iterator it = m_midiEvents.begin();
					it != m_midiEvents.end(); ++it, ++idx )
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

	m_currentSamplePos += bufferSize();

}




void RemoteVstPlugin::processMidiEvent( const midiEvent & _event,
							const f_cnt_t _offset )
{
	VstMidiEvent event;

	event.type = kVstMidiType;
	event.byteSize = 24;
	event.deltaFrames = _offset;
	event.flags = 0;
	event.detune = 0;
	event.noteLength = 0;
	event.noteOffset = 0;
	event.noteOffVelocity = 0;
	event.reserved1 = 0;
	event.reserved2 = 0;
	event.midiData[0] = _event.m_type + _event.m_channel;
	switch( _event.m_type )
	{
		case MidiPitchBend:
			event.midiData[1] = _event.m_data.m_param[0] & 0x7f;
			event.midiData[2] = _event.m_data.m_param[0] >> 7;
			break;
		// TODO: handle more special cases
		default:
			event.midiData[1] = _event.key();
			event.midiData[2] = _event.velocity();
			break;
	}
	event.midiData[3] = 0;
	m_midiEvents.push_back( event );
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




void RemoteVstPlugin::getParameterDump()
{
	VstParameterProperties vst_props;
	message m( IdVstParameterDump );
	m.addInt( m_plugin->numParams );
	for( int i = 0; i < m_plugin->numParams; ++i )
	{
		pluginDispatch( effGetParameterProperties, i, 0, &vst_props );
		m.addInt( i );
		m.addString( vst_props.shortLabel );
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

			memset( &_timeInfo, 0, sizeof( _timeInfo ) );

			_timeInfo.samplePos = __plugin->m_currentSamplePos;
			_timeInfo.sampleRate = __plugin->sampleRate();
			_timeInfo.flags = 0;
			_timeInfo.tempo = __plugin->m_bpm;
			_timeInfo.timeSigNumerator = 4;
			_timeInfo.timeSigDenominator = 4;
			_timeInfo.flags |= (/* kVstBarsValid|*/kVstTempoValid );
			_timeInfo.flags |= kVstTransportPlaying;
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
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
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
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif

	return 0;

}

