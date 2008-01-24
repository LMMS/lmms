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
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMdiArea>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>


#include "arp_and_chords_tab_widget.h"
#include "audio_port.h"
#include "automatable_model_templates.h"
#include "automation_pattern.h"
#include "config_mgr.h"
#include "debug.h"
#include "effect_board.h"
#include "effect_chain.h"
#include "effect_rack_view.h"
#include "embed.h"
#include "engine.h"
#include "envelope_tab_widget.h"
#include "fade_button.h"
#include "gui_templates.h"
#include "instrument.h"
#include "led_checkbox.h"
#include "main_window.h"
#include "midi_client.h"
#include "midi_port.h"
#include "midi_tab_widget.h"
#include "mmp.h"
#include "note_play_handle.h"
#include "pattern.h"
#include "piano_widget.h"
#include "plugin_view.h"
#include "sample_play_handle.h"
#include "song_editor.h"
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


instrumentTrack::instrumentTrack( trackContainer * _tc ) :
	track( _tc ),
	midiEventProcessor(),
	m_trackType( INSTRUMENT_TRACK ),
	m_midiPort( engine::getMixer()->getMIDIClient()->addPort( this,
						tr( "unnamed_channel" ) ) ),
	m_audioPort( tr( "unnamed_channel" ), this ),
	m_notes(),
	m_baseNoteModel( 0, 0, NOTES_PER_OCTAVE * OCTAVES - 1, 1/* this */ ),
        m_volumeModel( DEFAULT_VOLUME, MIN_VOLUME, MAX_VOLUME,
							1.0f /* this */ ),
        m_surroundAreaModel( NULL /* this */, this ),
        m_effectChannelModel( DEFAULT_EFFECT_CHANNEL,
					MIN_EFFECT_CHANNEL, MAX_EFFECT_CHANNEL
								 /* this */ ),
//	m_effects( /* this */ NULL ),
	m_instrument( NULL ),
	m_instrumentView( NULL ),
	m_midiInputAction( NULL ),
	m_midiOutputAction( NULL )
{
	m_baseNoteModel.setTrack( this );
	m_baseNoteModel.setInitValue( DEFAULT_OCTAVE * NOTES_PER_OCTAVE + A );
	connect( &m_baseNoteModel, SIGNAL( dataChanged() ),
					this, SLOT( updateBaseNote() ) );

	m_volumeModel.setTrack( this );
	m_effectChannelModel.setTrack( this );


	for( int i = 0; i < NOTES; ++i )
	{
		m_notes[i] = NULL;
	}


	setAcceptDrops( TRUE );

	getTrackWidget()->setFixedHeight( 32 );


	// creation of widgets for track-settings-widget
	m_tswVolumeKnob = new volumeKnob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Channel volume" ) );
	m_tswVolumeKnob->setModel( &m_volumeModel );
	m_tswVolumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
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
	connect( m_tswInstrumentTrackButton, SIGNAL( toggled( bool ) ), this,
				SLOT( toggledInstrumentTrackButton( bool ) ) );



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
	m_volumeKnob->setModel( &m_volumeModel );
	m_volumeKnob->move( 10, 44 );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->setLabel( tr( "VOLUME" ) );

	m_volumeKnob->setWhatsThis( tr( volume_help ) );
	//volumeKnob::linkObjects( m_tswVolumeKnob, m_volumeKnob );


	// setup surround-area
	m_surroundArea = new surroundArea( m_generalSettingsWidget,
							tr( "Surround area" ) );
	m_surroundArea->setModel( &m_surroundAreaModel );
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


	setName( tr( "Default" ) );


	m_tabWidget = new tabWidget( "", this );
	m_tabWidget->setFixedHeight( INSTRUMENT_HEIGHT + 12 );


	// create other tab-widgets
	m_envWidget = new envelopeTabWidget( this );
	m_arpWidget = new arpAndChordsTabWidget( this );
	m_midiWidget = new midiTabWidget( this, m_midiPort );
	m_effectRack = new effectRackView( m_audioPort.getEffects(),
								m_tabWidget );
	m_tabWidget->addTab( m_envWidget, tr( "ENV/LFO/FILTER" ), 1 );
	m_tabWidget->addTab( m_arpWidget, tr( "ARP/CHORD" ), 2 );
	m_tabWidget->addTab( m_effectRack, tr( "FX" ), 3 );
	m_tabWidget->addTab( m_midiWidget, tr( "MIDI" ), 4 );

	// setup piano-widget
	m_pianoWidget = new pianoWidget( this );
	m_pianoWidget->setFixedSize( INSTRUMENT_WIDTH, PIANO_HEIGHT );


	vlayout->addWidget( m_generalSettingsWidget );
	vlayout->addWidget( m_tabWidget );
	vlayout->addWidget( m_pianoWidget );

	if( m_midiWidget->m_readablePorts )
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
	}


	// set window-icon
	setWindowIcon( embed::getIconPixmap( "instrument_track" ) );


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




instrumentTrack::~instrumentTrack()
{
	engine::getMixer()->removePlayHandles( this );
	delete m_effectRack;
	engine::getMixer()->getMIDIClient()->removePort( m_midiPort );

	if( engine::getMainWindow()->workspace() )
	{
		parentWidget()->hide();
		parentWidget()->deleteLater();
	}
}




void instrumentTrack::saveSettingsBtnClicked( void )
{
	QFileDialog sfd( this, tr( "Save channel-settings in file" ), "",
				tr( "Channel-Settings-File (*.cs.xml)" ) );

	QString preset_root = configManager::inst()->userPresetsDir();
	if( !QDir( preset_root ).exists() )
	{
		QDir().mkdir( preset_root );
	}
	if( !QDir( preset_root + instrumentName() ).exists() )
	{
		QDir( preset_root ).mkdir( instrumentName() );
	}

	sfd.setDirectory( preset_root + instrumentName() );
	sfd.setFileMode( QFileDialog::AnyFile );
	if( sfd.exec () == QDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != ""
	)
	{
		multimediaProject mmp(
				multimediaProject::INSTRUMENT_TRACK_SETTINGS );
		QDomElement _this = mmp.createElement( nodeName() );
		saveTrackSpecificSettings( mmp, _this );
		mmp.content().appendChild( _this );
		QString f = sfd.selectedFiles()[0];
		mmp.writeFile( f );
	}
}




void instrumentTrack::activityIndicatorPressed( void )
{
	processInEvent( midiEvent( NOTE_ON, 0,
					DEFAULT_OCTAVE * NOTES_PER_OCTAVE + A,
					127 ), midiTime() );
}




void instrumentTrack::activityIndicatorReleased( void )
{
	processInEvent( midiEvent( NOTE_OFF, 0,
					DEFAULT_OCTAVE * NOTES_PER_OCTAVE + A,
					0 ), midiTime() );
}




void instrumentTrack::midiInSelected( void )
{
	m_midiInputAction->setChecked( !m_midiInputAction->isChecked() );
	m_midiWidget->m_receiveCheckBox->setChecked(
					m_midiInputAction->isChecked() );
}



void instrumentTrack::midiOutSelected( void )
{
	m_midiOutputAction->setChecked( !m_midiOutputAction->isChecked() );
	m_midiWidget->m_sendCheckBox->setChecked(
					m_midiOutputAction->isChecked() );
}




void instrumentTrack::midiConfigChanged( bool )
{
	m_midiInputAction->setChecked(
				m_midiWidget->m_receiveCheckBox->model()->value() );
	m_midiOutputAction->setChecked(
				m_midiWidget->m_sendCheckBox->model()->value() );
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
	return( m_envWidget->envFrames() );
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
	float v_scale = (float) getVolume() / DEFAULT_VOLUME;
	
	m_audioPort.getEffects()->startRunning();

	// instruments using instrument-play-handles will call this method
	// without any knowledge about notes, so they pass NULL for _n, which
	// is no problem for us since we just bypass the envelopes+LFOs
	if( _n != NULL )
	{
		m_envWidget->processAudioBuffer( _buf, _frames, _n );
		v_scale *= ( (float) _n->getVolume() / DEFAULT_VOLUME );
	}
	volumeVector v = m_surroundArea->model()->getVolumeVector( v_scale );

	engine::getMixer()->bufferToPort( _buf,
		( _n != NULL ) ? tMin<f_cnt_t>( _n->framesLeftForCurrentPeriod(), _frames ) :
											_frames,
			( _n != NULL ) ? _n->offset() : 0, v, &m_audioPort );
}




void instrumentTrack::processInEvent( const midiEvent & _me,
							const midiTime & _time )
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
						m_pianoWidget->setKeyState(
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
					0, n->tone(), n->octave(),
					n->getVolume(), n->getPanning() );
				// lock our play-handle while calling noteOff()
				// for not modifying it's member variables
				// asynchronously (while rendering!)
				// which can result in segfaults
				engine::getMixer()->lock();
				n->noteOff();
				engine::getMixer()->unlock();
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

/*		case PITCH_BEND:
			if( m_pitchBendKnob != NULL )
			{
				float range = tAbs(
						m_pitchBendKnob->maxValue() -
						m_pitchBendKnob->minValue() );
				m_pitchBendKnob->setValue(
						m_pitchBendKnob->minValue() +
						_me.m_data.m_param[0] *
								range / 16384 );
			}
			break;*/

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
				m_pianoWidget->setKeyState( _me.key(), TRUE );
			}
			if( !configManager::inst()->value( "ui",
				"disablechannelactivityindicators" ).toInt() )
			{
				if( m_notes[_me.key()] != NULL )
				{
					return;
				}
				m_tswActivityIndicator->activate();
			}
			break;

		case NOTE_OFF:
			if( !configManager::inst()->value( "ui",
						"manualchannelpiano" ).toInt() )
			{
				m_pianoWidget->setKeyState( _me.key(), FALSE );
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
	m_arpWidget->processNote( _n );

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
						_n->key(), 0 ), midiTime() );
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
				// we do not play previously released notes
				// anymore
				else if( *youngest_note != _n &&
							_n->released() )
				{
					//return;
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
					0, _n->tone(), _n->octave(),
					_n->getVolume(), _n->getPanning() );
		_n->noteOff();
		m_notes[_n->key()] = NULL;
		emit noteDone( done_note );
	}
}




void instrumentTrack::setName( const QString & _new_name )
{
	// when changing name of channel, also change name of those patterns,
	// which have the same name as the channel
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		pattern * p = dynamic_cast<pattern *>( getTCO( i ) );
		if( ( p != NULL && p->name() == m_name ) || p->name() == "" )
		{
			p->setName( _new_name );
		}
	}

	m_name = _new_name;
	setWindowTitle( m_name );

	if( m_instrumentNameLE->text() != _new_name )
	{
		m_instrumentNameLE->setText( m_name );
	}
#ifdef LMMS_DEBUG
	assert( m_tswInstrumentTrackButton != NULL );
#endif
	m_tswInstrumentTrackButton->setText( m_name );
	m_midiPort->setName( m_name );
	m_audioPort.setName( m_name );
}






void instrumentTrack::updateBaseNote( /* bool _modified*/ void )
{
	engine::getMixer()->lock();
	for( QList<notePlayHandle *>::iterator it = m_processHandles.begin();
					it != m_processHandles.end(); ++it )
	{
		( *it )->updateFrequency();
	}
	engine::getMixer()->unlock();
/*
	if( _modified )
	{
		engine::getSongEditor()->setModified();
	}*/
}




int instrumentTrack::masterKey( notePlayHandle * _n ) const
{
	int key = m_baseNoteModel.value() +
					engine::getSongEditor()->masterPitch();
	return( tLimit<int>( _n->key() -
		( key - A - DEFAULT_OCTAVE * NOTES_PER_OCTAVE ), 0, NOTES ) );
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
		if( p->frozen() &&
				engine::getSongEditor()->exporting() == FALSE )
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




void instrumentTrack::saveTrackSpecificSettings( QDomDocument & _doc,
							QDomElement & _this )
{
	_this.setAttribute( "name", name() );
	m_volumeModel.saveSettings( _doc, _this, "vol" );

	m_surroundAreaModel.saveSettings( _doc, _this, "surpos" );

	m_effectChannelModel.saveSettings( _doc, _this, "fxch" );
	m_baseNoteModel.saveSettings( _doc, _this, "basenote" );
	_this.setAttribute( "tab", m_tabWidget->activeTab() );

	mainWindow::saveWidgetState( this, _this );

	if( m_instrument != NULL )
	{
		m_instrument->saveState( _doc, _this );
	}
	m_envWidget->saveState( _doc, _this );
	m_arpWidget->saveState( _doc, _this );
	m_midiWidget->saveState( _doc, _this );
	m_audioPort.getEffects()->saveState( _doc, _this );
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
				* NOTES_PER_OCTAVE
				+ _this.attribute( "basetone" ).toInt() );
	}
	else
	{
		m_baseNoteModel.loadSettings( _this, "basenote" );
	}

	int tab = _this.attribute( "tab" ).toInt();

	bool had_fx = FALSE;

	QDomNode node = _this.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_envWidget->nodeName() == node.nodeName() )
			{
				m_envWidget->restoreState( node.toElement() );
			}
			else if( m_arpWidget->nodeName() == node.nodeName() )
			{
				m_arpWidget->restoreState( node.toElement() );
			}
			else if( m_midiWidget->nodeName() == node.nodeName() )
			{
				m_midiWidget->restoreState( node.toElement() );
			}
			else if( m_audioPort.getEffects()->nodeName() ==
							node.nodeName() )
			{
				m_audioPort.getEffects()->restoreState(
							node.toElement() );
				had_fx = TRUE;
			}
			else if( automationPattern::classNodeName()
							!= node.nodeName() )
			{
				// if node-name doesn't match any known one,
				// we assume that it is an instrument-plugin
				// which we'll try to load
				delete m_instrumentView;
				delete m_instrument;
				m_instrument = instrument::instantiate(
							node.nodeName(), this );
				if( m_instrument->nodeName() ==
							node.nodeName() )
				{
					m_instrument->restoreState(
							node.toElement() );
				}
				m_instrumentView = m_instrument->
						createView( m_tabWidget );
				m_tabWidget->addTab( m_instrumentView,
							tr( "PLUGIN" ), 0 );
			}
		}
		node = node.nextSibling();
        }
	// TODO: why not move above without any condition??
	if( !had_fx )
	{
		m_audioPort.getEffects()->deleteAllPlugins();
	}
	engine::getMixer()->unlock();

	m_tabWidget->setActiveTab( tab );

	m_pianoWidget->update();

	mainWindow::restoreWidgetState( this, _this );
	if( isVisible() == TRUE )
	{
		m_tswInstrumentTrackButton->setChecked( TRUE );
	}
}




instrument * instrumentTrack::loadInstrument( const QString & _plugin_name )
{
	invalidateAllMyNPH();

	engine::getMixer()->lock();
	delete m_instrumentView;
	delete m_instrument;
	m_instrument = instrument::instantiate( _plugin_name, this );
	engine::getMixer()->unlock();

	m_instrumentView = m_instrument->createView( m_tabWidget );
	m_tabWidget->addTab( m_instrumentView, tr( "PLUGIN" ), 0 );
	m_tabWidget->setActiveTab( 0 );

	m_tswInstrumentTrackButton->update();

	return( m_instrument );
}




void instrumentTrack::textChanged( const QString & _new_name )
{
	setName( _new_name );
	engine::getSongEditor()->setModified();
}




void instrumentTrack::toggledInstrumentTrackButton( bool _on )
{
	if( m_tswInstrumentTrackButton->isChecked() != _on )
	{
		m_tswInstrumentTrackButton->setChecked( _on );
	}
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




void instrumentTrack::closeEvent( QCloseEvent * _ce )
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
	m_tswInstrumentTrackButton->setChecked( FALSE );
}




void instrumentTrack::focusInEvent( QFocusEvent * )
{
	m_pianoWidget->setFocus();
}




void instrumentTrack::dragEnterEvent( QDragEnterEvent * _dee )
{
	stringPairDrag::processDragEnterEvent( _dee, "instrument,presetfile" );
}




void instrumentTrack::dropEvent( QDropEvent * _de )
{
	QString type = stringPairDrag::decodeKey( _de );
	QString value = stringPairDrag::decodeValue( _de );
	if( type == "instrument" )
	{
		loadInstrument( value );
		engine::getSongEditor()->setModified();
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		multimediaProject mmp( value );
		loadTrackSpecificSettings( mmp.content().firstChild().
								toElement() );
		engine::getSongEditor()->setModified();
		_de->accept();
	}
}




void instrumentTrack::invalidateAllMyNPH( void )
{
	engine::getMixer()->lock();
	for( int i = 0; i < NOTES; ++i )
	{
		m_notes[i] = NULL;
	}

	// invalidate all note-play-handles linked to this channel
	m_processHandles.clear();
	engine::getMixer()->removePlayHandles( this );
	engine::getMixer()->unlock();
}









instrumentTrackButton::instrumentTrackButton( instrumentTrack *
							_instrument_track ) :
	QPushButton( _instrument_track->getTrackSettingsWidget() ),
	m_instrumentTrack( _instrument_track )
{
	setAcceptDrops( TRUE );
}




instrumentTrackButton::~instrumentTrackButton()
{
}




void instrumentTrackButton::drawButtonLabel( QPainter * _p )
{
	QString in = m_instrumentTrack->instrumentName() + ":";
	int extra = isChecked() ? -1 : -3;
	_p->setFont( pointSize<7>( _p->font() ) );
	_p->setPen( QApplication::palette().buttonText().color() );
	_p->drawText( ( width() - QFontMetrics( _p->font() ).width( in ) ) / 2 +
					extra, height() / 2 + extra, in );
	_p->setPen( QColor( 0, 0, 0 ) );
	_p->drawText( ( width() - QFontMetrics( _p->font() ).width( text() ) ) /
				2 + extra, height() / 2 +
				QFontMetrics( _p->font() ).height() + extra,
								text() );
}




void instrumentTrackButton::dragEnterEvent( QDragEnterEvent * _dee )
{
	m_instrumentTrack->dragEnterEvent( _dee );
}




void instrumentTrackButton::dropEvent( QDropEvent * _de )
{
	m_instrumentTrack->dropEvent( _de );
	setChecked( TRUE );
}



#include "instrument_track.moc"


#endif
