/*
 * vestige.cpp - instrument-plugin for hosting VST-plugins
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QCursor>

#else

#include <qdom.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qcursor.h>

#endif


#include "vestige.h"
#include "channel_track.h"
#include "note_play_handle.h"
#include "buffer_allocator.h"
#include "mixer.h"
#include "song_editor.h"
#include "instrument_play_handle.h"
#include "pixmap_button.h"
#include "tooltip.h"
#include "spc_bg_hndl_widget.h"


#include "embed.cpp"


extern "C"
{

plugin::descriptor vestige_plugin_descriptor =
{
	STRINGIFY_PLUGIN_NAME( PLUGIN_NAME ),
	"VeSTige",
	QT_TRANSLATE_NOOP( "plugin",
			"experimental VST-hoster for using VST-plugins "
							"within LMMS" ),
	"Tobias Doerffel <tobydox@users.sf.net>",
	0x0100,
	plugin::INSTRUMENT,
	PLUGIN_NAME::findEmbeddedData( "logo.png" )
} ;

}


bool vestigeInstrument::s_initialized = FALSE;
bool vestigeInstrument::s_threadAdopted = FALSE;
QPixmap * vestigeInstrument::s_artwork = NULL;


vestigeInstrument::vestigeInstrument( channelTrack * _channel_track ) :
	instrument( _channel_track, vestige_plugin_descriptor.public_name ),
	specialBgHandlingWidget( PLUGIN_NAME::getIconPixmap( "artwork" ) ),
	m_handle( NULL ),
	m_fst( NULL )
{
	if( s_artwork == NULL )
	{
		s_artwork = new QPixmap( PLUGIN_NAME::getIconPixmap(
								"artwork" ) );
	}
#ifdef QT4
	QPalette pal;
	pal.setBrush( backgroundRole(), *s_artwork);
	setPalette( pal );
#else
	setErasePixmap( *s_artwork );
#endif

	m_openPluginButton = new pixmapButton( this );
	m_openPluginButton->setCheckable( FALSE );
	m_openPluginButton->setCursor( PointingHandCursor );
	m_openPluginButton->move( 200, 70 );
	m_openPluginButton->setActiveGraphic( embed::getIconPixmap(
							"project_open_down" ) );
	m_openPluginButton->setInactiveGraphic( embed::getIconPixmap(
							"project_open" ) );
	m_openPluginButton->setBgGraphic( getBackground(
						m_openPluginButton ) );
	connect( m_openPluginButton, SIGNAL( clicked() ), this,
						SLOT( openPlugin() ) );
	toolTip::add( m_openPluginButton, tr( "Open other VST-plugin" ) );

#ifdef QT4
	m_openPluginButton->setWhatsThis(
#else
	QWhatsThis::add( m_openPluginButton,
#endif
		tr( "Click here, if you want to open another VST-plugin. After "
			"clicking on this button, a file-open-dialog appears "
			"and you can select your file." ) );

	if( s_initialized == FALSE )
	{
		if( fst_init( fst_signal_handler ) )
		{
			// TODO: message-box
			return;
		}
		s_initialized = TRUE;
	}
	// now we need a play-handle which cares for calling play()
	instrumentPlayHandle * iph = new instrumentPlayHandle( this );
	mixer::inst()->addPlayHandle( iph );
}




vestigeInstrument::~vestigeInstrument()
{
	// this single call automates the rest of cleanup like trashing our
	// play-handle and so on
	invalidate();
	closePlugin();
}




void vestigeInstrument::loadSettings( const QDomElement & _this )
{
}




void vestigeInstrument::saveSettings( QDomDocument & _doc,
							QDomElement & _parent )
{
	QDomElement vst_de = _doc.createElement( nodeName() );
	_parent.appendChild( vst_de );
}




QString vestigeInstrument::nodeName( void ) const
{
	return( vestige_plugin_descriptor.name );
}




void vestigeInstrument::setParameter( const QString & _param,
							const QString & _value )
{
	if( _param == "plugin" )
	{
		closePlugin();

		m_plugin = _value;
		if( ( m_handle = fst_load(
#ifdef QT4
						m_plugin.constData().toAscii()
#else
						m_plugin.ascii()
#endif
							 ) ) == NULL )
		{
			QMessageBox::information( this,
					tr( "Failed loading plugin" ),
					tr( "The VST-plugin %1 couldn't be "
						"loaded for some reason." ).arg(
							m_plugin ),
							QMessageBox::Ok );
			return;
		}
		if( ( m_fst = fst_instantiate( m_handle, hostCallback,
							this ) ) == NULL )
		{
			QMessageBox::information( this,
					tr( "Failed instantiating plugin" ),
					tr( "Couldn't create an instance of "
						"VST-plugin %1 for some "
						"reason." ).arg( m_plugin ),
							QMessageBox::Ok );
			fst_unload( m_handle );
			m_handle = NULL;
			return;
		}

		// set sample-rate and blocksize
		m_fst->plugin->dispatcher( m_fst->plugin, effSetSampleRate, 0,
					0, NULL, mixer::inst()->sampleRate() );
		m_fst->plugin->dispatcher( m_fst->plugin, effSetBlockSize, 0, 
					mixer::inst()->framesPerAudioBuffer(),
					NULL, 0.0f );
		// set program to zero
		m_fst->plugin->dispatcher( m_fst->plugin, effSetProgram, 0, 0,
								NULL, 0.0f );
		// resume
		m_fst->plugin->dispatcher( m_fst->plugin, effMainsChanged, 0,
								1, NULL, 0.0f );
/*		if( fst_run_editor( m_fst ) )
		{
			printf( "VeSTige: cannot create editor\n" );
		}*/
		int vst_version = m_fst->plugin->dispatcher( m_fst->plugin,
							effGetVstVersion, 0, 0,
							NULL, 0.0f );
		if( vst_version < 2 )
		{
			QMessageBox::information( this,
					tr( "VST-plugin too old" ),
					tr( "The version of VST-plugin %1 "
						"is smaller than 2, which "
						"isn't supported." ).arg(
								m_plugin ),
							QMessageBox::Ok );
			return;
		}
/*		printf("WID:%d  %d\n", (int)fst_get_XID( m_fst ),
				(int)QWidget::find( fst_get_XID( m_fst ) ) );*/

		//printf("%d\n", m_fst->plugin->numParams);
	}
}




void vestigeInstrument::play( void )
{
	// the very first time, we have to adopt fst-thread
	if( !s_threadAdopted )
	{
		fst_adopt_thread();
		s_threadAdopted = TRUE;
	}

	if( m_handle == NULL || m_fst == NULL )
	{
		return;
	}

	// first we gonna post all MIDI-events we enqueued so far
	if( m_midiEvents.size() )
	{
		// since MIDI-events are not received immediately, we have
		// to have them stored somewhere even after dispatcher-call,
		// so we create static copies of the data and post them
		static VstEvents events;
		static vvector<VstMidiEvent> cur_events;
		cur_events = m_midiEvents;
		m_midiEvents.clear();
		for( csize i = 0; i < cur_events.size(); ++i )
		{
			events.events[i] = (VstEvent *) &cur_events[i];
		}
		events.numEvents = cur_events.size();
		events.reserved = 0;
		m_fst->plugin->dispatcher( m_fst->plugin, effProcessEvents,
							0, 0, &events, 0.0f );
	}

	// now we're ready to fetch sound from VST-plugin
	const Uint32 frames = mixer::inst()->framesPerAudioBuffer();

	int ch_in = m_fst->plugin->numInputs;
	int ch_out = m_fst->plugin->numOutputs;
	float * ins[ch_in];
	float * outs[ch_out];
	for( int i = 0; i < ch_in; ++i )
	{
		ins[i] = bufferAllocator::alloc<float>( frames );
	}
	for( int i = 0; i < ch_out; ++i )
	{
		outs[i] = bufferAllocator::alloc<float>( frames );
	}

	if( m_fst->plugin->flags & effFlagsCanReplacing )
	{
		m_fst->plugin->processReplacing( m_fst->plugin, ins, outs,
								frames );
	
	}
	else
	{
		printf("normal process\n");
		//mixer::inst()->clearAudioBuffer( buf, frames );
		m_fst->plugin->process( m_fst->plugin, ins, outs, frames );
		printf("normal process done\n");
	}

	for( int i = 0; i < ch_in; ++i )
	{
		bufferAllocator::free( ins[i] );
	}

	// got our data, now we just have to merge the 1/2 out-buffers
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( frames );
	Uint8 chnls = tMax<Uint8>( ch_out, DEFAULT_CHANNELS );
	if( chnls != DEFAULT_CHANNELS )
	{
		mixer::inst()->clearAudioBuffer( buf, frames );
	}

	for( Uint32 f = 0; f < frames; ++f )
	{
		for( Uint8 chnl = 0; chnl < chnls; ++chnl )
		{
			buf[f][chnl] = outs[chnl][f];
		}
	}

	for( int i = 0; i < ch_out; ++i )
	{
		bufferAllocator::free( outs[i] );
	}

	getChannelTrack()->processAudioBuffer( buf, frames, NULL );

	bufferAllocator::free( buf );
}






void vestigeInstrument::playNote( notePlayHandle * _n )
{
	if( _n->totalFramesPlayed() == 0 )
	{
		enqueueEvent( midiEvent( NOTE_ON, 0, _n->key(),
					_n->getVolume() ), _n->framesAhead() );
	}
}




void vestigeInstrument::deleteNotePluginData( notePlayHandle * _n )
{
	enqueueEvent( midiEvent( NOTE_OFF, 0, _n->key(), 0 ) );
}




void vestigeInstrument::openPlugin( void )
{
#ifdef QT4
	QFileDialog ofd( NULL, tr( "Open VST-plugin" ) );
#else
	QFileDialog ofd( QString::null, QString::null, NULL, "", TRUE );
	ofd.setWindowTitle( tr( "Open VST-plugin" ) );
#endif

	QString dir;
	if( m_plugin != "" )
	{
#ifdef QT4
		dir = QFileInfo( m_plugin ).absolutePath();
#else
		dir = QFileInfo( m_plugin ).dirPath( TRUE );
#endif
	}
	else
	{
		dir = QDir::home().path();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "DLL-files (*.dll)" )
		<< tr( "EXE-files (*.exe)" )
		;
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "DLL-files (*.dll)" ) );
	ofd.addFilter( tr( "EXE-files (*.exe)" ) );
	ofd.setSelectedFilter( tr( "DLL-files (*.dll)" ) );
#endif
	if( m_plugin != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_plugin ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return;
		}
		setParameter( "plugin", ofd.selectedFiles()[0] );
	}
}




void vestigeInstrument::paintEvent( QPaintEvent * )
{
#ifdef QT4
	QPainter p( this );
#else
	QPixmap pm( rect().size() );
	pm.fill( this, rect().topLeft() );

	QPainter p( &pm, this );
#endif

	p.drawPixmap( 0, 0, *s_artwork );

	QString plugin_name = ( m_handle != NULL && m_fst != NULL ) ? 
				QString( m_handle->name ) + " " +
				QString::number( m_fst->plugin->dispatcher(
							m_fst->plugin,
							effGetVendorVersion,
							0, 0, NULL, 0.0f ) ):
				tr( "No VST-plugin loaded" );
	QFont f = p.font();
	f.setBold( TRUE );
	p.setFont( pointSize<10>( f ) );
	p.setPen( QColor( 0, 0, 0 ) );

	p.drawText( 20, 80, plugin_name );

	if( m_handle != NULL && m_fst != NULL )
	{
		p.setPen( QColor( 64, 128, 64 ) );
		f.setBold( FALSE );
		p.setFont( pointSize<8>( f ) );
		char buf[1024];
		m_fst->plugin->dispatcher( m_fst->plugin, effGetVendorString,
								0, 0, buf, 0 );
		p.drawText( 20, 94, tr( "by" ) + " " + QString( buf ) );
	}
#ifndef QT4
	bitBlt( this, rect().topLeft(), &pm );
#endif
}




void vestigeInstrument::closePlugin( void )
{
	if( m_fst != NULL && m_handle != NULL )
	{
		//fst_destroy_editor( m_fst );
		printf( "closing VST-plugin" );
		fst_close( m_fst );
		printf( "unloading VST-plugin" );
		fst_unload( m_handle );
		m_fst = NULL;
		m_handle = NULL;
	}
}




void vestigeInstrument::enqueueEvent( const midiEvent & _e,
							Uint32 _frames_ahead )
{
	if( m_handle == NULL || m_fst == NULL )
	{
		return;
	}

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
	event.midiData[0] = _e.m_type + _e.m_channel;
	event.midiData[1] = _e.key();
	event.midiData[2] = _e.velocity();
	event.midiData[3] = 0;

	m_midiEvents.push_back( event );
}



#define DEBUG_CALLBACKS
#ifdef DEBUG_CALLBACKS
#define SHOW_CALLBACK printf
#else
#define SHOW_CALLBACK(...)
#endif

long vestigeInstrument::hostCallback( AEffect * _effect, long _opcode,
					long _index, long _value, void * _ptr,
								float _opt )
{
	static VstTimeInfo _timeInfo;

	SHOW_CALLBACK( "host-callback, opcode = %d", (int) _opcode );
	
	switch( _opcode )
	{
		case audioMasterAutomate:
			SHOW_CALLBACK( "amc: audioMasterAutomate\n" );
			// index, value, returns 0
			_effect->setParameter( _effect, _index, _opt );
			return( 0 );

		case audioMasterVersion:
			SHOW_CALLBACK( "amc: audioMasterVersion\n" );
			// vst version, currently 2 (0 for older)
			return( 2 );

		case audioMasterCurrentId:	
			SHOW_CALLBACK( "amc: audioMasterCurrentId\n" );
			// returns the unique id of a plug that's currently
			// loading
			return( 0 );
		
		case audioMasterIdle:
			SHOW_CALLBACK ("amc: audioMasterIdle\n");
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

		case audioMasterWantMidi:
			SHOW_CALLBACK( "amc: audioMasterWantMidi\n" );
			// <value> is a filter which is currently ignored
			return( 0 );

		case audioMasterGetTime:
			SHOW_CALLBACK( "amc: audioMasterGetTime\n" );
			// returns const VstTimeInfo* (or 0 if not supported)
			// <value> should contain a mask indicating which
			// fields are required (see valid masks above), as some
			// items may require extensive conversions

			memset( &_timeInfo, 0, sizeof( _timeInfo ) );

			//tstate = jack_transport_query (jackvst->client, &jack_pos);
			_timeInfo.samplePos = 0;
			_timeInfo.sampleRate = mixer::inst()->sampleRate();
			_timeInfo.flags = 0;
			_timeInfo.tempo = songEditor::inst()->getBPM();
			_timeInfo.timeSigNumerator = 4;//(long) floor (jack_pos.beats_per_bar);
			_timeInfo.timeSigDenominator = 4;//(long) floor (jack_pos.beat_type);
			_timeInfo.flags |= (kVstBarsValid|kVstTempoValid);
//			if (tstate == JackTransportRolling) {
			_timeInfo.flags |= kVstTransportPlaying;
//			}

			return( (long)&_timeInfo );

		case audioMasterProcessEvents:
			SHOW_CALLBACK( "amc: audioMasterProcessEvents\n" );
			// VstEvents* in <ptr>
			return( 0 );

		case audioMasterSetTime:
			SHOW_CALLBACK( "amc: audioMasterSetTime\n" );
			// VstTimenfo* in <ptr>, filter in <value>, not
			// supported

		case audioMasterTempoAt:
			SHOW_CALLBACK( "amc: audioMasterTempoAt\n" );
			// returns tempo (in bpm * 10000) at sample frame
			// location passed in <value>
			return( 0 );

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

		case audioMasterIOChanged:
			SHOW_CALLBACK( "amc: audioMasterIOChanged\n" );
		       // numInputs and/or numOutputs has changed
			return( 0 );

		case audioMasterNeedIdle:
			SHOW_CALLBACK( "amc: audioMasterNeedIdle\n" );
			// plug needs idle calls (outside its editor window)
			return( 0 );

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

		case audioMasterSetOutputSampleRate:
			SHOW_CALLBACK( "amc: audioMasterSetOutputSample"
								"Rate\n" );
			// for variable i/o, sample rate in <opt>
			return( 0 );

		case audioMasterGetSpeakerArrangement:
			SHOW_CALLBACK( "amc: audioMasterGetSpeaker"
							"Arrangement\n" );
			// (long)input in <value>, output in <ptr>
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
			strcpy( (char *) _ptr, "VeSTige" );
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
		
		case audioMasterSetIcon:
			SHOW_CALLBACK( "amc: audioMasterSetIcon\n" );
			// TODO
			// void* in <ptr>, format not defined yet
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
		
		case audioMasterGetDirectory:
			SHOW_CALLBACK( "amc: audioMasterGetDirectory\n" );
			// TODO
			// get plug directory, FSSpec on MAC, else char*
			return( 0 );
		
		case audioMasterUpdateDisplay:
			SHOW_CALLBACK( "amc: audioMasterUpdateDisplay\n" );
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
			SHOW_CALLBACK( "VST master dispatcher: undefed: "
					"%d, %d\n", (int) _opcode,
							effKeysRequired );
			break;
	}

	return( 0 );
}



extern "C"
{

// neccessary for getting instance out of shared lib
plugin * lmms_plugin_main( void * _data )
{
	return( new vestigeInstrument( static_cast<channelTrack *>( _data ) ) );
}


}


#include "vestige.moc"

