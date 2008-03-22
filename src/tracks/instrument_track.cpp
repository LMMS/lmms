#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_track.cpp - implementation of instrument-track-class
 *                        (window + data-structures)
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#include "instrument_track.h"


#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QQueue>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMdiArea>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>


#include "audio_port.h"
#include "automation_pattern.h"
#include "config_mgr.h"
#include "debug.h"
#include "effect_chain.h"
#include "effect_rack_view.h"
#include "embed.h"
#include "engine.h"
#include "instrument_sound_shaping.h"
#include "instrument_sound_shaping_view.h"
#include "fade_button.h"
#include "gui_templates.h"
#include "instrument.h"
#include "instrument_function_views.h"
#include "instrument_midi_io_view.h"
#include "led_checkbox.h"
#include "main_window.h"
#include "midi_client.h"
#include "midi_port.h"
#include "fx_mixer.h"
#include "instrument_midi_io.h"
#include "mmp.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "plugin_view.h"
#include "sample_play_handle.h"
#include "song.h"
#include "string_pair_drag.h"
#include "tab_widget.h"
#include "tooltip.h"
#include "volume_knob.h"



const char * volume_help = QT_TRANSLATE_NOOP( "instrumentTrack",
						"With this knob you can set "
						"the volume of the opened "
						"channel.");
const char * surroundarea_help = QT_TRANSLATE_NOOP( "instrumentTrack",
						"Within this rectangle you can "
						"set the position where the "
						"channel should be audible. "
						"You should have a soundcard "
						"supporting at least surround "
						"4.0 for enjoying this "
						"feature." );


const int INSTRUMENT_WIDTH	= 250;
const int INSTRUMENT_HEIGHT	= INSTRUMENT_WIDTH;
const int PIANO_HEIGHT		= 84;
const int INSTRUMENT_WINDOW_CACHE_SIZE = 8;


// #### IT:
instrumentTrack::instrumentTrack( trackContainer * _tc ) :
	track( InstrumentTrack, _tc ),
	midiEventProcessor(),
	m_midiPort( engine::getMixer()->getMIDIClient()->addPort( this,
						tr( "unnamed_channel" ) ) ),
	m_audioPort( tr( "unnamed_channel" ), this ),
	m_notes(),
	m_baseNoteModel( 0, 0, KeysPerOctave * NumOctaves - 1, 1, this ),
        m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 1.0f, this ),
        m_surroundAreaModel( this, this ),
        m_effectChannelModel( 0, 0, NumFxChannels, 1, this ),
	m_instrument( NULL ),
	m_soundShaping( this ),
	m_arpeggiator( this ),
	m_chordCreator( this ),
	m_midiIO( this, m_midiPort ),
	m_piano( this )
{
	m_baseNoteModel.setTrack( this );
	m_baseNoteModel.setInitValue( DefaultKey );
	connect( &m_baseNoteModel, SIGNAL( dataChanged() ),
			this, SLOT( updateBaseNote() ) );

	m_volumeModel.setTrack( this );
	m_effectChannelModel.setTrack( this );


	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = NULL;
	}


	setName( tr( "Default" ) );

}




instrumentTrack::~instrumentTrack()
{
	delete m_instrument;
	engine::getMixer()->removePlayHandles( this );
	engine::getMixer()->getMIDIClient()->removePort( m_midiPort );
}




f_cnt_t instrumentTrack::beatLen( notePlayHandle * _n ) const
{
	if( m_instrument != NULL )
	{
		const f_cnt_t len = m_instrument->beatLen( _n );
		if( len > 0 )
		{
			return( len );
		}
	}
	return( m_soundShaping.envFrames() );
}






void instrumentTrack::processAudioBuffer( sampleFrame * _buf,
							const fpp_t _frames,
							notePlayHandle * _n )
{
	// we must not play the sound if this instrumentTrack is muted...
	if( muted() || ( _n && _n->bbTrackMuted() ) )
	{
		return;
	}
	float v_scale = (float) getVolume() / DefaultVolume;
	
	m_audioPort.getEffects()->startRunning();

	// instruments using instrument-play-handles will call this method
	// without any knowledge about notes, so they pass NULL for _n, which
	// is no problem for us since we just bypass the envelopes+LFOs
	if( _n != NULL )
	{
		m_soundShaping.processAudioBuffer( _buf, _frames, _n );
		v_scale *= ( (float) _n->getVolume() / DefaultVolume );
	}
	volumeVector v = m_surroundAreaModel.getVolumeVector( v_scale );

	m_audioPort.setNextFxChannel( m_effectChannelModel.value() );
	engine::getMixer()->bufferToPort( _buf,
		( _n != NULL ) ? tMin<f_cnt_t>(
				_n->framesLeftForCurrentPeriod(), _frames ) :
								_frames,
			( _n != NULL ) ? _n->offset() : 0, v, &m_audioPort );
}




void instrumentTrack::processInEvent( const midiEvent & _me,
					const midiTime & _time, bool _lock )
{
	switch( _me.m_type )
	{
		case NOTE_ON: 
			if( _me.velocity() > 0 )
			{
				if( m_notes[_me.key()] == NULL )
				{
					if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
					{
						m_piano.setKeyState(
							_me.key(), TRUE );
					}
					// create temporary note
					note n;
					n.setKey( _me.key() );
					n.setVolume( _me.velocity() * 100 /
									127 );
					// create (timed) note-play-handle
					notePlayHandle * nph = new
						notePlayHandle( this,
							_time.frames(
						engine::framesPerTact64th() ),
						valueRanges<f_cnt_t>::max() / 2,
									n );
					if( engine::getMixer()->addPlayHandle(
									nph ) )
					{
						m_notes[_me.key()] = nph;
					}
					return;
				}
				break;
			}

		case NOTE_OFF:
		{
			notePlayHandle * n = m_notes[_me.key()];
			if( n != NULL )
			{
				// create dummy-note which has the same length
				// as the played note for sending it later
				// to all slots connected to signal noteDone()
				// this is for example needed by piano-roll for
				// recording notes into a pattern
				note done_note(
					midiTime( static_cast<f_cnt_t>(
						n->totalFramesPlayed() /
						engine::framesPerTact64th() ) ),
					0, n->key(),
					n->getVolume(), n->getPanning() );
				if( _lock )
				{
					// lock our play-handle while calling noteOff()
					// for not modifying it's member variables
					// asynchronously (while rendering!)
					// which can result in segfaults
					engine::getMixer()->lock();
					n->noteOff();
					engine::getMixer()->unlock();
				}
				m_notes[_me.key()] = NULL;
				emit noteDone( done_note );
			}
			break;
		}

		case KEY_PRESSURE:
			if( !m_instrument->handleMidiEvent( _me, _time ) &&
						m_notes[_me.key()] != NULL )
			{
				m_notes[_me.key()]->setVolume( _me.velocity() *
								100 / 127 );
			}
			break;

		case PITCH_BEND:
			m_instrument->handleMidiEvent( _me, _time );
			break;

		default:
			printf( "instrument-track: unhandled MIDI-event %d\n",
								_me.m_type );
			break;
	}
}




void instrumentTrack::processOutEvent( const midiEvent & _me,
							const midiTime & _time )
{
	switch( _me.m_type )
	{
		case NOTE_ON:
			if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
			{
				m_piano.setKeyState( _me.key(), TRUE );
			}
			if( !configManager::inst()->value( "ui",
				"disablechannelactivityindicators" ).toInt() )
			{
				if( m_notes[_me.key()] != NULL )
				{
					return;
				}
				emit newNote();
			}
			break;

		case NOTE_OFF:
			if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
			{
				m_piano.setKeyState( _me.key(), FALSE );
			}
			break;

		default:
			break;
	}
	// if appropriate, midi-port does futher routing
	m_midiPort->processOutEvent( _me, _time );
}




void instrumentTrack::playNote( notePlayHandle * _n, bool _try_parallelizing )
{
	// arpeggio- and chord-widget has to do its work -> adding sub-notes
	// for chords/arpeggios
	m_chordCreator.processNote( _n );
	m_arpeggiator.processNote( _n );

	if( _n->arpBaseNote() == FALSE && m_instrument != NULL )
	{
		if( m_instrument->isMonophonic() )
		{
			constNotePlayHandleVector v =
				notePlayHandle::nphsOfInstrumentTrack( this,
									TRUE );
			if( v.size() > 1 )
			{
				constNotePlayHandleVector::iterator
						youngest_note = v.begin();
				for( constNotePlayHandleVector::iterator it =
						v.begin(); it != v.end(); ++it )
				{
					if( !( *it )->arpBaseNote() &&
						( *it )->totalFramesPlayed() <=
						( *youngest_note )->
							totalFramesPlayed() )
					{
						youngest_note = it;
					}
				}
				if( *youngest_note != _n &&
					!( *youngest_note )->arpBaseNote() &&
					!_n->released() )
				{
					processInEvent( midiEvent( NOTE_OFF, 0,
						_n->key(), 0 ), midiTime(),
									FALSE );
					if( ( *youngest_note )->offset() >
								_n->offset() )
					{
						_n->noteOff( ( *youngest_note )->
								offset() -
								_n->offset() );
					}
					else
					{
						// for the case the youngest
						// note has an offset smaller
						// then the current note we
						// already played everything
						// in last period and have
						// to clear parts of it
						_n->noteOff();
	engine::getMixer()->clearAudioBuffer( m_audioPort.firstBuffer(),
				engine::getMixer()->framesPerPeriod() -
					( *youngest_note )->offset(),
					( *youngest_note )->offset() );
						return;
					}
				}
			}
		}
		// all is done, so now lets play the note!
		m_instrument->playNote( _n, _try_parallelizing );
	}
}




QString instrumentTrack::instrumentName( void ) const
{
	if( m_instrument != NULL )
	{
		return( m_instrument->publicName() );
	}
	return( "" );
}




void instrumentTrack::deleteNotePluginData( notePlayHandle * _n )
{
	if( m_instrument != NULL && _n->m_pluginData != NULL )
	{
		m_instrument->deleteNotePluginData( _n );
	}

	// Notes deleted when keys still pressed
	if( m_notes[_n->key()] == _n )
	{
		note done_note( midiTime( static_cast<f_cnt_t>(
						_n->totalFramesPlayed() /
						engine::framesPerTact64th() ) ),
					0, _n->key(),
					_n->getVolume(), _n->getPanning() );
		_n->noteOff();
		m_notes[_n->key()] = NULL;
		emit noteDone( done_note );
	}
}




void instrumentTrack::setName( const QString & _new_name )
{
	// when changing name of track, also change name of those patterns,
	// which have the same name as the instrument-track
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		pattern * p = dynamic_cast<pattern *>( getTCO( i ) );
		if( ( p != NULL && p->name() == name() ) || p->name() == "" )
		{
			p->setName( _new_name );
		}
	}

	track::setName( _new_name );
	m_midiPort->setName( name() );
	m_audioPort.setName( name() );

	emit nameChanged();
}






void instrumentTrack::updateBaseNote( void )
{
	engine::getMixer()->lock();
	for( QList<notePlayHandle *>::iterator it = m_processHandles.begin();
					it != m_processHandles.end(); ++it )
	{
		( *it )->updateFrequency();
	}
	engine::getMixer()->unlock();
}




int instrumentTrack::masterKey( notePlayHandle * _n ) const
{
	int key = m_baseNoteModel.value() + engine::getSong()->masterPitch();
	return( tLimit<int>( _n->key() -
		( key - Key_A - DefaultOctave * KeysPerOctave ), 0, NumKeys ) );
}




bool FASTCALL instrumentTrack::play( const midiTime & _start,
					const fpp_t _frames,
					const f_cnt_t _offset,
							Sint16 _tco_num )
{
	float frames_per_tact64th = engine::framesPerTact64th();

	QList<trackContentObject *> tcos;
	bbTrack * bb_track;
	if( _tco_num >= 0 )
	{
		trackContentObject * tco = getTCO( _tco_num );
		tcos.push_back( tco );
		bb_track = bbTrack::findBBTrack( _tco_num );
		if( !( ( bb_track && bb_track->automationDisabled( this ) )
				|| dynamic_cast<pattern *>( tco )->empty() ) )
		{
			sendMidiTime( _start );
		}
	}
	else
	{
		getTCOsInRange( tcos, _start, _start + static_cast<Sint32>(
					_frames / frames_per_tact64th ) );
		bb_track = NULL;
		sendMidiTime( _start );
	}

	// Handle automation: detuning
	for( QList<notePlayHandle *>::iterator it = m_processHandles.begin();
					it != m_processHandles.end(); ++it )
	{
		( *it )->processMidiTime( _start );
	}

	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	bool played_a_note = FALSE;	// will be return variable

	for( QList<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
	{
		pattern * p = dynamic_cast<pattern *>( *it );
		// everything which is not a pattern or muted won't be played
		if( p == NULL || ( *it )->muted() )
		{
			continue;
		}
		midiTime cur_start = _start;
		if( _tco_num < 0 )
		{
			cur_start -= p->startPosition();
		}
		if( p->frozen() && engine::getSong()->exporting() == FALSE )
		{
			if( cur_start > 0 )
			{
				continue;
			}

			samplePlayHandle * handle = new samplePlayHandle( p );
			handle->setBBTrack( bb_track );
			handle->setOffset( _offset );
			// send it to the mixer
			engine::getMixer()->addPlayHandle( handle );
			played_a_note = TRUE;
			continue;
		}

		// get all notes from the given pattern...
		const noteVector & notes = p->notes();
		// ...and set our index to zero
		noteVector::const_iterator it = notes.begin();
#if SINGERBOT_SUPPORT
		int note_idx = 0;
#endif

		// very effective algorithm for playing notes that are
		// posated within the current sample-frame


		if( cur_start > 0 )
		{
			// skip notes which are posated before start-tact
			while( it != notes.end() && ( *it )->pos() < cur_start )
			{
#if SINGERBOT_SUPPORT
				if( ( *it )->length() != 0 )
				{
					++note_idx;
				}
#endif
				++it;
			}
		}

		note * cur_note;
		while( it != notes.end() &&
					( cur_note = *it )->pos() == cur_start )
		{
			if( cur_note->length() != 0 )
			{
				const f_cnt_t note_frames =
					cur_note->length().frames(
							frames_per_tact64th );

/*				// generate according MIDI-events
				processOutEvent( midiEvent( NOTE_ON,
						m_midiPort->outputChannel(),
							cur_note->key(),
							cur_note->getVolume() *
								128 / 100 ),
						midiTime::fromFrames(
							_offset,
							frames_per_tact64th ) );

				processOutEvent( midiEvent( NOTE_OFF,
						m_midiPort->outputChannel(),
							cur_note->key(), 0 ),
						midiTime::fromFrames(
							_offset+
								note_frames,
							frames_per_tact64th ) );
*/

				notePlayHandle * note_play_handle =
					new notePlayHandle( this, _offset,
								note_frames,
								*cur_note );
				note_play_handle->setBBTrack( bb_track );
#if SINGERBOT_SUPPORT
				note_play_handle->setPatternIndex( note_idx );
#endif
				engine::getMixer()->addPlayHandle(
							note_play_handle );
				played_a_note = TRUE;
#if SINGERBOT_SUPPORT
				++note_idx;
#endif
			}
			++it;
		}
	}
	return( played_a_note );
}




trackContentObject * instrumentTrack::createTCO( const midiTime & )
{
	return( new pattern( this ) );
}




trackView * instrumentTrack::createView( trackContainerView * _tcv )
{
	return( new instrumentTrackView( this, _tcv ) );
}




void instrumentTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "name", name() );
	m_volumeModel.saveSettings( _doc, _this, "vol" );

	m_surroundAreaModel.saveSettings( _doc, _this, "surpos" );

	m_effectChannelModel.saveSettings( _doc, _this, "fxch" );
	m_baseNoteModel.saveSettings( _doc, _this, "basenote" );

	if( m_instrument != NULL )
	{
		m_instrument->saveState( _doc, _this );
	}
	m_soundShaping.saveState( _doc, _this );
	m_chordCreator.saveState( _doc, _this );
	m_arpeggiator.saveState( _doc, _this );
	m_midiIO.saveState( _doc, _this );
	m_audioPort.getEffects()->saveState( _doc, _this );
	if( getHook() )
	{
		getHook()->saveSettings( _doc, _this );
	}
}




void instrumentTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	invalidateAllMyNPH();

	engine::getMixer()->lock();
	setName( _this.attribute( "name" ) );
	m_volumeModel.loadSettings( _this, "vol" );

	m_surroundAreaModel.loadSettings( _this, "surpos" );

	m_effectChannelModel.loadSettings( _this, "fxch" );

	if( _this.hasAttribute( "baseoct" ) )
	{
		// TODO: move this compat code to mmp.cpp -> upgrade()
		m_baseNoteModel.setInitValue( _this.
			attribute( "baseoct" ).toInt()
				* KeysPerOctave
				+ _this.attribute( "basetone" ).toInt() );
	}
	else
	{
		m_baseNoteModel.loadSettings( _this, "basenote" );
	}

	// clear effect-chain just in case we load an old preset without FX-data
	m_audioPort.getEffects()->clear();

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_soundShaping.nodeName() == node.nodeName() )
			{
				m_soundShaping.restoreState( node.toElement() );
			}
			else if( m_chordCreator.nodeName() == node.nodeName() )
			{
				m_chordCreator.restoreState( node.toElement() );
			}
			else if( m_arpeggiator.nodeName() == node.nodeName() )
			{
				m_arpeggiator.restoreState( node.toElement() );
			}
			else if( m_midiIO.nodeName() == node.nodeName() )
			{
				m_midiIO.restoreState( node.toElement() );
			}
			else if( m_audioPort.getEffects()->nodeName() ==
							node.nodeName() )
			{
				m_audioPort.getEffects()->restoreState(
							node.toElement() );
			}
			else if( automationPattern::classNodeName() !=
							node.nodeName() )
			{
				// if node-name doesn't match any known one,
				// we assume that it is an instrument-plugin
				// which we'll try to load
				delete m_instrument;
				m_instrument = NULL;
				m_instrument = instrument::instantiate(
							node.nodeName(), this );
				if( m_instrument->nodeName() ==
							node.nodeName() )
				{
					m_instrument->restoreState(
							node.toElement() );
				}
				emit instrumentChanged();
			}
		}
		node = node.nextSibling();
        }
	engine::getMixer()->unlock();

	if( getHook() )
	{
		getHook()->loadSettings( _this );
	}
}




instrument * instrumentTrack::loadInstrument( const QString & _plugin_name )
{
	invalidateAllMyNPH();

	engine::getMixer()->lock();
	delete m_instrument;
	m_instrument = instrument::instantiate( _plugin_name, this );
	engine::getMixer()->unlock();

	emit instrumentChanged();

	return( m_instrument );
}




void instrumentTrack::invalidateAllMyNPH( void )
{
	engine::getMixer()->lock();
	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = NULL;
	}

	// invalidate all note-play-handles linked to this channel
	m_processHandles.clear();
	engine::getMixer()->removePlayHandles( this );
	engine::getMixer()->unlock();
}





// #### ITV:


QQueue<instrumentTrackWindow *> instrumentTrackView::s_windows;



instrumentTrackView::instrumentTrackView( instrumentTrack * _it,
						trackContainerView * _tcv ) :
	trackView( _it, _tcv ),
	m_window( NULL )
{
	setAcceptDrops( TRUE );
	setFixedHeight( 32 );

	// creation of widgets for track-settings-widget
	m_tswVolumeKnob = new volumeKnob( knobSmall_17,
						getTrackSettingsWidget(),
						tr( "Volume" ) );
	m_tswVolumeKnob->setModel( &_it->m_volumeModel );
	m_tswVolumeKnob->setHintText( tr( "Volume:" ) + " ", "%" );
	m_tswVolumeKnob->move( 4, 4 );
	m_tswVolumeKnob->setLabel( tr( "VOL" ) );
	m_tswVolumeKnob->show();
	m_tswVolumeKnob->setWhatsThis( tr( volume_help ) );

	QPushButton * tsw_midi = new QPushButton(
				embed::getIconPixmap( "piano" ), QString::null,
						getTrackSettingsWidget() );
	tsw_midi->setGeometry( 32, 2, 28, 28 );
	tsw_midi->show();
	toolTip::add( tsw_midi, tr( "MIDI input/output" ) );
	m_tswMidiMenu = new QMenu( tsw_midi );
	tsw_midi->setMenu( m_tswMidiMenu );


	m_tswActivityIndicator = new fadeButton( QColor( 96, 96, 96 ),
						QColor( 255, 204, 0 ),
						getTrackSettingsWidget() );
	m_tswActivityIndicator->setGeometry( 212, 2, 8, 28 );
	m_tswActivityIndicator->show();
	connect( m_tswActivityIndicator, SIGNAL( pressed( void ) ),
				this, SLOT( activityIndicatorPressed() ) );
	connect( m_tswActivityIndicator, SIGNAL( released( void ) ),
				this, SLOT( activityIndicatorReleased() ) );


	m_tswInstrumentTrackButton = new instrumentTrackButton( this );
	m_tswInstrumentTrackButton->setCheckable( TRUE );
	m_tswInstrumentTrackButton->setGeometry( 64, 2, 144, 28 );
	m_tswInstrumentTrackButton->show();

/*	if( m_midiWidget->m_readablePorts )
	{
		m_midiInputAction = m_tswMidiMenu->addMenu(
					m_midiWidget->m_readablePorts );
	}
	else
	{
		m_midiInputAction = m_tswMidiMenu->addAction( "" );
		connect( m_midiInputAction, SIGNAL( changed() ), this,
						SLOT( midiInSelected() ) );
	}
	if( m_midiWidget->m_writeablePorts )
	{
		m_midiOutputAction = m_tswMidiMenu->addMenu(
					m_midiWidget->m_writeablePorts );
	}
	else
	{
		m_midiOutputAction = m_tswMidiMenu->addAction( "" );
		connect( m_midiOutputAction, SIGNAL( changed() ), this,
					SLOT( midiOutSelected() ) );
	}
	m_midiInputAction->setText( tr( "MIDI input" ) );
	m_midiOutputAction->setText( tr( "MIDI output" ) );
	if( m_midiWidget->m_readablePorts == NULL ||
					m_midiWidget->m_writeablePorts == NULL )
	{
		connect( m_midiWidget->m_sendCheckBox,
						SIGNAL( toggled( bool ) ),
				this, SLOT( midiConfigChanged( bool ) ) );
		connect( m_midiWidget->m_receiveCheckBox,
						SIGNAL( toggled( bool ) ),
				this, SLOT( midiConfigChanged( bool ) ) );
	}*/

	setModel( _it );

	connect( m_tswInstrumentTrackButton, SIGNAL( toggled( bool ) ),
			this, SLOT( toggledInstrumentTrackButton( bool ) ) );

}




instrumentTrackView::~instrumentTrackView()
{
	freeInstrumentTrackWindow();
}




// TODO: Add windows to free list on freeInstrumentTrackWindow. 
// But, don't NULL m_window or disconnect signals.  This will allow windows 
// that are being show/hidden frequently to stay connected.
void instrumentTrackView::freeInstrumentTrackWindow( void )
{
	if( m_window != NULL )
	{
		if( s_windows.count() < INSTRUMENT_WINDOW_CACHE_SIZE )
		{
			s_windows.enqueue( m_window );
		}
		else
		{
			delete m_window;
		}
		
		m_window = NULL;
	}
}




instrumentTrackWindow * instrumentTrackView::getInstrumentTrackWindow( void )
{
	if( m_window != NULL )
	{
	}
	else if( !s_windows.isEmpty() )
	{
		m_window = s_windows.dequeue();
		
		m_window->setInstrumentTrackView( this );
		m_window->setModel( model() );
		m_window->updateInstrumentView();
	}
	else
	{
		m_window = new instrumentTrackWindow( this );
	}
		
	return( m_window );
}




void instrumentTrackView::toggledInstrumentTrackButton( bool _on )
{
	getInstrumentTrackWindow()->toggledInstrumentTrackButton( _on );
	
	if( !_on )
	{
		freeInstrumentTrackWindow();
	}
}




void instrumentTrackView::activityIndicatorPressed( void )
{
	model()->processInEvent( midiEvent( NOTE_ON, 0, DefaultKey, 127 ),
								midiTime() );
}




void instrumentTrackView::activityIndicatorReleased( void )
{
	model()->processInEvent( midiEvent( NOTE_OFF, 0, DefaultKey, 0 ),
								midiTime() );
}





void instrumentTrackView::updateName( void )
{
#ifdef LMMS_DEBUG
	assert( m_tswInstrumentTrackButton != NULL );
#endif
	m_tswInstrumentTrackButton->setText( model()->name() );
}







// #### ITW:
instrumentTrackWindow::instrumentTrackWindow( instrumentTrackView * _itv ) :
	QWidget(),
	modelView( NULL ),
	m_track( _itv->model() ),
	m_itv( _itv ),
	m_instrumentView( NULL ),
	m_midiInputAction( NULL ),
	m_midiOutputAction( NULL )
{
	setAcceptDrops( TRUE );

	// init own layout + widgets
	setFocusPolicy( Qt::StrongFocus );
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setMargin( 0 );
	vlayout->setSpacing( 0 );

	m_generalSettingsWidget = new tabWidget( tr( "GENERAL SETTINGS" ),
									this );
	m_generalSettingsWidget->setFixedHeight( 90 );

	// setup line-edit for changing channel-name
	m_instrumentNameLE = new QLineEdit( m_generalSettingsWidget );
	m_instrumentNameLE->setFont( pointSize<8>(
						m_instrumentNameLE->font() ) );
	m_instrumentNameLE->setGeometry( 10, 16, 230, 18 );
	connect( m_instrumentNameLE, SIGNAL( textChanged( const QString & ) ),
				this, SLOT( textChanged( const QString & ) ) );


	// setup volume-knob
	m_volumeKnob = new volumeKnob( knobBright_26, m_generalSettingsWidget,
						tr( "Channel volume" ) );
	m_volumeKnob->move( 10, 44 );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->setLabel( tr( "VOL" ) );

	m_volumeKnob->setWhatsThis( tr( volume_help ) );


	// setup surround-area
	m_surroundArea = new surroundArea( m_generalSettingsWidget,
							tr( "Surround area" ) );
	m_surroundArea->move( 20 + m_volumeKnob->width(), 38 );
	m_surroundArea->show();
	m_surroundArea->setWhatsThis( tr( surroundarea_help ) );


	// setup spinbox for selecting FX-channel
	m_effectChannelNumber = new lcdSpinBox( 2, m_generalSettingsWidget,
						tr( "FX channel" ) );
	m_effectChannelNumber->setLabel( tr( "FX CHNL" ) );
	m_effectChannelNumber->move( m_surroundArea->x() +
					m_surroundArea->width() + 16, 40 );


	m_saveSettingsBtn = new QPushButton( embed::getIconPixmap(
							"project_save" ), "",
						m_generalSettingsWidget );
	m_saveSettingsBtn->setGeometry( m_effectChannelNumber->x() +
					m_effectChannelNumber->width() + 20, 40,
					32, 32 );
	connect( m_saveSettingsBtn, SIGNAL( clicked() ), this,
					SLOT( saveSettingsBtnClicked() ) );
	toolTip::add( m_saveSettingsBtn, 
		tr( "Save current channel settings in a preset-file" ) );
	m_saveSettingsBtn->setWhatsThis(
		tr( "Click here, if you want to save current channel settings "
			"in a preset-file. Later you can load this preset by "
			"double-clicking it in the preset-browser." ) );


	m_tabWidget = new tabWidget( "", this );
	m_tabWidget->setFixedHeight( INSTRUMENT_HEIGHT + 12 );


	// create tab-widgets
	m_ssView = new instrumentSoundShapingView( m_tabWidget );
	QWidget * instrument_functions = new QWidget( m_tabWidget );
	m_chordView = new chordCreatorView( &m_track->m_chordCreator,
							instrument_functions );
	m_arpView= new arpeggiatorView( &m_track->m_arpeggiator,
							instrument_functions );
	m_midiView = new instrumentMidiIOView( m_tabWidget );
	m_effectView = new effectRackView( m_track->m_audioPort.getEffects(),
								m_tabWidget );
	m_tabWidget->addTab( m_ssView, tr( "ENV/LFO" ), 1 );
	m_tabWidget->addTab( instrument_functions, tr( "FUNC" ), 2 );
	m_tabWidget->addTab( m_effectView, tr( "FX" ), 3 );
	m_tabWidget->addTab( m_midiView, tr( "MIDI" ), 4 );

	// setup piano-widget
	m_pianoView= new pianoView( this );
	m_pianoView->setFixedSize( INSTRUMENT_WIDTH, PIANO_HEIGHT );

	vlayout->addWidget( m_generalSettingsWidget );
	vlayout->addWidget( m_tabWidget );
	vlayout->addWidget( m_pianoView );

	if( m_midiView->m_readablePorts )
	{
		m_midiInputAction = m_itv->m_tswMidiMenu->addMenu(
						m_midiView->m_readablePorts );
	}
	else
	{
		m_midiInputAction = m_itv->m_tswMidiMenu->addAction( "" );
		connect( m_midiInputAction, SIGNAL( changed() ), this,
						SLOT( midiInSelected() ) );
	}
	if( m_midiView->m_writeablePorts )
	{
		m_midiOutputAction = m_itv->m_tswMidiMenu->addMenu(
						m_midiView->m_writeablePorts );
	}
	else
	{
		m_midiOutputAction = m_itv->m_tswMidiMenu->addAction( "" );
		connect( m_midiOutputAction, SIGNAL( changed() ), this,
					SLOT( midiOutSelected() ) );
	}
	m_midiInputAction->setText( tr( "MIDI input" ) );
	m_midiOutputAction->setText( tr( "MIDI output" ) );
	if( m_midiView->m_readablePorts == NULL ||
					m_midiView->m_writeablePorts == NULL )
	{
		connect( m_midiView->m_sendCheckBox,
						SIGNAL( toggled( bool ) ),
				this, SLOT( midiConfigChanged( bool ) ) );
		connect( m_midiView->m_receiveCheckBox,
						SIGNAL( toggled( bool ) ),
				this, SLOT( midiConfigChanged( bool ) ) );
	}

	setModel( _itv->model() );

	// set window-icon
	setWindowIcon( embed::getIconPixmap( "instrument_track" ) );

	updateInstrumentView();

	setFixedWidth( INSTRUMENT_WIDTH );
	resize( sizeHint() );

	if( engine::getMainWindow()->workspace() )
	{
		engine::getMainWindow()->workspace()->addSubWindow( this );
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
}




instrumentTrackWindow::~instrumentTrackWindow()
{
	delete m_instrumentView;
	if( engine::getMainWindow()->workspace() )
	{
		parentWidget()->hide();
		parentWidget()->deleteLater();
	}
}




void instrumentTrackWindow::modelChanged( void )
{
	m_track = m_itv->model();

	m_instrumentNameLE->setText( m_track->name() );

	disconnect( m_track, SIGNAL( nameChanged() ) );
	disconnect( m_track, SIGNAL( instrumentChanged() ) );

	connect( m_track, SIGNAL( nameChanged() ),
			this, SLOT( updateName() ) );
	connect( m_track, SIGNAL( instrumentChanged() ),
			this, SLOT( updateInstrumentView() ),
			Qt::QueuedConnection );
	
	m_volumeKnob->setModel( &m_track->m_volumeModel );
	m_surroundArea->setModel( &m_track->m_surroundAreaModel );
	m_effectChannelNumber->setModel( &m_track->m_effectChannelModel );
	m_pianoView->setModel( &m_track->m_piano );

	m_ssView->setModel( &m_track->m_soundShaping );
	m_chordView->setModel( &m_track->m_chordCreator );
	m_arpView->setModel( &m_track->m_arpeggiator );
	m_midiView->setModel( &m_track->m_midiIO );
	updateName();
}




void instrumentTrackWindow::saveSettingsBtnClicked( void )
{
	QFileDialog sfd( this, tr( "Save channel-settings in file" ), "",
				tr( "Channel-Settings-File (*.cs.xml)" ) );

	QString preset_root = configManager::inst()->userPresetsDir();
	if( !QDir( preset_root ).exists() )
	{
		QDir().mkdir( preset_root );
	}
	if( !QDir( preset_root + m_track->instrumentName() ).exists() )
	{
		QDir( preset_root ).mkdir( m_track->instrumentName() );
	}

	sfd.setDirectory( preset_root + m_track->instrumentName() );
	sfd.setFileMode( QFileDialog::AnyFile );
	if( sfd.exec () == QDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != ""
	)
	{
		multimediaProject mmp(
				multimediaProject::InstrumentTrackSettings );
		QDomElement _this = mmp.createElement( m_track->nodeName() );
		m_track->saveTrackSpecificSettings( mmp, _this );
		mmp.content().appendChild( _this );
		QString f = sfd.selectedFiles()[0];
		mmp.writeFile( f );
	}
}





void instrumentTrackWindow::updateName( void )
{
	setWindowTitle( m_track->name() );

	if( m_instrumentNameLE->text() != m_track->name() )
	{
		m_instrumentNameLE->setText( m_track->name() );
	}
}





void instrumentTrackWindow::updateInstrumentView( void )
{
	delete m_instrumentView;
	if( m_track->m_instrument != NULL )
	{
		m_instrumentView = m_track->m_instrument->createView(
								m_tabWidget );
		m_tabWidget->addTab( m_instrumentView, tr( "PLUGIN" ), 0 );
		m_tabWidget->setActiveTab( 0 );
	}

//	m_tswInstrumentTrackButton->update();
}




void instrumentTrackWindow::textChanged( const QString & _new_name )
{
	m_track->setName( _new_name );
	engine::getSong()->setModified();
}




void instrumentTrackWindow::toggledInstrumentTrackButton( bool _on )
{

	if( _on )
	{
		if( engine::getMainWindow()->workspace() )
		{
			show();
			parentWidget()->show();
			parentWidget()->raise();
		}
		else
		{
			show();
			raise();
		}
	}
	else
	{
		if( engine::getMainWindow()->workspace() )
		{
			parentWidget()->hide();
		}
		else
		{
			hide();
		}
	}
}




void instrumentTrackWindow::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	if( engine::getMainWindow()->workspace() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}
	m_itv->m_tswInstrumentTrackButton->setChecked( FALSE );
}




void instrumentTrackWindow::focusInEvent( QFocusEvent * )
{
	m_pianoView->setFocus();
}




void instrumentTrackWindow::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "instrument,presetfile" );
}




void instrumentTrackWindow::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "instrument" )
	{
		m_track->loadInstrument( value );
		engine::getSong()->setModified();
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		multimediaProject mmp( value );
		m_track->loadTrackSpecificSettings( mmp.content().firstChild().
								toElement() );
		engine::getSong()->setModified();
		_de->accept();
	}
}




void instrumentTrackWindow::saveSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "tab", m_tabWidget->activeTab() );
	mainWindow::saveWidgetState( this, _this );
}




void instrumentTrackWindow::loadSettings( const QDomElement & _this )
{
	m_tabWidget->setActiveTab( _this.attribute( "tab" ).toInt() );
	mainWindow::restoreWidgetState( this, _this );
	if( isVisible() )
	{
		m_itv->m_tswInstrumentTrackButton->setChecked( TRUE );
	}
}



void instrumentTrackWindow::midiInSelected( void )
{
	m_midiInputAction->setChecked( !m_midiInputAction->isChecked() );
	m_midiView->m_receiveCheckBox->setChecked(
					m_midiInputAction->isChecked() );
}



void instrumentTrackWindow::midiOutSelected( void )
{
	m_midiOutputAction->setChecked( !m_midiOutputAction->isChecked() );
	m_midiView->m_sendCheckBox->setChecked(
					m_midiOutputAction->isChecked() );
}




void instrumentTrackWindow::midiConfigChanged( bool )
{
	m_midiInputAction->setChecked(
			m_midiView->m_receiveCheckBox->model()->value() );
	m_midiOutputAction->setChecked(
			m_midiView->m_sendCheckBox->model()->value() );
}






instrumentTrackButton::instrumentTrackButton( instrumentTrackView * _itv ) :
	QPushButton( _itv->getTrackSettingsWidget() ),
	m_instrumentTrackView( _itv )
{
	setAcceptDrops( TRUE );
}




instrumentTrackButton::~instrumentTrackButton()
{
}




void instrumentTrackButton::paintEvent( QPaintEvent * _pe )
{
	QPushButton::paintEvent( _pe );
	QPainter p( this );
	const QString in = m_instrumentTrackView->model()->instrumentName() +
									":";
	const QString n = m_instrumentTrackView->model()->name();
	int extra = isChecked() ? -1 : -3;
	p.setFont( pointSize<7>( p.font() ) );
	p.setPen( QApplication::palette().buttonText().color() );
	p.drawText( ( width() - QFontMetrics( p.font() ).width( in ) ) / 2 +
					extra, height() / 2 + extra, in );
	p.setPen( QColor( 0, 0, 0 ) );
	p.drawText( ( width() - QFontMetrics( p.font() ).width( n ) ) /
				2 + extra, height() / 2 +
				QFontMetrics( p.font() ).height() + extra, n );
}




void instrumentTrackButton::dragEnterEvent( QDragEnterEvent * _dee )
{
	m_instrumentTrackView->getInstrumentTrackWindow()->
							dragEnterEvent( _dee );
}




void instrumentTrackButton::dropEvent( QDropEvent * _de )
{
	m_instrumentTrackView->getInstrumentTrackWindow()->dropEvent( _de );
	setChecked( TRUE );
}




#include "instrument_track.moc"


#endif
