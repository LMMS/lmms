#ifndef SINGLE_SOURCE_COMPILE

/*
 * instrument_track.cpp - implementation of instrument-track-class
 *                        (window + data-structures)
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <Qt/QtXml>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>

#else

#include <qfiledialog.h>
#include <qdir.h>
#include <qfile.h>
#include <qapplication.h>
#include <qdom.h>
#include <qlineedit.h>
#include <qbuttongroup.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qpopupmenu.h>
#include <qwhatsthis.h>

#endif


#include "instrument_track.h"
#include "pattern.h"
#include "main_window.h"
#include "song_editor.h"
#include "effect_board.h"
#include "envelope_tab_widget.h"
#include "arp_and_chords_tab_widget.h"
#include "instrument.h"
#include "audio_port.h"
#include "midi_client.h"
#include "midi_port.h"
#include "midi_tab_widget.h"
#include "note_play_handle.h"
#include "embed.h"
#include "fade_button.h"
#include "lcd_spinbox.h"
#include "led_checkbox.h"
#include "piano_widget.h"
#include "surround_area.h"
#include "tooltip.h"
#include "tab_widget.h"
#include "buffer_allocator.h"
#include "config_mgr.h"
#include "debug.h"
#include "mmp.h"
#include "string_pair_drag.h"
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


const int CHANNEL_WIDTH		= 250;
const int INSTRUMENT_WIDTH	= CHANNEL_WIDTH;
const int INSTRUMENT_HEIGHT	= INSTRUMENT_WIDTH;
const int PIANO_HEIGHT		= 84;


instrumentTrack::instrumentTrack( trackContainer * _tc ) :
	QWidget( _tc->eng()->getMainWindow()->workspace() ),
 	track( _tc ),
	midiEventProcessor(),
	m_trackType( CHANNEL_TRACK ),
	m_midiPort( eng()->getMixer()->getMIDIClient()->addPort( this,
						tr( "unnamed_channel" ) ) ),
	m_audioPort( new audioPort( tr( "unnamed_channel" ), eng() ) ),
	m_notes(),
	m_notesMutex(),
	m_baseTone( A ),
	m_baseOctave( OCTAVE_4 ),
	m_instrument( NULL ),
#ifdef QT4
	m_midiInputAction( NULL ),
	m_midiOutputAction( NULL )
#else
	m_midiInputID( -1 ),
	m_midiOutputID( -1 )
#endif
{
	m_notesMutex.lock();
	for( int i = 0; i < NOTES_PER_OCTAVE * OCTAVES; ++i )
	{
		m_notes[i] = NULL;
	}
	m_notesMutex.unlock();


#ifdef QT4
	eng()->getMainWindow()->workspace()->addWindow( this );
#endif

	setAcceptDrops( TRUE );

	hide();

	getTrackWidget()->setFixedHeight( 32 );


	// creation of widgets for track-settings-widget
	m_tswVolumeKnob = new volumeKnob( knobSmall_17, getTrackSettingsWidget(),
					tr( "Channel volume" ), eng(), this );
	m_tswVolumeKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
	m_tswVolumeKnob->setInitValue( DEFAULT_VOLUME );
	m_tswVolumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_tswVolumeKnob->move( 4, 4 );
	m_tswVolumeKnob->setLabel( tr( "VOL" ) );
	m_tswVolumeKnob->show();
/*	connect( m_tswVolumeKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( volValueChanged( float ) ) );*/
#ifdef QT4
	m_tswVolumeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_tswVolumeKnob,
#endif
		tr( volume_help ) );

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
#ifdef QT4
	setFocusPolicy( Qt::StrongFocus );
#else
	setFocusPolicy( StrongFocus );
#endif
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
					tr( "Channel volume" ), eng(), this );
	m_volumeKnob->move( 10, 44 );
	m_volumeKnob->setRange( MIN_VOLUME, MAX_VOLUME, 1.0f );
	m_volumeKnob->setInitValue( DEFAULT_VOLUME );
	m_volumeKnob->setHintText( tr( "Channel volume:" ) + " ", "%" );
	m_volumeKnob->setLabel( tr( "VOLUME" ) );

#ifdef QT4
	m_volumeKnob->setWhatsThis(
#else
	QWhatsThis::add( m_volumeKnob,
#endif
		tr( volume_help ) );
/*	connect( m_volumeKnob, SIGNAL( valueChanged( float ) ), this,
					SLOT( volValueChanged( float ) ) );*/
	volumeKnob::linkObjects( m_tswVolumeKnob, m_volumeKnob );


	// setup surround-area
	m_surroundArea = new surroundArea( m_generalSettingsWidget,
					tr( "Surround area" ), eng(), this );
	m_surroundArea->move( 20 + m_volumeKnob->width(), 38 );
	m_surroundArea->show();
#ifdef QT4
	m_surroundArea->setWhatsThis(
#else
	QWhatsThis::add( m_surroundArea,
#endif
		tr( surroundarea_help ) );

	connect( m_surroundArea, SIGNAL( valueChanged( const QPoint & ) ),
			this,
			SLOT( surroundAreaPosChanged( const QPoint & ) ) );



	// setup spinbox for selecting FX-channel
	m_effectChannelNumber = new lcdSpinBox( MIN_EFFECT_CHANNEL,
						MAX_EFFECT_CHANNEL, 2,
						m_generalSettingsWidget,
						eng() );
	m_effectChannelNumber->setInitValue( DEFAULT_EFFECT_CHANNEL );
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
#ifdef QT4
	m_saveSettingsBtn->setWhatsThis(
#else
	QWhatsThis::add( m_saveSettingsBtn,
#endif
		tr( "Click here, if you want to save current channel settings "
			"in a preset-file. Later you can load this preset by "
			"double-clicking it in the preset-browser." ) );


	setVolume( DEFAULT_VOLUME );
	setSurroundAreaPos( QPoint() );
	setName( tr( "Default" ) );


	m_tabWidget = new tabWidget( "", this );
	m_tabWidget->setFixedHeight( INSTRUMENT_HEIGHT + 12 );


	// create other tab-widgets
	m_envWidget = new envelopeTabWidget( this );
	m_arpWidget = new arpAndChordsTabWidget( this );
	m_midiWidget = new midiTabWidget( this, m_midiPort );
	m_tabWidget->addTab( m_envWidget, tr( "ENV/LFO/FILTER" ), 1 );
	m_tabWidget->addTab( m_arpWidget, tr( "ARP/CHORD" ), 2 );
	m_tabWidget->addTab( m_midiWidget, tr( "MIDI" ), 3 );

	// setup piano-widget
	m_pianoWidget = new pianoWidget( this );
	m_pianoWidget->setFixedSize( CHANNEL_WIDTH, PIANO_HEIGHT );


	vlayout->addWidget( m_generalSettingsWidget );
	vlayout->addWidget( m_tabWidget );
	vlayout->addWidget( m_pianoWidget );

#ifdef QT4
	m_midiInputAction = m_tswMidiMenu->addMenu(
						m_midiWidget->m_readablePorts );
	m_midiOutputAction = m_tswMidiMenu->addMenu(
					m_midiWidget->m_writeablePorts );
	m_midiInputAction->setText( tr( "MIDI input" ) );
	m_midiOutputAction->setText( tr( "MIDI output" ) );
	if( m_midiWidget->m_readablePorts == NULL )
	{
		connect( m_midiInputAction, SIGNAL( changed() ), this,
						SLOT( midiInSelected() ) );
	}
	if( m_midiWidget->m_writeablePorts == NULL )
	{
		connect( m_midiOutputAction, SIGNAL( changed() ), this,
						SLOT( midiOutSelected() ) );
	}
#else
	m_midiInputID = m_tswMidiMenu->insertItem( tr( "MIDI input" ),
					m_midiWidget->m_readablePorts );
	if( m_midiWidget->m_readablePorts == NULL )
	{
		m_tswMidiMenu->connectItem( m_midiInputID, this,
						SLOT( midiInSelected() ) );
	}
	m_midiOutputID = m_tswMidiMenu->insertItem( tr( "MIDI output" ),
					m_midiWidget->m_writeablePorts );
	if( m_midiWidget->m_writeablePorts == NULL )
	{
		m_tswMidiMenu->connectItem( m_midiOutputID, this,
						SLOT( midiOutSelected() ) );
	}
#endif
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


	_tc->updateAfterTrackAdd();
#ifndef QT3
	setFixedWidth( CHANNEL_WIDTH );
	resize( sizeHint() );
#endif
}






instrumentTrack::~instrumentTrack()
{
	invalidateAllMyNPH();
	delete m_audioPort;
	eng()->getMixer()->getMIDIClient()->removePort( m_midiPort );
}







void instrumentTrack::saveSettingsBtnClicked( void )
{
#ifdef QT4
	QFileDialog sfd( this, tr( "Save channel-settings in file" ), "",
				tr( "Channel-Settings-File (*.cs.xml)" ) );
#else
	QFileDialog sfd( this, "", TRUE );
	sfd.setWindowTitle( tr( "Save channel-settings in file" ) );
	sfd.setFilter( tr( "Channel-Settings-File (*.cs.xml)" ) );
#endif

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
#ifdef QT4
		!sfd.selectedFiles().isEmpty() && sfd.selectedFiles()[0] != ""
#else
		sfd.selectedFile() != ""
#endif
	)
	{
		multimediaProject mmp(
				multimediaProject::INSTRUMENT_TRACK_SETTINGS );
		QDomElement _this = mmp.createElement( nodeName() );
		saveTrackSpecificSettings( mmp, _this );
		mmp.content().appendChild( _this );
#ifdef QT4
		mmp.writeFile( sfd.selectedFiles()[0] );
#else
		mmp.writeFile( sfd.selectedFile() );
#endif
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
#ifdef QT4
	m_midiInputAction->setChecked( !m_midiInputAction->isChecked() );
	m_midiWidget->m_receiveCheckBox->setChecked(
					m_midiInputAction->isChecked() );
#else
	m_tswMidiMenu->setItemChecked( m_midiInputID,
			!m_tswMidiMenu->isItemChecked( m_midiInputID ) );
	m_midiWidget->m_receiveCheckBox->setChecked(
			m_tswMidiMenu->isItemChecked( m_midiInputID ) );
#endif
}



void instrumentTrack::midiOutSelected( void )
{
#ifdef QT4
	m_midiOutputAction->setChecked( !m_midiOutputAction->isChecked() );
	m_midiWidget->m_sendCheckBox->setChecked(
					m_midiOutputAction->isChecked() );
#else
	m_tswMidiMenu->setItemChecked( m_midiOutputID,
			!m_tswMidiMenu->isItemChecked( m_midiOutputID ) );
	m_midiWidget->m_sendCheckBox->setChecked(
			m_tswMidiMenu->isItemChecked( m_midiOutputID ) );
#endif
}




void instrumentTrack::midiConfigChanged( bool )
{
#ifdef QT4
	m_midiInputAction->setChecked(
				m_midiWidget->m_receiveCheckBox->isChecked() );
	m_midiOutputAction->setChecked(
				m_midiWidget->m_sendCheckBox->isChecked() );
#else
	m_tswMidiMenu->setItemChecked( m_midiInputID,
				m_midiWidget->m_receiveCheckBox->isChecked( ) );
	m_tswMidiMenu->setItemChecked( m_midiOutputID,
				m_midiWidget->m_sendCheckBox->isChecked( ) );
#endif
}




float instrumentTrack::frequency( notePlayHandle * _n ) const
{
	return( BASE_FREQ * powf( 2.0f, (float)( _n->tone() - m_baseTone +
			eng()->getSongEditor()->masterPitch() ) / 12.0f +
				(float)( _n->octave() - m_baseOctave ) ) );
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
							const fpab_t _frames,
							notePlayHandle * _n )
{
	// we must not play the sound if this instrumentTrack is muted...
	if( muted() )
	{
		return;
	}
	float v_scale = (float) getVolume() / DEFAULT_VOLUME;

	// instruments using instrument-play-handles will call this method
	// without any knowledge about notes, so they pass NULL for _n, which
	// is no problem for us since we just bypass the envelopes+LFOs
	if( _n != NULL )
	{
		m_envWidget->processAudioBuffer( _buf, _frames, _n );
		v_scale *= ( (float) _n->getVolume() / DEFAULT_VOLUME );
		const fpab_t ENV_FRAMES = 32;
		if( _n->totalFramesPlayed() == 0 )
		{
			// very basic envelope for not having clicks at the
			// beginning
			for( fpab_t i = 0; i < tMin<fpab_t>( _frames,
							ENV_FRAMES ); ++i )
			{
				for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS;
									++ch )
				{
					_buf[i][ch] *= (float) i / ENV_FRAMES;
				}
			}
		}
		// last time we're called for current note?
		if( ( _n->actualReleaseFramesToDo() == 0 &&
			_n->totalFramesPlayed() +
				eng()->getMixer()->framesPerAudioBuffer() >=
							_n->frames() ) ||
				( _n->released() && _n->releaseFramesDone() +
			eng()->getMixer()->framesPerAudioBuffer() >=
					_n->actualReleaseFramesToDo() ) )
		{
			// then do a soft fade-out at the end to avoid clicks
			for( fpab_t i = ( _frames >= ENV_FRAMES ) ?
						_frames - ENV_FRAMES : 0;
							i < _frames; ++i )
			{
				for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS;
									++ch )
				{
					_buf[i][ch] *= (float) ( _frames - i ) /
								ENV_FRAMES;
				}
			}
		}
	}

	volumeVector v = m_surroundArea->getVolumeVector( v_scale );

	eng()->getMixer()->bufferToPort( _buf, _frames,
				( _n != NULL ) ? _n->framesAhead() : 0, v,
								m_audioPort );
}




void instrumentTrack::processInEvent( const midiEvent & _me,
							const midiTime & _time )
{
	m_notesMutex.lock();
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
			_time.frames( eng()->getSongEditor()->framesPerTact() ),
						valueRanges<f_cnt_t>::max, n );
					if( eng()->getMixer()->addPlayHandle(
									nph ) )
					{
						m_notes[_me.key()] = nph;
					}
				}
				break;
			}

		case NOTE_OFF:
			if( m_notes[_me.key()] != NULL )
			{
				notePlayHandle * n = m_notes[_me.key()];
				// create dummy-note which has the same length
				// as the played note for sending it later
				// to all slots connected to signal noteDone()
				// this is for example needed by piano-roll for
				// recording notes into a pattern
				note done_note( NULL,
					midiTime( static_cast<f_cnt_t>(
					n->totalFramesPlayed() * 64 /
				eng()->getSongEditor()->framesPerTact() ) ),
					0, n->tone(), n->octave(),
					n->getVolume(), n->getPanning() );
				n->noteOff();
				m_notes[_me.key()] = NULL;
				emit noteDone( done_note );
			}
			break;

		case KEY_PRESSURE:
			if( m_notes[_me.key()] != NULL )
			{
				m_notes[_me.key()]->setVolume( _me.velocity() *
								100 / 128 );
			}
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
			printf( "channel-track: unhandled MIDI-event %d\n",
								_me.m_type );
			break;
	}
	m_notesMutex.unlock();
}




void instrumentTrack::processOutEvent( const midiEvent & _me,
							const midiTime & _time )
{
	if( _me.m_type == NOTE_ON && !configManager::inst()->value( "ui",
				"disablechannelactivityindicators" ).toInt() )
	{
		//QMutexLocker ml( &m_notesMutex );
		if( m_notes[_me.key()] != NULL )
		{
			return;
		}
		m_tswActivityIndicator->activate();
	}
	// if appropriate, midi-port does futher routing
	m_midiPort->processOutEvent( _me, _time );
}




void instrumentTrack::playNote( notePlayHandle * _n )
{
	// arpeggio- and chord-widget has to do its work -> adding sub-notes
	// for chords/arpeggios
	m_arpWidget->processNote( _n );

	if( _n->arpBaseNote() == FALSE && m_instrument != NULL )
	{
		// all is done, so now lets play the note!
		m_instrument->playNote( _n );
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
	if( m_instrument != NULL )
	{
		m_instrument->deleteNotePluginData( _n );
	}
}




void instrumentTrack::setName( const QString & _new_name )
{
	// when changing name of channel, also change name of those patterns,
	// which have the same name as the channel
	for( csize i = 0; i < numOfTCOs(); ++i )
	{
		pattern * p = dynamic_cast<pattern *>( getTCO( i ) );
		if( p != NULL && p->name() == m_name || p->name() == "" )
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
	m_audioPort->setName( m_name );
}




void instrumentTrack::setVolume( volume _new_volume )
{
	if( _new_volume <= MAX_VOLUME )
	{
		m_volumeKnob->setValue( _new_volume );
		//m_tswVolumeKnob->setValue( _new_volume );
	}
}




volume instrumentTrack::getVolume( void ) const
{
	return( static_cast<volume>( m_volumeKnob->value() ) );
}




void instrumentTrack::setSurroundAreaPos( const QPoint & _p )
{
	if( m_surroundArea->value() != _p )
	{
		m_surroundArea->setValue( _p );
	}
/*	if( m_tswSurroundArea->value() != _p )
	{
		m_tswSurroundArea->setValue( _p );
	}*/
}




void instrumentTrack::setBaseNote( Uint32 _new_note )
{
	setBaseTone( (tones)( _new_note % NOTES_PER_OCTAVE ) );
	setBaseOctave( (octaves)( _new_note / NOTES_PER_OCTAVE ) );
	eng()->getSongEditor()->setModified();
	emit baseNoteChanged();
}




void instrumentTrack::setBaseTone( tones _new_tone )
{
	if( _new_tone >= C && _new_tone <= H )
	{
		m_baseTone = _new_tone;
	}
}




void instrumentTrack::setBaseOctave( octaves _new_octave )
{
	if( _new_octave >= MIN_OCTAVE && _new_octave <= MAX_OCTAVE )
	{
		m_baseOctave = _new_octave;
	}
}




int instrumentTrack::masterKey( notePlayHandle * _n ) const
{
	int key = baseTone() + baseOctave() * NOTES_PER_OCTAVE +
					eng()->getSongEditor()->masterPitch();
	return( _n->key() - ( key - A - DEFAULT_OCTAVE * NOTES_PER_OCTAVE ) );
}




bool FASTCALL instrumentTrack::play( const midiTime & _start,
					const f_cnt_t _start_frame,
					const fpab_t _frames,
					const f_cnt_t _frame_base,
							Sint16 _tco_num )
{
	sendMidiTime( _start );

	// calculate samples per tact; need that later when calculating
	// sample-pos of a note
	float frames_per_tact = eng()->getSongEditor()->framesPerTact();

	vlist<trackContentObject *> tcos;
	if( _tco_num >= 0 )
	{
		tcos.push_back( getTCO( _tco_num ) );
	}
	else
	{
		getTCOsInRange( tcos, _start, _start +
				static_cast<Sint32>( _frames * 64 /
							frames_per_tact ) );
	}
	
	if ( tcos.size() == 0 )
	{
		return( FALSE );
	}

	bool played_a_note = FALSE;	// will be return variable

	// calculate the end of the current sample-frame
	const f_cnt_t end_frame = _start_frame+_frames-1;

	for( vlist<trackContentObject *>::iterator it = tcos.begin();
							it != tcos.end(); ++it )
	{
		pattern * p = dynamic_cast<pattern *>( *it );
		// everything which is not a pattern or muted won't be played
		if( p == NULL || ( *it )->muted() )
		{
			continue;
		}
		midiTime cur_start = _start.getTact() * 64 -
					( ( _tco_num < 0 ) ?
						p->startPosition() :
							midiTime( 0 ) );
		if( p->frozen() &&
				eng()->getSongEditor()->exporting() == FALSE )
		{
			volumeVector v = m_surroundArea->getVolumeVector(
									1.0f );
			// volume-vector was already used when freezing
			// pattern, but only in stereo-mode, so front speakers
			// are already setup
			v.vol[0] = 1.0f;
			v.vol[1] = 1.0f;
			sampleFrame * buf = bufferAllocator::alloc<sampleFrame>(
								_frames );

			p->playFrozenData( buf, _start_frame +
						static_cast<f_cnt_t>(
							cur_start.getTact() *
							frames_per_tact ),
						_frames );
			eng()->getMixer()->bufferToPort( buf, _frames,
								_frame_base +
				static_cast<f_cnt_t>(
					p->startPosition().getTact64th() *
							frames_per_tact /
								64.0f ),
							v, m_audioPort );
			bufferAllocator::free( buf );
			continue;
		}

		// get all notes from the given pattern...
		noteVector & notes = p->notes();
		// ...and set our index to zero
		noteVector::iterator it = notes.begin();

		// very effective algorithm for playing notes that are
		// posated within the current sample-frame


		if( cur_start > 0 )
		{
			// skip notes which are posated before start-tact
			while( it != notes.end() && ( *it )->pos() < cur_start )
			{
				++it;
			}
		}

		// skip notes before sample-frame
		while( it != notes.end() &&
			( *it )->pos( cur_start ).frames( frames_per_tact ) <
								_start_frame )
		{
			++it;
		}

		note * cur_note;
		while( it != notes.end() &&
			( ( cur_note = *it )->pos( cur_start ).frames(
					frames_per_tact ) ) <= end_frame )
		{
			if( cur_note->length() != 0 )
			{
				const f_cnt_t frames_ahead = _frame_base -
								_start_frame +
			cur_note->pos( cur_start ).frames( frames_per_tact );

				const f_cnt_t note_frames =
					cur_note->length().frames(
							frames_per_tact );

/*				// generate according MIDI-events
				processOutEvent( midiEvent( NOTE_ON,
						m_midiPort->outputChannel(),
							cur_note->key(),
							cur_note->getVolume() *
								128 / 100 ),
						midiTime::fromFrames(
							frames_ahead,
							frames_per_tact ) );

				processOutEvent( midiEvent( NOTE_OFF,
						m_midiPort->outputChannel(),
							cur_note->key(), 0 ),
						midiTime::fromFrames(
							frames_ahead +
								note_frames,
							frames_per_tact ) );*/

				notePlayHandle * note_play_handle =
					new notePlayHandle( this,
								frames_ahead,
								note_frames,
								*cur_note );
				note_play_handle->play();
				// could we play all within current number of
				// frames per audio-buffer?
				if( note_play_handle->done() == FALSE )
				{
					// no, then insert it into
					// play-handle-vector of mixer
					eng()->getMixer()->addPlayHandle(
							note_play_handle );
				}
				else
				{
					// otherwise just throw it away...
					delete note_play_handle;
				}
				played_a_note = TRUE;
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
	m_volumeKnob->saveSettings( _doc, _this, "vol" );

	m_surroundArea->saveSettings( _doc, _this, "surpos" );

	_this.setAttribute( "fxch", m_effectChannelNumber->value() );
	_this.setAttribute( "basetone", m_baseTone );
	_this.setAttribute( "baseoct", m_baseOctave );
	_this.setAttribute( "tab", m_tabWidget->activeTab() );

	mainWindow::saveWidgetState( this, _this );

	if( m_instrument != NULL )
	{
		m_instrument->saveState( _doc, _this );
	}
	m_envWidget->saveState( _doc, _this );
	m_arpWidget->saveState( _doc, _this );
	m_midiWidget->saveState( _doc, _this );
}




void instrumentTrack::loadTrackSpecificSettings( const QDomElement & _this )
{
	invalidateAllMyNPH();

	setName( _this.attribute( "name" ) );
	m_volumeKnob->loadSettings( _this, "vol" );

	m_surroundArea->loadSettings( _this, "surpos" );

	m_effectChannelNumber->setInitValue(
					_this.attribute( "fxch" ).toInt() );
	m_baseTone = static_cast<tones>( _this.attribute(
							"basetone" ).toInt() );
	m_baseOctave = static_cast<octaves>( _this.attribute(
							"baseoct" ).toInt() );

	int tab = _this.attribute( "tab" ).toInt();

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
			else if( automationPattern::classNodeName()
							!= node.nodeName() )
			{
				// if node-name doesn't match any known one,
				// we assume that it is an instrument-plugin
				// which we'll try to load
				delete m_instrument;
				m_instrument = instrument::instantiate(
							node.nodeName(), this );
				if( m_instrument->nodeName() ==
							node.nodeName() )
				{
					m_instrument->restoreState(
							node.toElement() );
				}
				m_tabWidget->addTab( m_instrument,
							tr( "PLUGIN" ), 0 );
			}
		}
		node = node.nextSibling();
        }

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

	delete m_instrument;
	m_instrument = instrument::instantiate( _plugin_name, this );

	m_tabWidget->addTab( m_instrument, tr( "PLUGIN" ), 0 );
	m_tabWidget->setActiveTab( 0 );

	m_tswInstrumentTrackButton->update();

	return( m_instrument );
}




/*void instrumentTrack::volValueChanged( float _new_value )
{
	setVolume( (volume) _new_value );
}*/




void instrumentTrack::surroundAreaPosChanged( const QPoint & _p )
{
	setSurroundAreaPos( _p );
	eng()->getSongEditor()->setModified();
}




void instrumentTrack::textChanged( const QString & _new_name )
{
	setName( _new_name );
	eng()->getSongEditor()->setModified();
}




void instrumentTrack::toggledInstrumentTrackButton( bool _on )
{
	if( m_tswInstrumentTrackButton->isChecked() != _on )
	{
		m_tswInstrumentTrackButton->setChecked( _on );
	}
	if( _on )
	{
		show();
		raise();
	}
	else
	{
		hide();
	}
}




void instrumentTrack::closeEvent( QCloseEvent * _ce )
{
	_ce->ignore();
	hide();
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
		eng()->getSongEditor()->setModified();
		_de->accept();
	}
	else if( type == "presetfile" )
	{
		multimediaProject mmp( value );
		loadTrackSpecificSettings( mmp.content().firstChild().
								toElement() );
		eng()->getSongEditor()->setModified();
		_de->accept();
	}
}




void instrumentTrack::invalidateAllMyNPH( void )
{
	// note-play-handles check track-type to determine whether their
	// channel-track is being deleted (if this is the case, they
	// invalidate themselves)
	m_trackType = NULL_TRACK;

	m_notesMutex.lock();
	for( int i = 0; i < NOTES_PER_OCTAVE * OCTAVES; ++i )
	{
		m_notes[i] = NULL;
	}
	m_notesMutex.unlock();

	// invalidate all note-play-handles linked to this channel
	eng()->getMixer()->checkValidityOfPlayHandles();

	m_trackType = CHANNEL_TRACK;
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
#ifdef QT4
	_p->setPen( QApplication::palette().buttonText().color() );
#else
	_p->setPen( QApplication::palette().color( QPalette::Normal,
						QColorGroup::ButtonText ) );
#endif
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
