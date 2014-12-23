/*
 * InstrumentTrack.cpp - implementation of instrument-track-class
 *                        (window + data-structures)
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QQueue>
#include <QtGui/QApplication>
#include <QtGui/QCloseEvent>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QMdiArea>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QMdiSubWindow>
#include <QtGui/QPainter>

#include "FileDialog.h"
#include "InstrumentTrack.h"
#include "AudioPort.h"
#include "AutomationPattern.h"
#include "bb_track.h"
#include "config_mgr.h"
#include "ControllerConnection.h"
#include "debug.h"
#include "EffectChain.h"
#include "EffectRackView.h"
#include "embed.h"
#include "engine.h"
#include "FileBrowser.h"
#include "FxMixer.h"
#include "FxMixerView.h"
#include "InstrumentSoundShaping.h"
#include "InstrumentSoundShapingView.h"
#include "fade_button.h"
#include "gui_templates.h"
#include "Instrument.h"
#include "InstrumentFunctionViews.h"
#include "InstrumentMidiIOView.h"
#include "knob.h"
#include "LcdSpinBox.h"
#include "led_checkbox.h"
#include "MainWindow.h"
#include "MidiClient.h"
#include "MidiPortMenu.h"
#include "MixHelpers.h"
#include "DataFile.h"
#include "NotePlayHandle.h"
#include "Pattern.h"
#include "PluginView.h"
#include "SamplePlayHandle.h"
#include "song.h"
#include "string_pair_drag.h"
#include "tab_widget.h"
#include "tooltip.h"
#include "track_label_button.h"



const char * volume_help = QT_TRANSLATE_NOOP( "InstrumentTrack",
						"With this knob you can set "
						"the volume of the opened "
						"channel.");

const int INSTRUMENT_WIDTH	= 254;
const int INSTRUMENT_HEIGHT	= INSTRUMENT_WIDTH;
const int PIANO_HEIGHT		= 82;
const int INSTRUMENT_WINDOW_CACHE_SIZE = 8;


// #### IT:
InstrumentTrack::InstrumentTrack( TrackContainer* tc ) :
	track( track::InstrumentTrack, tc ),
	MidiEventProcessor(),
	m_audioPort( tr( "unnamed_track" ) ),
	m_midiPort( tr( "unnamed_track" ), engine::mixer()->midiClient(),
								this, this ),
	m_notes(),
	m_sustainPedalPressed( false ),
	m_silentBuffersProcessed( false ),
	m_baseNoteModel( 0, 0, KeysPerOctave * NumOctaves - 1, this,
							tr( "Base note" ) ),
	m_volumeModel( DefaultVolume, MinVolume, MaxVolume, 0.1f, this, tr( "Volume" ) ),
	m_panningModel( DefaultPanning, PanningLeft, PanningRight, 0.1f, this, tr( "Panning" ) ),
	m_pitchModel( 0, MinPitchDefault, MaxPitchDefault, 1, this, tr( "Pitch" ) ),
	m_pitchRangeModel( 1, 1, 24, this, tr( "Pitch range" ) ),
	m_effectChannelModel( 0, 0, 0, this, tr( "FX channel" ) ),
	m_instrument( NULL ),
	m_soundShaping( this ),
	m_arpeggio( this ),
	m_noteStacking( this ),
	m_piano( this )
{
	m_pitchModel.setCenterValue( 0 );
	m_panningModel.setCenterValue( DefaultPanning );
	m_baseNoteModel.setInitValue( DefaultKey );

	connect( &m_baseNoteModel, SIGNAL( dataChanged() ), this, SLOT( updateBaseNote() ) );
	connect( &m_pitchModel, SIGNAL( dataChanged() ), this, SLOT( updatePitch() ) );
	connect( &m_pitchRangeModel, SIGNAL( dataChanged() ), this, SLOT( updatePitchRange() ) );

    m_effectChannelModel.setRange( 0, engine::fxMixer()->numChannels()-1, 1);

	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = NULL;
		m_runningMidiNotes[i] = 0;
	}


	setName( tr( "Default preset" ) );

}


int InstrumentTrack::baseNote() const
{
	return m_baseNoteModel.value() - engine::getSong()->masterPitch();
}



InstrumentTrack::~InstrumentTrack()
{
	// kill all running notes and the iph
	silenceAllNotes( true );

	// now we're save deleting the instrument
	delete m_instrument;
}




void InstrumentTrack::processAudioBuffer( sampleFrame* buf, const fpp_t frames, NotePlayHandle* n )
{
	// we must not play the sound if this InstrumentTrack is muted...
	if( isMuted() || ( n && n->isBbTrackMuted() ) )
	{
		return;
	}

	// Test for silent input data if instrument provides a single stream only (i.e. driven by InstrumentPlayHandle)
	// We could do that in all other cases as well but the overhead for silence test is bigger than
	// what we potentially save. While playing a note, a NotePlayHandle-driven instrument will produce sound in
	// 99 of 100 cases so that test would be a waste of time.
	if( m_instrument->flags().testFlag( Instrument::IsSingleStreamed ) &&
		MixHelpers::isSilent( buf, frames ) )
	{
		// at least pass one silent buffer to allow
		if( m_silentBuffersProcessed )
		{
			// skip further processing
			return;
		}
		m_silentBuffersProcessed = true;
	}
	else
	{
		m_silentBuffersProcessed = false;
	}

	// if effects "went to sleep" because there was no input, wake them up
	// now
	m_audioPort.effects()->startRunning();

	float v_scale = (float) getVolume() / DefaultVolume;

	// instruments using instrument-play-handles will call this method
	// without any knowledge about notes, so they pass NULL for n, which
	// is no problem for us since we just bypass the envelopes+LFOs
	if( m_instrument->flags().testFlag( Instrument::IsSingleStreamed ) == false && n != NULL )
	{
		m_soundShaping.processAudioBuffer( buf, frames, n );
		v_scale *= ( (float) n->getVolume() / DefaultVolume );
	}

	m_audioPort.setNextFxChannel( m_effectChannelModel.value() );
	
	int framesToMix = frames;
	int offset = 0;
	int panning = m_panningModel.value();

	if( n )
	{
		framesToMix = qMin<f_cnt_t>( n->framesLeftForCurrentPeriod(), framesToMix );
		offset = n->offset();

		panning += n->getPanning();
		panning = tLimit<int>( panning, PanningLeft, PanningRight );
	}

	engine::mixer()->bufferToPort( buf, framesToMix, offset, panningToVolumeVector( panning, v_scale ), &m_audioPort );
}




MidiEvent InstrumentTrack::applyMasterKey( const MidiEvent& event )
{
	MidiEvent copy( event );
	switch( event.type() )
	{
		case MidiNoteOn:
		case MidiNoteOff:
		case MidiKeyPressure:
			copy.setKey( masterKey( event.key() ) );
			break;
		default:
			break;
	}
	return copy;
}




void InstrumentTrack::processInEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	engine::mixer()->lock();
	bool eventHandled = false;

	switch( event.type() )
	{
		// we don't send MidiNoteOn, MidiNoteOff and MidiKeyPressure
		// events to instrument as NotePlayHandle will send them on its
		// own
		case MidiNoteOn:
			if( event.velocity() > 0 )
			{
				if( m_notes[event.key()] == NULL )
				{
					// create (timed) note-play-handle
					NotePlayHandle* nph = new NotePlayHandle( this, time.frames( engine::framesPerTick() ),
																typeInfo<f_cnt_t>::max() / 2,
																note( MidiTime(), MidiTime(), event.key(), event.volume( midiPort()->baseVelocity() ) ),
																NULL, event.channel(),
																NotePlayHandle::OriginMidiInput );
					if( engine::mixer()->addPlayHandle( nph ) )
					{
						m_notes[event.key()] = nph;
					}
				}

				eventHandled = true;
				break;
			}

		case MidiNoteOff:
			if( m_notes[event.key()] != NULL )
			{
				// do actual note off and remove internal reference to NotePlayHandle (which itself will
				// be deleted later automatically)
				m_notes[event.key()]->noteOff();
				m_notes[event.key()] = NULL;
			}
			eventHandled = true;
			break;

		case MidiKeyPressure:
			if( m_notes[event.key()] != NULL )
			{
				// setVolume() calls processOutEvent() with MidiKeyPressure so the
				// attached instrument will receive the event as well
				m_notes[event.key()]->setVolume( event.volume( midiPort()->baseVelocity() ) );
			}
			eventHandled = true;
			break;

		case MidiPitchBend:
			// updatePitch() is connected to m_pitchModel::dataChanged() which will send out
			// MidiPitchBend events
			m_pitchModel.setValue( m_pitchModel.minValue() + event.pitchBend() * m_pitchModel.range() / MidiMaxPitchBend );
			break;

		case MidiControlChange:
			if( event.controllerNumber() == MidiControllerSustain )
			{
				if( event.controllerValue() > MidiMaxControllerValue/2 )
				{
					m_sustainPedalPressed = true;
				}
				else
				{
					m_sustainPedalPressed = false;
				}
			}
			if( event.controllerNumber() == MidiControllerAllSoundOff ||
			    event.controllerNumber() == MidiControllerAllNotesOff ||
			    event.controllerNumber() == MidiControllerOmniOn ||
			    event.controllerNumber() == MidiControllerOmniOff ||
			    event.controllerNumber() == MidiControllerMonoOn ||
			    event.controllerNumber() == MidiControllerPolyOn )
			{
				silenceAllNotes();
			}
			break;

		case MidiMetaEvent:
			// handle special cases such as note panning
			switch( event.metaEvent() )
			{
				case MidiNotePanning:
					if( m_notes[event.key()] != NULL )
					{
						eventHandled = true;
						m_notes[event.key()]->setPanning( event.panning() );
					}
					break;
				default:
					qWarning( "InstrumentTrack: unhandled MIDI meta event: %i", event.metaEvent() );
					break;
			}
			break;
			
		default:
			break;
	}

	if( eventHandled == false && instrument()->handleMidiEvent( event, time, offset ) == false )
	{
		qWarning( "InstrumentTrack: unhandled MIDI event %d", event.type() );
	}

	engine::mixer()->unlock();
}




void InstrumentTrack::processOutEvent( const MidiEvent& event, const MidiTime& time, f_cnt_t offset )
{
	// do nothing if we do not have an instrument instance (e.g. when loading settings)
	if( m_instrument == NULL )
	{
		return;
	}

	const MidiEvent transposedEvent = applyMasterKey( event );
	const int key = transposedEvent.key();

	switch( event.type() )
	{
		case MidiNoteOn:
			m_piano.setKeyState( event.key(), true );	// event.key() = original key

			if( key >= 0 && key < NumKeys )
			{
				if( m_runningMidiNotes[key] > 0 )
				{
					m_instrument->handleMidiEvent( MidiEvent( MidiNoteOff, midiPort()->realOutputChannel(), key, 0 ), time, offset );
				}
				++m_runningMidiNotes[key];
				m_instrument->handleMidiEvent( MidiEvent( MidiNoteOn, midiPort()->realOutputChannel(), key, event.velocity() ), time, offset );

				emit newNote();
			}
			break;

		case MidiNoteOff:
			m_piano.setKeyState( event.key(), false );	// event.key() = original key

			if( key >= 0 && key < NumKeys && --m_runningMidiNotes[key] <= 0 )
			{
				m_runningMidiNotes[key] = qMax( 0, m_runningMidiNotes[key] );
				m_instrument->handleMidiEvent( MidiEvent( MidiNoteOff, midiPort()->realOutputChannel(), key, 0 ), time, offset );
			}
			break;

		default:
			m_instrument->handleMidiEvent( transposedEvent, time, offset );
			break;
	}

	// if appropriate, midi-port does futher routing
	m_midiPort.processOutEvent( event, time );
}




void InstrumentTrack::silenceAllNotes( bool removeIPH )
{
	engine::mixer()->lock();
	for( int i = 0; i < NumKeys; ++i )
	{
		m_notes[i] = NULL;
		m_runningMidiNotes[i] = 0;
	}

	// invalidate all NotePlayHandles linked to this track
	m_processHandles.clear();
	engine::mixer()->removePlayHandles( this, removeIPH );
	engine::mixer()->unlock();
}




f_cnt_t InstrumentTrack::beatLen( NotePlayHandle * _n ) const
{
	if( m_instrument != NULL )
	{
		const f_cnt_t len = m_instrument->beatLen( _n );
		if( len > 0 )
		{
			return len;
		}
	}
	return m_soundShaping.envFrames();
}




void InstrumentTrack::playNote( NotePlayHandle* n, sampleFrame* workingBuffer )
{
	// arpeggio- and chord-widget has to do its work -> adding sub-notes
	// for chords/arpeggios
	m_noteStacking.processNote( n );
	m_arpeggio.processNote( n );

	if( n->isMasterNote() == false && m_instrument != NULL )
	{
		// all is done, so now lets play the note!
		m_instrument->playNote( n, workingBuffer );
	}
}




QString InstrumentTrack::instrumentName() const
{
	if( m_instrument != NULL )
	{
		return m_instrument->displayName();
	}
	return QString::null;
}




void InstrumentTrack::deleteNotePluginData( NotePlayHandle* n )
{
	if( m_instrument != NULL )
	{
		m_instrument->deleteNotePluginData( n );
	}
}




void InstrumentTrack::setName( const QString & _new_name )
{
	// when changing name of track, also change name of those patterns,
	// which have the same name as the instrument-track
	for( int i = 0; i < numOfTCOs(); ++i )
	{
		Pattern* p = dynamic_cast<Pattern*>( getTCO( i ) );
		if( ( p != NULL && p->name() == name() ) || p->name() == "" )
		{
			p->setName( _new_name );
		}
	}

	track::setName( _new_name );
	m_midiPort.setName( name() );
	m_audioPort.setName( name() );

	emit nameChanged();
}






void InstrumentTrack::updateBaseNote()
{
	engine::mixer()->lock();
	for( NotePlayHandleList::Iterator it = m_processHandles.begin();
					it != m_processHandles.end(); ++it )
	{
		( *it )->updateFrequency();
	}
	engine::mixer()->unlock();
}




void InstrumentTrack::updatePitch()
{
	updateBaseNote();

	processOutEvent( MidiEvent( MidiPitchBend, midiPort()->realOutputChannel(), midiPitch() ) );
}




void InstrumentTrack::updatePitchRange()
{
	const int r = m_pitchRangeModel.value();
	m_pitchModel.setRange( MinPitchDefault * r, MaxPitchDefault * r );

	processOutEvent( MidiEvent( MidiControlChange, midiPort()->realOutputChannel(),
								MidiControllerRegisteredParameterNumberLSB, MidiPitchBendSensitivityRPN & 0x7F ) );
	processOutEvent( MidiEvent( MidiControlChange, midiPort()->realOutputChannel(),
								MidiControllerRegisteredParameterNumberMSB, ( MidiPitchBendSensitivityRPN >> 8 ) & 0x7F ) );
	processOutEvent( MidiEvent( MidiControlChange, midiPort()->realOutputChannel(), MidiControllerDataEntry, midiPitchRange() ) );
}




int InstrumentTrack::masterKey( int _midi_key ) const
{
	int key = baseNote();
	return tLimit<int>( _midi_key - ( key - DefaultKey ), 0, NumKeys );
}




void InstrumentTrack::removeMidiPortNode( DataFile & _dataFile )
{
	QDomNodeList n = _dataFile.elementsByTagName( "midiport" );
	n.item( 0 ).parentNode().removeChild( n.item( 0 ) );
}




bool InstrumentTrack::play( const MidiTime & _start, const fpp_t _frames,
							const f_cnt_t _offset, int _tco_num )
{
	const float frames_per_tick = engine::framesPerTick();

	tcoVector tcos;
	bbTrack * bb_track = NULL;
	if( _tco_num >= 0 )
	{
		trackContentObject * tco = getTCO( _tco_num );
		tcos.push_back( tco );
		bb_track = bbTrack::findBBTrack( _tco_num );
	}
	else
	{
		getTCOsInRange( tcos, _start, _start + static_cast<int>(
					_frames / frames_per_tick ) );
	}

	// Handle automation: detuning
	for( NotePlayHandleList::Iterator it = m_processHandles.begin();
					it != m_processHandles.end(); ++it )
	{
		( *it )->processMidiTime( _start );
	}

	if ( tcos.size() == 0 )
	{
		return false;
	}

	bool played_a_note = false;	// will be return variable

	for( tcoVector::Iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		Pattern* p = dynamic_cast<Pattern*>( *it );
		// everything which is not a pattern or muted won't be played
		if( p == NULL || ( *it )->isMuted() )
		{
			continue;
		}
		MidiTime cur_start = _start;
		if( _tco_num < 0 )
		{
			cur_start -= p->startPosition();
		}

		// get all notes from the given pattern...
		const NoteVector & notes = p->notes();
		// ...and set our index to zero
		NoteVector::ConstIterator nit = notes.begin();

		// very effective algorithm for playing notes that are
		// posated within the current sample-frame


		if( cur_start > 0 )
		{
			// skip notes which are posated before start-tact
			while( nit != notes.end() && ( *nit )->pos() < cur_start )
			{
				++nit;
			}
		}

		note * cur_note;
		while( nit != notes.end() &&
					( cur_note = *nit )->pos() == cur_start )
		{
			if( cur_note->length() != 0 )
			{
				const f_cnt_t note_frames =
					cur_note->length().frames(
							frames_per_tick );

				NotePlayHandle* notePlayHandle = new NotePlayHandle( this, _offset, note_frames, *cur_note );
				notePlayHandle->setBBTrack( bb_track );
				// are we playing global song?
				if( _tco_num < 0 )
				{
					// then set song-global offset of pattern in order to
					// properly perform the note detuning
					notePlayHandle->setSongGlobalParentOffset( p->startPosition() );
				}

				engine::mixer()->addPlayHandle( notePlayHandle );
				played_a_note = true;
			}
			++nit;
		}
	}
	return played_a_note;
}




trackContentObject * InstrumentTrack::createTCO( const MidiTime & )
{
	return new Pattern( this );
}




trackView * InstrumentTrack::createView( TrackContainerView* tcv )
{
	return new InstrumentTrackView( this, tcv );
}




void InstrumentTrack::saveTrackSpecificSettings( QDomDocument& doc, QDomElement & thisElement )
{
	m_volumeModel.saveSettings( doc, thisElement, "vol" );
	m_panningModel.saveSettings( doc, thisElement, "pan" );
	m_pitchModel.saveSettings( doc, thisElement, "pitch" );
	m_pitchRangeModel.saveSettings( doc, thisElement, "pitchrange" );

	m_effectChannelModel.saveSettings( doc, thisElement, "fxch" );
	m_baseNoteModel.saveSettings( doc, thisElement, "basenote" );

	if( m_instrument != NULL )
	{
		QDomElement i = doc.createElement( "instrument" );
		i.setAttribute( "name", m_instrument->descriptor()->name );
		m_instrument->saveState( doc, i );
		thisElement.appendChild( i );
	}
	m_soundShaping.saveState( doc, thisElement );
	m_noteStacking.saveState( doc, thisElement );
	m_arpeggio.saveState( doc, thisElement );
	m_midiPort.saveState( doc, thisElement );
	m_audioPort.effects()->saveState( doc, thisElement );
}




void InstrumentTrack::loadTrackSpecificSettings( const QDomElement & thisElement )
{
	silenceAllNotes( true );

	engine::mixer()->lock();

	m_volumeModel.loadSettings( thisElement, "vol" );
	m_panningModel.loadSettings( thisElement, "pan" );
	m_pitchRangeModel.loadSettings( thisElement, "pitchrange" );
	m_pitchModel.loadSettings( thisElement, "pitch" );
	m_effectChannelModel.setRange( 0, engine::fxMixer()->numChannels()-1 );
	m_effectChannelModel.loadSettings( thisElement, "fxch" );
	m_baseNoteModel.loadSettings( thisElement, "basenote" );

	// clear effect-chain just in case we load an old preset without FX-data
	m_audioPort.effects()->clear();

	QDomNode node = thisElement.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( m_soundShaping.nodeName() == node.nodeName() )
			{
				m_soundShaping.restoreState( node.toElement() );
			}
			else if( m_noteStacking.nodeName() == node.nodeName() )
			{
				m_noteStacking.restoreState( node.toElement() );
			}
			else if( m_arpeggio.nodeName() == node.nodeName() )
			{
				m_arpeggio.restoreState( node.toElement() );
			}
			else if( m_midiPort.nodeName() == node.nodeName() )
			{
				m_midiPort.restoreState( node.toElement() );
			}
			else if( m_audioPort.effects()->nodeName() == node.nodeName() )
			{
				m_audioPort.effects()->restoreState( node.toElement() );
			}
			else if( node.nodeName() == "instrument" )
			{
				delete m_instrument;
				m_instrument = NULL;
				m_instrument = Instrument::instantiate( node.toElement().attribute( "name" ), this );
				m_instrument->restoreState( node.firstChildElement() );

				emit instrumentChanged();
			}
			// compat code - if node-name doesn't match any known
			// one, we assume that it is an instrument-plugin
			// which we'll try to load
			else if( AutomationPattern::classNodeName() != node.nodeName() &&
					ControllerConnection::classNodeName() != node.nodeName() &&
					!node.toElement().hasAttribute( "id" ) )
			{
				delete m_instrument;
				m_instrument = NULL;
				m_instrument = Instrument::instantiate( node.nodeName(), this );
				if( m_instrument->nodeName() == node.nodeName() )
				{
					m_instrument->restoreState( node.toElement() );
				}
				emit instrumentChanged();
			}
		}
		node = node.nextSibling();
	}
	updatePitchRange();
	engine::mixer()->unlock();
}




Instrument * InstrumentTrack::loadInstrument( const QString & _plugin_name )
{
	silenceAllNotes( true );

	engine::mixer()->lock();
	delete m_instrument;
	m_instrument = Instrument::instantiate( _plugin_name, this );
	engine::mixer()->unlock();

	setName( m_instrument->displayName() );
	emit instrumentChanged();

	return m_instrument;
}





// #### ITV:


QQueue<InstrumentTrackWindow *> InstrumentTrackView::s_windowCache;



InstrumentTrackView::InstrumentTrackView( InstrumentTrack * _it, TrackContainerView* tcv ) :
	trackView( _it, tcv ),
	m_window( NULL ),
	m_lastPos( -1, -1 )
{
	setAcceptDrops( true );
	setFixedHeight( 32 );

	m_tlb = new trackLabelButton( this, getTrackSettingsWidget() );
	m_tlb->setCheckable( true );
	m_tlb->setIcon( embed::getIconPixmap( "instrument_track" ) );
	m_tlb->move( 3, 1 );
	m_tlb->show();

	connect( m_tlb, SIGNAL( toggled( bool ) ),
			this, SLOT( toggleInstrumentWindow( bool ) ) );

	connect( _it, SIGNAL( nameChanged() ),
			m_tlb, SLOT( update() ) );

	// creation of widgets for track-settings-widget
	int widgetWidth;
	if( configManager::inst()->value( "ui",
					  "compacttrackbuttons" ).toInt() )
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH_COMPACT;
	}
	else 
	{
		widgetWidth = DEFAULT_SETTINGS_WIDGET_WIDTH;
	}

	m_volumeKnob = new knob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setModel( &_it->m_volumeModel );
	m_volumeKnob->setHintText( tr( "Volume:" ) + " ", "%" );
	m_volumeKnob->move( widgetWidth-2*24, 2 );
	m_volumeKnob->setLabel( tr( "VOL" ) );
	m_volumeKnob->show();
	m_volumeKnob->setWhatsThis( tr( volume_help ) );

	m_panningKnob = new knob( knobSmall_17, getTrackSettingsWidget(),
							tr( "Panning" ) );
	m_panningKnob->setModel( &_it->m_panningModel );
	m_panningKnob->setHintText( tr( "Panning:" ) + " ", "%" );
	m_panningKnob->move( widgetWidth-24, 2 );
	m_panningKnob->setLabel( tr( "PAN" ) );
	m_panningKnob->show();

	m_midiMenu = new QMenu( tr( "MIDI" ), this );

	// sequenced MIDI?
	if( !engine::mixer()->midiClient()->isRaw() )
	{
		_it->m_midiPort.m_readablePortsMenu = new MidiPortMenu(
							MidiPort::Input );
		_it->m_midiPort.m_writablePortsMenu = new MidiPortMenu(
							MidiPort::Output );
		_it->m_midiPort.m_readablePortsMenu->setModel(
							&_it->m_midiPort );
		_it->m_midiPort.m_writablePortsMenu->setModel(
							&_it->m_midiPort );
		m_midiInputAction = m_midiMenu->addMenu(
					_it->m_midiPort.m_readablePortsMenu );
		m_midiOutputAction = m_midiMenu->addMenu(
					_it->m_midiPort.m_writablePortsMenu );
	}
	else
	{
		m_midiInputAction = m_midiMenu->addAction( "" );
		m_midiOutputAction = m_midiMenu->addAction( "" );
		m_midiInputAction->setCheckable( true );
		m_midiOutputAction->setCheckable( true );
		connect( m_midiInputAction, SIGNAL( changed() ), this,
						SLOT( midiInSelected() ) );
		connect( m_midiOutputAction, SIGNAL( changed() ), this,
					SLOT( midiOutSelected() ) );
		connect( &_it->m_midiPort, SIGNAL( modeChanged() ),
				this, SLOT( midiConfigChanged() ) );
	}

	m_midiInputAction->setText( tr( "Input" ) );
	m_midiOutputAction->setText( tr( "Output" ) );

	m_activityIndicator = new fadeButton( QApplication::palette().color( QPalette::Active,
							QPalette::Background),
						QApplication::palette().color( QPalette::Active,
							QPalette::BrightText ),
						getTrackSettingsWidget() );
	m_activityIndicator->setGeometry(
					 widgetWidth-2*24-11, 2, 8, 28 );
	m_activityIndicator->show();
	connect( m_activityIndicator, SIGNAL( pressed() ),
				this, SLOT( activityIndicatorPressed() ) );
	connect( m_activityIndicator, SIGNAL( released() ),
				this, SLOT( activityIndicatorReleased() ) );
	connect( _it, SIGNAL( newNote() ),
				m_activityIndicator, SLOT( activate() ) );


	setModel( _it );
}




InstrumentTrackView::~InstrumentTrackView()
{
	freeInstrumentTrackWindow();

	delete model()->m_midiPort.m_readablePortsMenu;
	delete model()->m_midiPort.m_writablePortsMenu;
}




InstrumentTrackWindow * InstrumentTrackView::topLevelInstrumentTrackWindow()
{
	InstrumentTrackWindow * w = NULL;
	foreach( QMdiSubWindow * sw,
				engine::mainWindow()->workspace()->subWindowList(
											QMdiArea::ActivationHistoryOrder ) )
	{
		if( sw->isVisible() && sw->widget()->inherits( "InstrumentTrackWindow" ) )
		{
			w = qobject_cast<InstrumentTrackWindow *>( sw->widget() );
		}
	}

	return w;
}



// TODO: Add windows to free list on freeInstrumentTrackWindow. 
// But, don't NULL m_window or disconnect signals.  This will allow windows 
// that are being show/hidden frequently to stay connected.
void InstrumentTrackView::freeInstrumentTrackWindow()
{
	if( m_window != NULL )
	{
		m_lastPos = m_window->parentWidget()->pos();

		if( configManager::inst()->value( "ui",
										"oneinstrumenttrackwindow" ).toInt() ||
						s_windowCache.count() < INSTRUMENT_WINDOW_CACHE_SIZE )
		{
			model()->setHook( NULL );
			m_window->setInstrumentTrackView( NULL );
			m_window->parentWidget()->hide();
			//m_window->setModel(
			//	engine::dummyTrackContainer()->
			//			dummyInstrumentTrack() );
			m_window->updateInstrumentView();
			s_windowCache << m_window;
		}
		else
		{
			delete m_window;
		}
		
		m_window = NULL;
	}
}




void InstrumentTrackView::cleanupWindowCache()
{
	while( !s_windowCache.isEmpty() )
	{
		delete s_windowCache.dequeue();
	}
}




InstrumentTrackWindow * InstrumentTrackView::getInstrumentTrackWindow()
{
	if( m_window != NULL )
	{
	}
	else if( !s_windowCache.isEmpty() )
	{
		m_window = s_windowCache.dequeue();
		
		m_window->setInstrumentTrackView( this );
		m_window->setModel( model() );
		m_window->updateInstrumentView();
		model()->setHook( m_window );

		if( configManager::inst()->
							value( "ui", "oneinstrumenttrackwindow" ).toInt() )
		{
			s_windowCache << m_window;
		}
		else if( m_lastPos.x() > 0 || m_lastPos.y() > 0 )
		{
			m_window->parentWidget()->move( m_lastPos );
		}
	}
	else
	{
		m_window = new InstrumentTrackWindow( this );
		if( configManager::inst()->
							value( "ui", "oneinstrumenttrackwindow" ).toInt() )
		{
			// first time, an InstrumentTrackWindow is opened
			s_windowCache << m_window;
		}
	}
		
	return m_window;
}




void InstrumentTrackView::dragEnterEvent( QDragEnterEvent * _dee )
{
	InstrumentTrackWindow::dragEnterEventGeneric( _dee );
	if( !_dee->isAccepted() )
	{
		trackView::dragEnterEvent( _dee );
	}
}




void InstrumentTrackView::dropEvent( QDropEvent * _de )
{
	getInstrumentTrackWindow()->dropEvent( _de );
	trackView::dropEvent( _de );
}




void InstrumentTrackView::toggleInstrumentWindow( bool _on )
{
	getInstrumentTrackWindow()->toggleVisibility( _on );
	
	if( !_on )
	{
		freeInstrumentTrackWindow();
	}
}




void InstrumentTrackView::activityIndicatorPressed()
{
	model()->processInEvent( MidiEvent( MidiNoteOn, 0, DefaultKey, MidiDefaultVelocity ) );
}




void InstrumentTrackView::activityIndicatorReleased()
{
	model()->processInEvent( MidiEvent( MidiNoteOff, 0, DefaultKey, 0 ) );
}





void InstrumentTrackView::midiInSelected()
{
	if( model() )
	{
		model()->m_midiPort.setReadable( m_midiInputAction->isChecked() );
	}
}




void InstrumentTrackView::midiOutSelected()
{
	if( model() )
	{
		model()->m_midiPort.setWritable( m_midiOutputAction->isChecked() );
	}
}




void InstrumentTrackView::midiConfigChanged()
{
	m_midiInputAction->setChecked( model()->m_midiPort.isReadable() );
	m_midiOutputAction->setChecked( model()->m_midiPort.isWritable() );
}






class fxLineLcdSpinBox : public LcdSpinBox 
{
	public:
		fxLineLcdSpinBox( int _num_digits, QWidget * _parent, 
				const QString & _name ) :
			LcdSpinBox( _num_digits, _parent, _name ) {}

	protected:
		virtual void mouseDoubleClickEvent ( QMouseEvent * _me )
		{
			engine::fxMixerView()->setCurrentFxLine( model()->value() );

			engine::fxMixerView()->show();// show fxMixer window
			engine::fxMixerView()->setFocus();// set focus to fxMixer window
			//engine::getFxMixerView()->raise();
		}
};



// #### ITW:
InstrumentTrackWindow::InstrumentTrackWindow( InstrumentTrackView * _itv ) :
	QWidget(),
	ModelView( NULL, this ),
	m_track( _itv->model() ),
	m_itv( _itv ),
	m_instrumentView( NULL )
{
	setAcceptDrops( true );

	// init own layout + widgets
	setFocusPolicy( Qt::StrongFocus );
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setMargin( 0 );
	vlayout->setSpacing( 0 );

	tabWidget* generalSettingsWidget = new tabWidget( tr( "GENERAL SETTINGS" ), this );

	QVBoxLayout* generalSettingsLayout = new QVBoxLayout( generalSettingsWidget );

	generalSettingsLayout->setContentsMargins( 8, 18, 8, 8 );
	generalSettingsLayout->setSpacing( 6 );

	// setup line edit for changing instrument track name
	m_nameLineEdit = new QLineEdit;
	m_nameLineEdit->setFont( pointSize<9>( m_nameLineEdit->font() ) );
	connect( m_nameLineEdit, SIGNAL( textChanged( const QString & ) ),
				this, SLOT( textChanged( const QString & ) ) );

	generalSettingsLayout->addWidget( m_nameLineEdit );

	QHBoxLayout* basicControlsLayout = new QHBoxLayout;
	basicControlsLayout->setSpacing( 3 );

	// set up volume knob
	m_volumeKnob = new knob( knobBright_26, NULL, tr( "Instrument volume" ) );
	m_volumeKnob->setVolumeKnob( true );
	m_volumeKnob->setHintText( tr( "Volume:" ) + " ", "%" );
	m_volumeKnob->setLabel( tr( "VOL" ) );

	m_volumeKnob->setWhatsThis( tr( volume_help ) );

	basicControlsLayout->addWidget( m_volumeKnob );

	// set up panning knob
	m_panningKnob = new knob( knobBright_26, NULL, tr( "Panning" ) );
	m_panningKnob->setHintText( tr( "Panning:" ) + " ", "" );
	m_panningKnob->setLabel( tr( "PAN" ) );

	basicControlsLayout->addWidget( m_panningKnob );
	basicControlsLayout->addStretch();

	// set up pitch knob
	m_pitchKnob = new knob( knobBright_26, NULL, tr( "Pitch" ) );
	m_pitchKnob->setHintText( tr( "Pitch:" ) + " ", " " + tr( "cents" ) );
	m_pitchKnob->setLabel( tr( "PITCH" ) );

	basicControlsLayout->addWidget( m_pitchKnob );

	// set up pitch range knob
	m_pitchRangeSpinBox= new LcdSpinBox( 2, NULL, tr( "Pitch range (semitones)" ) );
	m_pitchRangeSpinBox->setLabel( tr( "RANGE" ) );

	basicControlsLayout->addWidget( m_pitchRangeSpinBox );
	basicControlsLayout->addStretch();

	// setup spinbox for selecting FX-channel
	m_effectChannelNumber = new fxLineLcdSpinBox( 2, NULL, tr( "FX channel" ) );
	m_effectChannelNumber->setLabel( tr( "FX" ) );

	basicControlsLayout->addWidget( m_effectChannelNumber );

	basicControlsLayout->addStretch();


	QPushButton* saveSettingsBtn = new QPushButton( embed::getIconPixmap( "project_save" ), QString() );
	saveSettingsBtn->setMinimumSize( 32, 32 );

	connect( saveSettingsBtn, SIGNAL( clicked() ), this, SLOT( saveSettingsBtnClicked() ) );

	toolTip::add( saveSettingsBtn, tr( "Save current instrument track settings in a preset file" ) );
	saveSettingsBtn->setWhatsThis(
		tr( "Click here, if you want to save current instrument track settings in a preset file. "
			"Later you can load this preset by double-clicking it in the preset-browser." ) );

	basicControlsLayout->addWidget( saveSettingsBtn );

	generalSettingsLayout->addLayout( basicControlsLayout );


	m_tabWidget = new tabWidget( "", this );
	m_tabWidget->setFixedHeight( INSTRUMENT_HEIGHT + 10 );


	// create tab-widgets
	m_ssView = new InstrumentSoundShapingView( m_tabWidget );

	// FUNC tab
	QWidget* instrumentFunctions = new QWidget( m_tabWidget );
	QVBoxLayout* instrumentFunctionsLayout = new QVBoxLayout( instrumentFunctions );
	instrumentFunctionsLayout->setMargin( 5 );
	m_noteStackingView = new InstrumentFunctionNoteStackingView( &m_track->m_noteStacking );
	m_arpeggioView = new InstrumentFunctionArpeggioView( &m_track->m_arpeggio );

	instrumentFunctionsLayout->addWidget( m_noteStackingView );
	instrumentFunctionsLayout->addWidget( m_arpeggioView );
	instrumentFunctionsLayout->addStretch();

	// MIDI tab
	m_midiView = new InstrumentMidiIOView( m_tabWidget );

	// FX tab
	m_effectView = new EffectRackView( m_track->m_audioPort.effects(), m_tabWidget );

	m_tabWidget->addTab( m_ssView, tr( "ENV/LFO" ), 1 );
	m_tabWidget->addTab( instrumentFunctions, tr( "FUNC" ), 2 );
	m_tabWidget->addTab( m_effectView, tr( "FX" ), 3 );
	m_tabWidget->addTab( m_midiView, tr( "MIDI" ), 4 );

	// setup piano-widget
	m_pianoView = new PianoView( this );
	m_pianoView->setFixedSize( INSTRUMENT_WIDTH, PIANO_HEIGHT );

	vlayout->addWidget( generalSettingsWidget );
	vlayout->addWidget( m_tabWidget );
	vlayout->addWidget( m_pianoView );


	setModel( _itv->model() );

	updateInstrumentView();

	setFixedWidth( INSTRUMENT_WIDTH );
	resize( sizeHint() );

	QMdiSubWindow * subWin = engine::mainWindow()->workspace()->addSubWindow( this );
	Qt::WindowFlags flags = subWin->windowFlags();
	flags |= Qt::MSWindowsFixedSizeDialogHint;
	flags &= ~Qt::WindowMaximizeButtonHint;
	subWin->setWindowFlags( flags );

  // Hide the Size and Maximize options from the system menu
  // since the dialog size is fixed.
  QMenu * systemMenu = subWin->systemMenu();
  systemMenu->actions().at( 2 )->setVisible( false ); // Size
  systemMenu->actions().at( 4 )->setVisible( false ); // Maximize

  subWin->setWindowIcon( embed::getIconPixmap( "instrument_track" ) );
	subWin->setFixedSize( subWin->size() );
	subWin->hide();
}




InstrumentTrackWindow::~InstrumentTrackWindow()
{
	InstrumentTrackView::s_windowCache.removeAll( this );

	delete m_instrumentView;

	if( engine::mainWindow()->workspace() )
	{
		parentWidget()->hide();
		parentWidget()->deleteLater();
	}
}




void InstrumentTrackWindow::setInstrumentTrackView( InstrumentTrackView* view )
{
	if( m_itv && view )
	{
		m_itv->m_tlb->setChecked( false );
	}

	m_itv = view;
}




void InstrumentTrackWindow::modelChanged()
{
	m_track = castModel<InstrumentTrack>();

	m_nameLineEdit->setText( m_track->name() );

	m_track->disconnect( SIGNAL( nameChanged() ), this );
	m_track->disconnect( SIGNAL( instrumentChanged() ), this );

	connect( m_track, SIGNAL( nameChanged() ),
			this, SLOT( updateName() ) );
	connect( m_track, SIGNAL( instrumentChanged() ),
			this, SLOT( updateInstrumentView() ) );
	
	m_volumeKnob->setModel( &m_track->m_volumeModel );
	m_panningKnob->setModel( &m_track->m_panningModel );
	m_effectChannelNumber->setModel( &m_track->m_effectChannelModel );
	m_pianoView->setModel( &m_track->m_piano );

	if( m_track->instrument() && m_track->instrument()->flags().testFlag( Instrument::IsNotBendable ) == false )
	{
		m_pitchKnob->setModel( &m_track->m_pitchModel );
		m_pitchRangeSpinBox->setModel( &m_track->m_pitchRangeModel );
		m_pitchKnob->show();
		m_pitchRangeSpinBox->show();
	}
	else
	{
		m_pitchKnob->hide();
		m_pitchKnob->setModel( NULL );
		m_pitchRangeSpinBox->hide();
	}

	m_ssView->setModel( &m_track->m_soundShaping );
	m_noteStackingView->setModel( &m_track->m_noteStacking );
	m_arpeggioView->setModel( &m_track->m_arpeggio );
	m_midiView->setModel( &m_track->m_midiPort );
	m_effectView->setModel( m_track->m_audioPort.effects() );
	updateName();
}




void InstrumentTrackWindow::saveSettingsBtnClicked()
{
	FileDialog sfd( this, tr( "Save preset" ), "", tr( "XML preset file (*.xpf)" ) );

	QString presetRoot = configManager::inst()->userPresetsDir();
	if( !QDir( presetRoot ).exists() )
	{
		QDir().mkdir( presetRoot );
	}
	if( !QDir( presetRoot + m_track->instrumentName() ).exists() )
	{
		QDir( presetRoot ).mkdir( m_track->instrumentName() );
	}

	sfd.setAcceptMode( FileDialog::AcceptSave );
	sfd.setDirectory( presetRoot + m_track->instrumentName() );
	sfd.setFileMode( FileDialog::AnyFile );

	if( sfd.exec() == QDialog::Accepted &&
		!sfd.selectedFiles().isEmpty() &&
		!sfd.selectedFiles().first().isEmpty() )
	{
		DataFile::LocaleHelper localeHelper( DataFile::LocaleHelper::ModeSave );

		DataFile dataFile( DataFile::InstrumentTrackSettings );
		m_track->setSimpleSerializing();
		m_track->saveSettings( dataFile, dataFile.content() );
		QString f = sfd.selectedFiles()[0];
		dataFile.writeFile( f );
	}
}





void InstrumentTrackWindow::updateName()
{
	setWindowTitle( m_track->name().length() > 25 ? ( m_track->name().left(24)+"..." ) : m_track->name() );

	if( m_nameLineEdit->text() != m_track->name() )
	{
		m_nameLineEdit->setText( m_track->name() );
	}
}





void InstrumentTrackWindow::updateInstrumentView()
{
	delete m_instrumentView;
	if( m_track->m_instrument != NULL )
	{
		m_instrumentView = m_track->m_instrument->createView( m_tabWidget );
		m_tabWidget->addTab( m_instrumentView, tr( "PLUGIN" ), 0 );
		m_tabWidget->setActiveTab( 0 );

		m_ssView->setFunctionsHidden( m_track->m_instrument->flags().testFlag( Instrument::IsSingleStreamed ) );

		modelChanged(); 		// Get the instrument window to refresh
		m_track->dataChanged(); // Get the text on the trackButton to change
	}
}




void InstrumentTrackWindow::textChanged( const QString& newName )
{
	m_track->setName( newName );
	engine::getSong()->setModified();
}




void InstrumentTrackWindow::toggleVisibility( bool on )
{
	if( on )
	{
		show();
		parentWidget()->show();
		parentWidget()->raise();
	}
	else
	{
		parentWidget()->hide();
	}
}




void InstrumentTrackWindow::closeEvent( QCloseEvent* event )
{
	event->ignore();

	if( engine::mainWindow()->workspace() )
	{
		parentWidget()->hide();
	}
	else
	{
		hide();
	}

	m_itv->m_tlb->setFocus();
	m_itv->m_tlb->setChecked( false );
}




void InstrumentTrackWindow::focusInEvent( QFocusEvent* )
{
	m_pianoView->setFocus();
}




void InstrumentTrackWindow::dragEnterEventGeneric( QDragEnterEvent* event )
{
	stringPairDrag::processDragEnterEvent( event, "instrument,presetfile,pluginpresetfile" );
}




void InstrumentTrackWindow::dragEnterEvent( QDragEnterEvent* event )
{
	dragEnterEventGeneric( event );
}




void InstrumentTrackWindow::dropEvent( QDropEvent* event )
{
	QString type = stringPairDrag::decodeKey( event );
	QString value = stringPairDrag::decodeValue( event );

	if( type == "instrument" )
	{
		m_track->loadInstrument( value );

		engine::getSong()->setModified();

		event->accept();
	}
	else if( type == "presetfile" )
	{
		DataFile dataFile( value );
		InstrumentTrack::removeMidiPortNode( dataFile );
		m_track->setSimpleSerializing();
		m_track->loadSettings( dataFile.content().toElement() );

		engine::getSong()->setModified();

		event->accept();
	}
	else if( type == "pluginpresetfile" )
	{
		const QString ext = FileItem::extension( value );
		Instrument * i = m_track->instrument();

		if( !i->descriptor()->supportsFileType( ext ) )
		{
			i = m_track->loadInstrument( engine::pluginFileHandling()[ext] );
		}

		i->loadFile( value );

		event->accept();
	}
}




void InstrumentTrackWindow::saveSettings( QDomDocument& doc, QDomElement & thisElement )
{
	thisElement.setAttribute( "tab", m_tabWidget->activeTab() );
	MainWindow::saveWidgetState( this, thisElement );
}




void InstrumentTrackWindow::loadSettings( const QDomElement& thisElement )
{
	m_tabWidget->setActiveTab( thisElement.attribute( "tab" ).toInt() );
	MainWindow::restoreWidgetState( this, thisElement );
	if( isVisible() )
	{
		m_itv->m_tlb->setChecked( true );
	}
}




#include "moc_InstrumentTrack.cxx"


