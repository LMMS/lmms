/*
 * lvsl_server.cpp - LMMS VST Support Layer Server
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * Code partly taken from XFST:
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <windows.h>
#include <wine/exception.h>


#include <vector>
#include <string>


#include "types.h"
#include "midi.h"
#include "communication.h"


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


#ifdef HAVE_TLS
static __thread int ejmpbuf_valid = false;
static __thread jmp_buf ejmpbuf;
#else
static pthread_key_t ejmpbuf_valid_key;
static pthread_key_t ejmpbuf_key;
#endif


class VSTPlugin;

VSTPlugin * plugin = NULL;



void lvsMessage( const char * _fmt, ... )
{
	va_list ap;
	char buffer[512];

	va_start( ap, _fmt );
	vsnprintf( buffer, sizeof( buffer ), _fmt, ap );
	writeValue<Sint16>( VST_DEBUG_MSG, 1 );
	writeString( buffer, 1 );
	va_end( ap );
}




class VSTPlugin
{
public:
	VSTPlugin( const std::string & _plugin_file );

	~VSTPlugin();

	void showEditor( void )
	{
		if( m_window != NULL )
		{
			PostThreadMessageA( m_guiThreadID, WM_USER, SHOW_EDITOR,
									0 );
		}
	}

	void process( void );

	// enqueue given MIDI-event to be processed the next time process() is
	// called
	void enqueueMidiEvent( const midiEvent & _event,
						const Uint32 _frames_ahead );

	// set given sample-rate for plugin
	void setSampleRate( const Sint32 _rate )
	{
		m_plugin->dispatcher( m_plugin, effSetSampleRate, 0, 0, NULL, 
							    (float) _rate );
	}

	// set given block-size for plugin
	void setBlockSize( const Uint32 _bsize );

	// set given tempo
	void setBPM( const Uint16 _bpm )
	{
		m_bpm = _bpm;
	}

	// determine VST-version the plugin uses
	Sint32 pluginVersion( void ) const
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
	void getParameterDump( void ) const;

	// read parameter-dump and set it for plugin
	void setParameterDump( void );

	// post properties of specified parameter
	void getParameterProperties( const Sint32 _idx );

	// number of inputs
	Uint8 inputCount( void ) const
	{
		return( m_plugin->numInputs );
	}

	// number of outputs
	Uint8 outputCount( void ) const
	{
		return( m_plugin->numOutputs );
	}

	// called once at initialization and everytime number of inputs/outputs
	// or block-size changes and is responsible for resizing shared memory
	// and inform client about changed shm-keys, input/output-count etc.
	void resizeSharedMemory( void );


private:
	enum guiThreadMessages
	{
		NONE, SHOW_EDITOR, CLOSE_PLUGIN
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
	int m_windowXID;

	pthread_mutex_t m_lock;
	pthread_cond_t m_windowStatusChange;
	DWORD m_guiThreadID;


	Uint32 m_blockSize;
	float * m_shm;

	float * * m_inputs;
	float * * m_outputs;

	std::vector<VstMidiEvent> m_midiEvents;

	Uint16 m_bpm;

} ;




VSTPlugin::VSTPlugin( const std::string & _plugin_file ) :
	m_shortName( "" ),
	m_libInst( NULL ),
	m_plugin( NULL ),
	m_window( NULL ),
	m_windowXID( 0 ),
	m_lock(),
	m_windowStatusChange(),
	m_guiThreadID( 0 ),
	m_blockSize( 0 ),
	m_shm( NULL ),
	m_inputs( NULL ),
	m_outputs( NULL ),
	m_midiEvents(),
	m_bpm( 0 )
{
	if( load( _plugin_file ) == false )
	{
		writeValue<Sint16>( VST_FAILED_LOADING_PLUGIN );
		return;
	}

	/* set program to zero */
	/* i comment this out because it breaks dfx Geometer
	 * looks like we cant set programs for it
	 *
	m_plugin->dispatcher( m_plugin, effSetProgram, 0, 0, NULL, 0.0f); */
	// request rate and blocksize
	writeValue<Sint16>( VST_GET_SAMPLE_RATE );
	writeValue<Sint16>( VST_GET_BUFFER_SIZE );


	m_plugin->dispatcher( m_plugin, effMainsChanged, 0, 1, NULL, 0.0f );


	if( CreateThread( NULL, 0, guiEventLoop, this, 0, NULL ) == NULL )
	{
		lvsMessage( "could not create GUI-thread" );
		return;
	}
	pthread_cond_wait( &m_windowStatusChange, &m_lock);


	// now post some information about our plugin
	writeValue<Sint16>( VST_PLUGIN_XID );
	writeValue<Sint32>( m_windowXID );

	writeValue<Sint16>( VST_PLUGIN_NAME );
	writeString( pluginName() );

	writeValue<Sint16>( VST_PLUGIN_VERSION );
	writeValue<Sint32>( pluginVersion() );

	writeValue<Sint16>( VST_PLUGIN_VENDOR_STRING );
	writeString( pluginVendorString() );

	writeValue<Sint16>( VST_PLUGIN_PRODUCT_STRING );
	writeString( pluginProductString() );

	writeValue<Sint16>( VST_PARAMETER_COUNT );
	writeValue<Sint32>( (Sint32) m_plugin->numParams );

	// tell client that we've done everything so far
	writeValue<Sint16>( VST_INITIALIZATION_DONE );
}




VSTPlugin::~VSTPlugin()
{
	// acknowledge quit
	writeValue<Sint16>( VST_QUIT_ACK );

	if( m_window != NULL )
	{
		// notify GUI-thread
		if( !PostThreadMessageA( m_guiThreadID, WM_USER,
							CLOSE_PLUGIN, 0 ) )
		{
			//lvsMessage( "could not post message to gui thread" );
		}
		pthread_cond_wait( &m_windowStatusChange, &m_lock );
		m_plugin->dispatcher( m_plugin, effEditClose, 0, 0, NULL, 0.0 );
		CloseWindow( m_window );
		m_window = NULL;
	}

	if( m_libInst != NULL )
	{
		FreeLibrary( m_libInst );
		m_libInst = NULL;
	}

	delete[] m_inputs;
	delete[] m_outputs;

	if( m_shm != NULL )
	{
		shmdt( m_shm );
	}
}




bool VSTPlugin::load( const std::string & _plugin_file )
{
	if( ( m_libInst = LoadLibraryA( _plugin_file.c_str() ) ) ==
								NULL )
	{
		return( false );
	}

	char * tmp = strdup( _plugin_file.c_str() );
	m_shortName = basename( tmp );
	free( tmp );

	typedef AEffect * ( * mainEntry )( audioMasterCallback );
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
		lvsMessage( "%s is not a VST plugin\n", _plugin_file.c_str() );
	}

	m_plugin->dispatcher( m_plugin, effOpen, 0, 0, 0, 0 );

	return( true );
}



void VSTPlugin::process( void )
{
	// first we gonna post all MIDI-events we enqueued so far
	if( m_midiEvents.size() )
	{
		// since MIDI-events are not received immediately, we
		// have to have them stored somewhere even after
		// dispatcher-call, so we create static copies of the
		// data and post them
#define MIDI_EVENT_BUFFER_COUNT		1024
		static char event_buf[sizeof(VstMidiEvent * ) *
						MIDI_EVENT_BUFFER_COUNT +
							sizeof( VstEvents )];
		static VstMidiEvent vme[MIDI_EVENT_BUFFER_COUNT];
		VstEvents * events = (VstEvents *) event_buf;
		events->reserved = 0;
		events->numEvents = m_midiEvents.size();
		for( unsigned int i = 0; i < m_midiEvents.size(); ++i )
		{
			memcpy( &vme[i], &m_midiEvents[i],
						sizeof( VstMidiEvent ) );
			events->events[i] = (VstEvent *) &vme[i];
		}
		m_midiEvents.clear();
		m_plugin->dispatcher( m_plugin, effProcessEvents, 0, 0, events,
									0.0f );
	}

	// now we're ready to fetch sound from VST-plugin

	for( int i = 0; i < inputCount(); ++i )
	{
		m_inputs[i] = &m_shm[i * m_blockSize];
	}

	for( int i = 0; i < outputCount(); ++i )
	{
		m_outputs[i] = &m_shm[( i + inputCount() ) *
							m_blockSize];
	}

#ifdef OLD_VST_SDK
	if( m_plugin->flags & effFlagsCanReplacing )
	{
#endif
		m_plugin->processReplacing( m_plugin, m_inputs,
							m_outputs,
							m_blockSize );
#ifdef OLD_VST_SDK
	}
	else
	{
		m_plugin->process( m_plugin, m_inputs, m_outputs,
							m_blockSize );
	}
#endif

	writeValue<Sint16>( VST_PROCESS_DONE );

	// give plugin some idle-time for GUI-update and so on...
	m_plugin->dispatcher( m_plugin, effEditIdle, 0, 0, NULL, 0 );

}




void VSTPlugin::enqueueMidiEvent( const midiEvent & _event,
						const Uint32 _frames_ahead )
{
	VstMidiEvent event;

	event.type = kVstMidiType;
	event.byteSize = 24;
	event.deltaFrames = _frames_ahead;
	event.flags = 0;
	event.detune = 0;
	event.noteLength = 0;
	event.noteOffset = 0;
	event.noteOffVelocity = 0;
	event.reserved1 = 0;
	event.reserved2 = 0;
	event.midiData[0] = _event.m_type + _event.m_channel;
	event.midiData[1] = _event.key();
	event.midiData[2] = _event.velocity();
	event.midiData[3] = 0;
	m_midiEvents.push_back( event );
}




void VSTPlugin::setBlockSize( const Uint32 _bsize )
{
	if( _bsize == m_blockSize )
	{
		return;
	}
	m_blockSize = _bsize;
	resizeSharedMemory();
	m_plugin->dispatcher( m_plugin, effSetBlockSize, 0, _bsize, NULL,
									0.0f );
}




const char * VSTPlugin::pluginName( void ) const
{
	static char buf[32];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetEffectName, 0, 0, buf, 0.0f );
	buf[31] = 0;
	return( buf );
}




const char * VSTPlugin::pluginVendorString( void ) const
{
	static char buf[64];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetVendorString, 0, 0, buf, 0.0f );
	buf[63] = 0;
	return( buf );
}




const char * VSTPlugin::pluginProductString( void ) const
{
	static char buf[64];
	buf[0] = 0;
	m_plugin->dispatcher( m_plugin, effGetProductString, 0, 0, buf, 0.0f );
	buf[63] = 0;
	return( buf );
}




void VSTPlugin::getParameterDump( void ) const
{
	VstParameterProperties vst_props;
	vstParameterDumpItem dump_item;
	writeValue<Sint16>( VST_PARAMETER_DUMP );
	writeValue<Sint32>( (Sint32) m_plugin->numParams - 1 );
	for( Sint32 i = 0; i < m_plugin->numParams - 1; ++i )
	{
		dump_item.index = i;
		m_plugin->dispatcher( m_plugin, effGetParameterProperties, i, 0,
							&vst_props, 0.0f );
		memcpy( dump_item.shortLabel, vst_props.shortLabel,
					sizeof( dump_item.shortLabel ) );
		dump_item.value = m_plugin->getParameter( m_plugin, i );
		writeValue<vstParameterDumpItem>( dump_item );
	}
}




void VSTPlugin::setParameterDump( void )
{
	const Sint32 sz = readValue<Sint32>();
	const Sint32 params = ( sz > m_plugin->numParams - 1 ) ?
					m_plugin->numParams - 1 : sz;
	for( Sint32 i = 0; i < params; ++i )
	{
		vstParameterDumpItem dump_item =
					readValue<vstParameterDumpItem>();
		m_plugin->setParameter( m_plugin, dump_item.index,
							dump_item.value );
	}
}




void VSTPlugin::getParameterProperties( const Sint32 _idx )
{
	VstParameterProperties vst_props;
	m_plugin->dispatcher( m_plugin, effGetParameterProperties, _idx, 0,
							&vst_props, 0.0f );
	vstParamProperties props;
	memcpy( props.label, vst_props.label, sizeof( props.label ) );
	memcpy( props.shortLabel, vst_props.shortLabel,
						sizeof( props.shortLabel) );
	memcpy( props.categoryLabel, vst_props.categoryLabel,
						sizeof( props.categoryLabel ) );
	props.minValue = vst_props.minInteger;
	props.maxValue = vst_props.maxInteger;
	props.step = ( vst_props.flags & kVstParameterUsesFloatStep ) ?
							vst_props.stepFloat :
							vst_props.stepInteger;
	props.category = vst_props.category;
	writeValue<Sint16>( VST_PARAMETER_PROPERTIES );
	writeValue<vstParamProperties>( props );
}




void VSTPlugin::resizeSharedMemory( void )
{
	delete[] m_inputs;
	delete[] m_outputs;

	size_t s = ( inputCount() + outputCount() ) * m_blockSize *
								sizeof( float );
	if( m_shm != NULL )
	{
		shmdt( m_shm );
	}

	int shm_id;
	Uint16 shm_key = 0;
	while( ( shm_id = shmget( ++shm_key, s, IPC_CREAT | IPC_EXCL |
						0666 ) ) == -1 )
	{
	}

	m_shm = (float *) shmat( shm_id, 0, 0 );

	if( inputCount() > 0 )
	{
		m_inputs = new float * [inputCount()];
	}
	if( outputCount() > 0 )
	{
		m_outputs = new float * [outputCount()];
	}

	writeValue<Sint16>( VST_INPUT_COUNT );
	writeValue<Uint8>( inputCount() );

	writeValue<Sint16>( VST_OUTPUT_COUNT );
	writeValue<Uint8>( outputCount() );

	writeValue<Sint16>( VST_SHM_KEY_AND_SIZE );
	writeValue<Uint16>( shm_key );
	writeValue<size_t>( s );
}



#define DEBUG_CALLBACKS
#ifdef DEBUG_CALLBACKS
#define SHOW_CALLBACK lvsMessage 
#else
#define SHOW_CALLBACK(...)
#endif


VstIntPtr VSTPlugin::hostCallback( AEffect * _effect, VstInt32 _opcode,
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
			// vst version, currently 2 (0 for older)
			return( 2 );

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
			SHOW_CALLBACK( "amc: audioMasterPinConnected\n" );
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

			_timeInfo.samplePos = 0;
			// TODO: _timeInfo.sampleRate = mixer::inst()->sampleRate();
			_timeInfo.flags = 0;
			_timeInfo.tempo = plugin->m_bpm;
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
			plugin->resizeSharedMemory();
			SHOW_CALLBACK( "amc: audioMasterIOChanged\n" );
			// numInputs and/or numOutputs has changed
			return( 0 );

#ifdef OLD_VST_SDK
		case audioMasterWantMidi:
			//SHOW_CALLBACK( "amc: audioMasterWantMidi\n" );
			// <value> is a filter which is currently ignored
			return( 0 );

		case audioMasterSetTime:
			SHOW_CALLBACK( "amc: audioMasterSetTime\n" );
			// VstTimenfo* in <ptr>, filter in <value>, not
			// supported

		case audioMasterTempoAt:
			SHOW_CALLBACK( "amc: audioMasterTempoAt\n" );
			return( plugin->m_bpm * 10000 );

		case audioMasterGetNumAutomatableParameters:
			SHOW_CALLBACK( "amc: audioMasterGetNumAutomatable"
							"Parameters\n" );
			return( 0 );

		case audioMasterGetParameterQuantization:	
			SHOW_CALLBACK( "amc: audioMasterGetParameter\n"
							"Quantization\n" );
			// returns the integer value for +1.0 representation,
			// or 1 if full single float precision is maintained
			// in automation. parameter index in <value> (-1: all,
			// any)
			return( 0 );

		case audioMasterNeedIdle:
			//SHOW_CALLBACK( "amc: audioMasterNeedIdle\n" );
			// plug needs idle calls (outside its editor window)
			return( 0 );

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
			// TODO using lmms-main-window-size
			SHOW_CALLBACK( "amc: audioMasterSizeWindow\n" );
			// index: width, value: height
			return( 0 );

		case audioMasterGetSampleRate:
			// TODO using mixer-call
			SHOW_CALLBACK( "amc: audioMasterGetSampleRate\n" );
			return( 0 );

		case audioMasterGetBlockSize:
			// TODO using mixer-call
			SHOW_CALLBACK( "amc: audioMasterGetBlockSize\n" );
			return( 0 );

		case audioMasterGetInputLatency:
			// TODO using mixer-call
			SHOW_CALLBACK( "amc: audioMasterGetInputLatency\n" );
			return( 0 );

		case audioMasterGetOutputLatency:
			// TODO using mixer-call
			SHOW_CALLBACK( "amc: audioMasterGetOutputLatency\n" );
			return( 0 );

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
			SHOW_CALLBACK( "amc: audioMasterGetVendorString\n" );
			// fills <ptr> with a string identifying the vendor
			// (max 64 char)
			strcpy( (char *) _ptr, "LAD");
			return( 0 );

		case audioMasterGetProductString:
			SHOW_CALLBACK( "amc: audioMasterGetProductString\n" );
			// fills <ptr> with a string with product name
			// (max 64 char)
			strcpy( (char *) _ptr, "XFST-Server" );
			return( 0 );

		case audioMasterGetVendorVersion:
			SHOW_CALLBACK( "amc: audioMasterGetVendorVersion\n" );
			// TODO
			// returns vendor-specific version
			return( 1000 );

		case audioMasterVendorSpecific:
			SHOW_CALLBACK( "amc: audioMasterVendorSpecific\n" );
			// no definition, vendor specific handling
			return( 0 );
		
		case audioMasterCanDo:
			SHOW_CALLBACK( "amc: audioMasterCanDo\n" );
			// string in ptr, see below
			return( 0 );
		
		case audioMasterGetLanguage:
			SHOW_CALLBACK( "amc: audioMasterGetLanguage\n" );
			// TODO
			// see enum
			return( 0 );

		case audioMasterGetDirectory:
			SHOW_CALLBACK( "amc: audioMasterGetDirectory\n" );
			// TODO
			// get plug directory, FSSpec on MAC, else char*
			return( 0 );
		
		case audioMasterUpdateDisplay:
			//SHOW_CALLBACK( "amc: audioMasterUpdateDisplay\n" );
			// something has changed, update 'multi-fx' display
			_effect->dispatcher( _effect, effEditIdle, 0, 0, NULL,
									0.0f );
			return( 0 );

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

		default:
			SHOW_CALLBACK( "VST master dispatcher: undefined: "
						"%d\n", (int) _opcode );
			break;
	}

	return( 0 );
}




DWORD WINAPI VSTPlugin::guiEventLoop( LPVOID _param )
{
	VSTPlugin * _this = static_cast<VSTPlugin *>( _param );
	_this->m_guiThreadID = GetCurrentThreadId();

	// "guard point" to trap errors that occur during plugin loading
#ifdef HAVE_TLS
	if( sigsetjmp( ejmpbuf, 1 ) )
	{
		lvsMessage( "creating the editor for %s failed",
						_this->m_shortName.c_str() );
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}

	ejmpbuf_valid = true;
#else
	jmp_buf * ejmpbuf = new jmp_buf[1];
	int * ejmpbuf_valid = new int;
	*ejmpbuf_valid = false;

	pthread_key_create( &ejmpbuf_key, NULL );
	pthread_setspecific( ejmpbuf_key, ejmpbuf );
	pthread_key_create( &ejmpbuf_valid_key, NULL );
	pthread_setspecific( ejmpbuf_valid_key, ejmpbuf_valid );

	if( sigsetjmp( *ejmpbuf, 1 ) )
	{
		exit( 1 );
	}
	
	*ejmpbuf_valid = true;
#endif

	// Note: m_lock is held while this function is called
	if( !( _this->m_plugin->flags & effFlagsHasEditor ) )
	{
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}


	HMODULE hInst = GetModuleHandleA( NULL );
	if( hInst == NULL )
	{
		lvsMessage( "can't get module handle" );
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}
	
	if( ( _this->m_window = CreateWindowExA(
				0, "LVSL", _this->m_shortName.c_str(),
			       ( WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME &
					 		~WS_MAXIMIZEBOX ),
			       0, 0, 1, 1, NULL, NULL, hInst, NULL ) ) == NULL )
	{
		lvsMessage( "cannot create editor window" );
		pthread_cond_signal( &_this->m_windowStatusChange );
		return( 1 );
	}

	_this->m_windowXID = (int) GetPropA( _this->m_window,
						"__wine_x11_whole_window" );


	_this->m_plugin->dispatcher( _this->m_plugin, effEditOpen, 0, 0,
							_this->m_window, 0 );

	ERect * er;
	_this->m_plugin->dispatcher( _this->m_plugin, effEditGetRect, 0, 0,
								&er, 0 );

/*	const int width = er->right - er->left;
	const int height = er->bottom - er->top;*/
		
	SetWindowPos( _this->m_window, 0, 0, 0, er->right - er->left + 8,
			er->bottom-er->top + 26, SWP_NOACTIVATE | SWP_NOREDRAW |
						SWP_NOMOVE | SWP_NOZORDER );
	*ejmpbuf_valid = false;

	pthread_cond_signal( &_this->m_windowStatusChange );


	MSG msg;

	bool quit = false;
	while( quit == false && GetMessageA( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessageA( &msg );

		if( msg.message == WM_USER )
		{
			switch( msg.wParam )
			{
				case SHOW_EDITOR:
					ShowWindow( _this->m_window, SW_SHOW );
					break;

				case CLOSE_PLUGIN:
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




int main( void ) 
{
	// WINE-startup
	HMODULE hInst = GetModuleHandleA( NULL );
	if( hInst == NULL )
	{
		lvsMessage( "can't get module handle" );
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

	Sint16 cmd;
	while( ( cmd = readValue<Sint16>() ) != VST_CLOSE_PLUGIN )
	{
		switch( cmd )
		{
			case VST_LOAD_PLUGIN:
				plugin = new VSTPlugin( readString() );
				break;

			case VST_SHOW_EDITOR:
				plugin->showEditor();
				break;

			case VST_PROCESS:
				plugin->process();
				break;

			case VST_ENQUEUE_MIDI_EVENT:
			{
				const midiEvent ev = readValue<midiEvent>();
				const Uint32 fr_ahead = readValue<Uint32>();
				plugin->enqueueMidiEvent( ev, fr_ahead );
				break;
			}

			case VST_SAMPLE_RATE:
				plugin->setSampleRate( readValue<Sint32>() );
				break;


			case VST_BUFFER_SIZE:
				plugin->setBlockSize( readValue<Uint32>() );
				break;

			case VST_BPM:
				plugin->setBPM( readValue<Uint16>() );
				break;

			case VST_GET_PARAMETER_DUMP:
				plugin->getParameterDump();
				break;

			case VST_SET_PARAMETER_DUMP:
				plugin->setParameterDump();
				break;

			case VST_GET_PARAMETER_PROPERTIES:
				plugin->getParameterProperties(
							readValue<Sint32>() );
				break;

			default:
				lvsMessage( "unhandled message: %d\n",
								(int) cmd );
				break;
		}
	}

	delete plugin;

	return( 0 );

}

