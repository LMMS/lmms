/*
 * remote_vst_plugin.cpp - LMMS VST Support Layer (remotePlugin client)
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "remote_plugin.h"

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef LMMS_BUILD_LINUX

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif

#include <wine/exception.h>

#endif

#include <windows.h>

#ifdef LMMS_BUILD_WIN32
#include <libgen.h>

#endif


#include <list>
#include <string>


#include <aeffectx.h>

#if kVstVersion < 2400

#define OLD_VST_SDK

#define VstInt32 long int
#define VstIntPtr long int

struct ERect
{
    short top;
    short left;
    short bottom;
    short right;
} ;

#endif


#include "types.h"
#include "midi.h"
#include "communication.h"



static VstHostLanguages hlang = LanguageEnglish;


class remoteVstPlugin;

remoteVstPlugin * __plugin = NULL;




class remoteVstPlugin : public remotePluginClient
{
public:
	remoteVstPlugin( key_t _shm_in, key_t _shm_out );
	virtual ~remoteVstPlugin();

	virtual bool processMessage( const message & _m );

	void init( const std::string & _plugin_file );

	virtual bool process( const sampleFrame * _in, sampleFrame * _out );


	virtual void processMidiEvent( const midiEvent & _event,
							const f_cnt_t _offset );

	// set given sample-rate for plugin
	void updateSampleRate( void )
	{
		if( m_plugin )
		{
			m_plugin->dispatcher( m_plugin, effSetSampleRate, 0, 0,
						NULL, (float) sampleRate() );
		}
	}

	// set given buffer-size for plugin
	void updateBufferSize( void )
	{
		if( m_plugin )
		{
			m_plugin->dispatcher( m_plugin, effSetBlockSize, 0,
						bufferSize(), NULL, 0.0f );
		}
	}



	// set given tempo
	void setBPM( const bpm_t _bpm )
	{
		m_bpm = _bpm;
	}

	// determine VST-version the plugin uses
	int pluginVersion( void ) const
	{
		return( m_plugin->dispatcher( m_plugin,
				effGetVendorVersion, 0, 0, NULL, 0.0f ) );
	}

	// determine name of plugin
	const char * pluginName( void ) const;

	// determine vendor of plugin
	const char * pluginVendorString( void ) const;

	// determine product-string of plugin
	const char * pluginProductString( void ) const;

	// do a complete parameter-dump and post it
	void getParameterDump( void );

	// read parameter-dump and set it for plugin
	void setParameterDump( const message & _m );

	// post properties of specified parameter
	void getParameterProperties( const int _idx );

	// number of inputs
	virtual int inputCount( void ) const
	{
		return( m_plugin->numInputs );
	}

	// number of outputs
	virtual int outputCount( void ) const
	{
		return( m_plugin->numOutputs );
	}

	// has to be called as soon as input- or output-count changes
	void updateInOutCount( void );


private:
	enum GuiThreadMessages
	{
		None, ClosePlugin
	} ;

	// callback used by plugin for being able to communicate with it's host
	static VstIntPtr hostCallback( AEffect * _effect, VstInt32 _opcode,
					VstInt32 _index, VstIntPtr _value,
					void * _ptr, float _opt );

	static DWORD WINAPI guiEventLoop( LPVOID _param );


	bool load( const std::string & _plugin_file );


	std::string m_shortName;

	HINSTANCE m_libInst;

	AEffect * m_plugin;
	HWND m_window;
	Sint32 m_windowID;
	int m_windowWidth;
	int m_windowHeight;

	pthread_mutex_t m_lock;
	pthread_cond_t m_windowStatusChange;
	DWORD m_guiThreadID;


	float * * m_inputs;
	float * * m_outputs;

	std::list<VstMidiEvent> m_midiEvents;

	bpm_t m_bpm;
	double m_currentSamplePos;

} ;




remoteVstPlugin::remoteVstPlugin( key_t _shm_in, key_t _shm_out ) :
	remotePluginClient( _shm_in, _shm_out ),
	m_shortName( "" ),
	m_libInst( NULL ),
	m_plugin( NULL ),
	m_window( NULL ),
	m_windowID( 0 ),
	m_windowWidth( 0 ),
	m_windowHeight( 0 ),
	m_lock(),
	m_windowStatusChange(),
	m_guiThreadID( 0 ),
	m_inputs( NULL ),
	m_outputs( NULL ),
	m_midiEvents(),
	m_bpm( 0 ),
	m_currentSamplePos( 0 )
{
	pthread_mutex_init( &m_lock, NULL );
	pthread_cond_init( &m_windowStatusChange, NULL );
}



remoteVstPlugin::~remoteVstPlugin()
{
	if( m_window != NULL )
	{
		// notify GUI-thread
		if( !PostThreadMessageA( m_guiThreadID, WM_USER,
							ClosePlugin, 0 ) )
		{
			//lvsMessage( "could not post message to gui thread" );
		}
		pthread_mutex_lock( &m_lock );
		pthread_cond_wait( &m_windowStatusChange, &m_lock );
		m_plugin->dispatcher( m_plugin, effEditClose, 0, 0, NULL, 0.0 );
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

	pthread_mutex_destroy( &m_lock );
	pthread_cond_destroy( &m_windowStatusChange );
}




bool remoteVstPlugin::processMessage( const message & _m )
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

		default:
			return remotePluginClient::processMessage( _m );
	}
	return true;
}



void remoteVstPlugin::init( const std::string & _plugin_file )
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
	m_plugin->dispatcher( m_plugin, effSetProgram, 0, 0, NULL, 0.0f); */
	// request rate and blocksize

	m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 1, NULL, 0.0f );


	if( CreateThread( NULL, 0, guiEventLoop, this, 0, NULL ) == NULL )
	{
		fprintf( stderr, "could not create GUI-thread\n" );
		return;
	}
	pthread_mutex_lock( &m_lock );
	pthread_cond_wait( &m_windowStatusChange, &m_lock );


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
}




bool remoteVstPlugin::load( const std::string & _plugin_file )
{
	if( ( m_libInst = LoadLibraryA( _plugin_file.c_str() ) ) ==
								NULL )
	{
		return( false );
	}

	char * tmp = strdup( _plugin_file.c_str() );
	m_shortName = basename( tmp );
	free( tmp );

	typedef AEffect * ( __stdcall * mainEntry )( audioMasterCallback );
	mainEntry main_entry = (mainEntry) GetProcAddress( m_libInst,
								"main" );
	if( main_entry == NULL )
	{
		return( false );
	}

	m_plugin = main_entry( hostCallback );
	if( m_plugin == NULL )
	{
		return( false );
	}

	m_plugin->user = this;

	if( m_plugin->magic != kEffectMagic )
	{
		fprintf( stderr, "%s is not a VST plugin\n",
							_plugin_file.c_str() );
	}

	m_plugin->dispatcher( m_plugin, effOpen, 0, 0, 0, 0 );

	return( true );
}



bool remoteVstPlugin::process( const sampleFrame * _in, sampleFrame * _out )
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
		for( std::list<VstMidiEvent>::iterator it =
							m_midiEvents.begin();
					it != m_midiEvents.end(); ++it, ++idx )
		{
			memcpy( &vme[idx], &*it, sizeof( VstMidiEvent ) );
			events->events[idx] = (VstEvent *) &vme[idx];
		}

		m_midiEvents.clear();
		m_plugin->dispatcher( m_plugin, effProcessEvents, 0, 0, events,
									0.0f );
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




void remoteVstPlugin::processMidiEvent( const midiEvent & _event,
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




const char * remoteVstPlugin::pluginName( void ) const
{
	static char buf[32];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetEffectName, 0, 0, buf, 0.0f );
	buf[31] = 0;
	return( buf );
}




const char * remoteVstPlugin::pluginVendorString( void ) const
{
	static char buf[64];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetVendorString, 0, 0, buf, 0.0f );
	buf[63] = 0;
	return( buf );
}




const char * remoteVstPlugin::pluginProductString( void ) const
{
	static char buf[64];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetProductString, 0, 0, buf, 0.0f );
	buf[63] = 0;
	return( buf );
}




void remoteVstPlugin::getParameterDump( void )
{
	VstParameterProperties vst_props;
	message m( IdVstParameterDump );
	m.addInt( m_plugin->numParams );
	for( int i = 0; i < m_plugin->numParams; ++i )
	{
		m_plugin->dispatcher( m_plugin, effGetParameterProperties, i,
							0, &vst_props, 0.0f );
		m.addInt( i );
		m.addString( vst_props.shortLabel );
		m.addFloat( m_plugin->getParameter( m_plugin, i ) );
	}
	sendMessage( m );
}




void remoteVstPlugin::setParameterDump( const message & _m )
{
	const int n = _m.getInt( 0 );
	const int params = ( n > m_plugin->numParams ) ?
					m_plugin->numParams : n;
	int p = 0;
	for( int i = 0; i < params; ++i )
	{
		vstParameterDumpItem item;
		item.index = _m.getInt( ++p );
		item.shortLabel = _m.getString( ++p );
		item.value = _m.getFloat( ++p );
		m_plugin->setParameter( m_plugin, item.index, item.value );
	}
}




void remoteVstPlugin::getParameterProperties( const int _idx )
{
	VstParameterProperties p;
	m_plugin->dispatcher( m_plugin, effGetParameterProperties, _idx, 0,
								&p, 0.0f );
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




void remoteVstPlugin::updateInOutCount( void )
{
	delete[] m_inputs;
	delete[] m_outputs;

	setInputCount( inputCount() );
	setOutputCount( outputCount() );

	if( inputCount() > 0 )
	{
		m_inputs = new float * [inputCount()];
	}

	if( outputCount() > 0 )
	{
		m_outputs = new float * [outputCount()];
	}
}



#define DEBUG_CALLBACKS
#ifdef DEBUG_CALLBACKS
#define SHOW_CALLBACK printf
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
VstIntPtr remoteVstPlugin::hostCallback( AEffect * _effect, VstInt32 _opcode,
					VstInt32 _index, VstIntPtr _value,
						void * _ptr, float _opt )
{
	static VstTimeInfo _timeInfo;
	//SHOW_CALLBACK( "host-callback, opcode = %d\n", (int) _opcode );
	switch( _opcode )
	{
		case audioMasterAutomate:
			//SHOW_CALLBACK( "amc: audioMasterAutomate\n" );
			// index, value, returns 0
			_effect->setParameter( _effect, _index, _opt );
			return( 0 );

		case audioMasterVersion:
			//SHOW_CALLBACK( "amc: audioMasterVersion\n" );
			return( 2300 );

		case audioMasterCurrentId:	
			SHOW_CALLBACK( "amc: audioMasterCurrentId\n" );
			// returns the unique id of a plug that's currently
			// loading
			return( 0 );
		
		case audioMasterIdle:
			//SHOW_CALLBACK ("amc: audioMasterIdle\n" );
			// call application idle routine (this will
			// call effEditIdle for all open editors too) 
			_effect->dispatcher( _effect, effEditIdle, 0, 0, NULL,
									0.0f );
			return( 0 );

		case audioMasterPinConnected:		
			//SHOW_CALLBACK( "amc: audioMasterPinConnected\n" );
			// inquire if an input or output is beeing connected;
			// index enumerates input or output counting from zero:
			// value is 0 for input and != 0 otherwise. note: the
			// return value is 0 for <true> such that older versions
			// will always return true.
			return( 1 );

		case audioMasterGetTime:
			//SHOW_CALLBACK( "amc: audioMasterGetTime\n" );
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

			return( (long) &_timeInfo );

		case audioMasterProcessEvents:
			//SHOW_CALLBACK( "amc: audioMasterProcessEvents\n" );
			// VstEvents* in <ptr>
			return( 0 );

		case audioMasterIOChanged:
			__plugin->updateInOutCount();
			//SHOW_CALLBACK( "amc: audioMasterIOChanged\n" );
			// numInputs and/or numOutputs has changed
			return( 0 );

#ifdef OLD_VST_SDK
		case audioMasterWantMidi:
			//SHOW_CALLBACK( "amc: audioMasterWantMidi\n" );
			// <value> is a filter which is currently ignored
			return( 1 );

		case audioMasterSetTime:
			SHOW_CALLBACK( "amc: audioMasterSetTime\n" );
			// VstTimenfo* in <ptr>, filter in <value>, not
			// supported
			return( 0 );

		case audioMasterTempoAt:
			//SHOW_CALLBACK( "amc: audioMasterTempoAt\n" );
			return( __plugin->m_bpm * 10000 );

		case audioMasterGetNumAutomatableParameters:
			SHOW_CALLBACK( "amc: audioMasterGetNumAutomatable"
							"Parameters\n" );
			return( 5000 );

		case audioMasterGetParameterQuantization:	
			SHOW_CALLBACK( "amc: audioMasterGetParameter\n"
							"Quantization\n" );
			// returns the integer value for +1.0 representation,
			// or 1 if full single float precision is maintained
			// in automation. parameter index in <value> (-1: all,
			// any)
			return( 1 );

		case audioMasterNeedIdle:
			//SHOW_CALLBACK( "amc: audioMasterNeedIdle\n" );
			// plug needs idle calls (outside its editor window)
			return( 1 );

		case audioMasterGetPreviousPlug:
			SHOW_CALLBACK( "amc: audioMasterGetPreviousPlug\n" );
			// input pin in <value> (-1: first to come), returns
			// cEffect*
			return( 0 );

		case audioMasterGetNextPlug:
			SHOW_CALLBACK( "amc: audioMasterGetNextPlug\n" );
			// output pin in <value> (-1: first to come), returns
			// cEffect*
			return( 0 );

		case audioMasterWillReplaceOrAccumulate:
			SHOW_CALLBACK( "amc: audioMasterWillReplaceOr"
							"Accumulate\n" );
			// returns: 0: not supported, 1: replace, 2: accumulate
			return( 0 );

		case audioMasterGetSpeakerArrangement:
			SHOW_CALLBACK( "amc: audioMasterGetSpeaker"
							"Arrangement\n" );
			// (long)input in <value>, output in <ptr>
			return( 0 );

		case audioMasterSetOutputSampleRate:
			SHOW_CALLBACK( "amc: audioMasterSetOutputSample"
								"Rate\n" );
			// for variable i/o, sample rate in <opt>
			return( 0 );

		case audioMasterSetIcon:
			SHOW_CALLBACK( "amc: audioMasterSetIcon\n" );
			// TODO
			// void* in <ptr>, format not defined yet
			return( 0 );

		case audioMasterOpenWindow:
			SHOW_CALLBACK( "amc: audioMasterOpenWindow\n" );
			// TODO
			// returns platform specific ptr
			return( 0 );
		
		case audioMasterCloseWindow:
			SHOW_CALLBACK( "amc: audioMasterCloseWindow\n" );
			// TODO
			// close window, platform specific handle in <ptr>
			return( 0 );
#endif

		case audioMasterSizeWindow:
			//SHOW_CALLBACK( "amc: audioMasterSizeWindow\n" );
			if( __plugin->m_window == 0 )
			{
				return( 0 );
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
			return( 1 );

		case audioMasterGetSampleRate:
			//SHOW_CALLBACK( "amc: audioMasterGetSampleRate\n" );
			return( __plugin->sampleRate() );

		case audioMasterGetBlockSize:
			//SHOW_CALLBACK( "amc: audioMasterGetBlockSize\n" );
			return( __plugin->bufferSize() );

		case audioMasterGetInputLatency:
			//SHOW_CALLBACK( "amc: audioMasterGetInputLatency\n" );
			return( __plugin->bufferSize() );

		case audioMasterGetOutputLatency:
			//SHOW_CALLBACK( "amc: audioMasterGetOutputLatency\n" );
			return( __plugin->bufferSize() );

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
			return( 0 );

		case audioMasterGetAutomationState:
			SHOW_CALLBACK( "amc: audioMasterGetAutomationState\n" );
			// returns 0: not supported, 1: off, 2:read, 3:write,
			// 4:read/write offline
			return( 0 );

		case audioMasterOfflineStart:
			SHOW_CALLBACK( "amc: audioMasterOfflineStart\n" );
			return( 0 );

		case audioMasterOfflineRead:
			SHOW_CALLBACK( "amc: audioMasterOfflineRead\n" );
			// ptr points to offline structure, see below.
			// return 0: error, 1 ok
			return( 0 );

		case audioMasterOfflineWrite:
			SHOW_CALLBACK( "amc: audioMasterOfflineWrite\n" );
			// same as read
			return( 0 );

		case audioMasterOfflineGetCurrentPass:
			SHOW_CALLBACK( "amc: audioMasterOfflineGetCurrent"
								"Pass\n" );
			return( 0 );

		case audioMasterOfflineGetCurrentMetaPass:
			SHOW_CALLBACK( "amc: audioMasterOfflineGetCurrentMeta"
								"Pass\n");
			return( 0 );

		case audioMasterGetVendorString:
			//SHOW_CALLBACK( "amc: audioMasterGetVendorString\n" );
			// fills <ptr> with a string identifying the vendor
			// (max 64 char)
			strcpy( (char *) _ptr, "Tobias Doerffel & others");
			return( 1 );

		case audioMasterGetProductString:
			//SHOW_CALLBACK( "amc: audioMasterGetProductString\n" );
			// fills <ptr> with a string with product name
			// (max 64 char)
			strcpy( (char *) _ptr,
					"LMMS VST Support Layer (LVSL)" );
			return( 1 );

		case audioMasterGetVendorVersion:
			SHOW_CALLBACK( "amc: audioMasterGetVendorVersion\n" );
			// returns vendor-specific version
			return( 1000 );

		case audioMasterVendorSpecific:
			SHOW_CALLBACK( "amc: audioMasterVendorSpecific\n" );
			// no definition, vendor specific handling
			return( 0 );
		
		case audioMasterCanDo:
			//SHOW_CALLBACK( "amc: audioMasterCanDo\n" );
			return( !strcmp( (char *) _ptr, "sendVstEvents" ) ||
				!strcmp( (char *) _ptr, "sendVstMidiEvent" ) ||
				!strcmp( (char *) _ptr, "sendVstTimeInfo" ) ||
				!strcmp( (char *) _ptr, "sizeWindow" ) );

		case audioMasterGetLanguage:
			//SHOW_CALLBACK( "amc: audioMasterGetLanguage\n" );
			return( hlang );

		case audioMasterGetDirectory:
			SHOW_CALLBACK( "amc: audioMasterGetDirectory\n" );
			// get plug directory, FSSpec on MAC, else char*
			return( 0 );
		
		case audioMasterUpdateDisplay:
			//SHOW_CALLBACK( "amc: audioMasterUpdateDisplay\n" );
			// something has changed, update 'multi-fx' display
			_effect->dispatcher( _effect, effEditIdle, 0, 0, NULL,
									0.0f );
			return( 0 );

#if kVstVersion > 2
		case audioMasterBeginEdit:
			SHOW_CALLBACK( "amc: audioMasterBeginEdit\n" );
			// begin of automation session (when mouse down),	
			// parameter index in <index>
			return( 0 );

		case audioMasterEndEdit:
			SHOW_CALLBACK( "amc: audioMasterEndEdit\n" );
			// end of automation session (when mouse up),
			// parameter index in <index>
			return( 0 );

		case audioMasterOpenFileSelector:
			SHOW_CALLBACK( "amc: audioMasterOpenFileSelector\n" );
			// open a fileselector window with VstFileSelect*
			// in <ptr>
			return( 0 );
#endif
		default:
			SHOW_CALLBACK( "VST master dispatcher: undefined: "
						"%d\n", (int) _opcode );
			break;
	}

	return( 0 );
}




DWORD WINAPI remoteVstPlugin::guiEventLoop( LPVOID _param )
{
	remoteVstPlugin * _this = static_cast<remoteVstPlugin *>( _param );
	_this->m_guiThreadID = GetCurrentThreadId();

	if( !( _this->m_plugin->flags & effFlagsHasEditor ) )
	{
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}


	HMODULE hInst = GetModuleHandleA( NULL );
	if( hInst == NULL )
	{
		fprintf( stderr, "can't get module handle\n" );
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}

#ifdef LMMS_BUILD_LINUX
	if( ( _this->m_window = CreateWindowExA(
				0, "LVSL", _this->m_shortName.c_str(),
			       ( WS_OVERLAPPEDWINDOW  | WS_THICKFRAME &
					 		~WS_MAXIMIZEBOX ),
			       0, 0, 10, 10,
				NULL, NULL, hInst, NULL ) ) == NULL )
	{
		fprintf( stderr, "cannot create editor window\n" );
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}

#else
	_this->m_windowID = 1;	// arbitrary value on win32 to signal
				// vstPlugin-class that we have an editor
#endif


	_this->m_plugin->dispatcher( _this->m_plugin, effEditOpen, 0, 0,
							_this->m_window, 0 );

	ERect * er;
	_this->m_plugin->dispatcher( _this->m_plugin, effEditGetRect, 0, 0,
								&er, 0 );

	_this->m_windowWidth = er->right - er->left;
	_this->m_windowHeight = er->bottom - er->top;

	SetWindowPos( _this->m_window, 0, 0, 0, _this->m_windowWidth + 8,
			_this->m_windowHeight + 26, SWP_NOMOVE | SWP_NOZORDER );
	_this->m_plugin->dispatcher( _this->m_plugin, effEditTop, 0, 0,
								NULL, 0 );

	ShowWindow( _this->m_window, SW_SHOWNORMAL );

#ifdef LMMS_BUILD_LINUX
	_this->m_windowID = (Sint32) GetPropA( _this->m_window,
						"__wine_x11_whole_window" );
#endif

	pthread_cond_signal( &_this->m_windowStatusChange );


	HWND timer_window = CreateWindowEx( 0, "LVSL", "dummy",
					0, 0, 0, 0, 0, NULL, NULL, hInst, NULL );
	// install GUI update timer
	SetTimer( timer_window, 1000, 50, NULL );

	MSG msg;

	bool quit = false;
	while( quit == false && GetMessageA( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessageA( &msg );

		if( msg.message == WM_TIMER )
		{
			// give plugin some idle-time for GUI-update
			_this->m_plugin->dispatcher( _this->m_plugin,
							effEditIdle, 0, 0,
								NULL, 0 );
		}
		else if( msg.message == WM_USER )
		{
			switch( msg.wParam )
			{
				case ClosePlugin:
					quit = true;
					break;

				default:
					break;
			}
		}
	}

	pthread_cond_signal( &_this->m_windowStatusChange );

	return( 0 );
}




int main( int _argc, char * * _argv )
{
	if( _argc < 3 )
	{
		fprintf( stderr, "not enough arguments\n" );
		return( -1 );
	}

#ifdef LMMS_BUILD_WIN32
	// (non-portable) initialization of statically linked pthread library
	pthread_win32_process_attach_np();
	pthread_win32_thread_attach_np();
#endif

	// WINE-startup
	HMODULE hInst = GetModuleHandleA( NULL );
	if( hInst == NULL )
	{
		fprintf( stderr, "can't get module handle\n" );
		return( -1 );
	}

	WNDCLASSA wc;
	wc.style = 0;
	wc.lpfnWndProc = DefWindowProcA;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIconA( hInst, "LVSL" );
	wc.hCursor = LoadCursorA( NULL, IDI_APPLICATION );
	wc.hbrBackground = (HBRUSH) GetStockObject( BLACK_BRUSH );
	wc.lpszMenuName = "MENU_LVSL";
	wc.lpszClassName = "LVSL";

	if( !RegisterClassA( &wc ) )
	{
		return( -1 );
	}

#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_SCHED_H
	// try to set realtime-priority
	struct sched_param sparam;
	sparam.sched_priority = ( sched_get_priority_max( SCHED_FIFO ) +
				sched_get_priority_min( SCHED_FIFO ) ) / 2;
	if( sched_setscheduler( 0, SCHED_FIFO, &sparam ) == -1 )
	{
		fprintf( stderr, "could not set realtime priority for "
							"remoteVstPlugin\n" );
	}
#endif
#endif

	__plugin = new remoteVstPlugin( atoi( _argv[1] ), atoi( _argv[2] ) );

	remotePluginClient::message m;
	while( ( m = __plugin->receiveMessage() ).id != IdQuit )
        {
		__plugin->processMessage( m );
	}

	delete __plugin;


#ifdef LMMS_BUILD_WIN32
	pthread_win32_thread_detach_np();
	pthread_win32_process_detach_np();
#endif

	return 0;

}

