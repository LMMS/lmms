/*
 * midiWriter.cpp - export backend.
 *
 * Copyright (c) 2016-2017 Tony Chyi <tonychee1989/at/gmail.com>
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

#include <QObject>
#include <drumstick.h>

#include "AutomationTrack.h"
#include "Engine.h"
#include "Instrument.h"
#include "InstrumentTrack.h"
#include "Midi.h"
#include "MidiEvent.h"
#include "MidiTime.h"
#include "Note.h"
#include "Pattern.h"
#include "Song.h"
#include "Track.h"

#include "midiWriter.h"

#define SEARCH_AUTOTRACKS_START( automodel ) \
	for( int autoTrackIndex : AutomationTracks ) \
	{ \
		AutomationTrack * autoTrack = dynamic_cast< AutomationTrack * >( m_tl[ autoTrackIndex ] ); \
		for( TrackContentObject * tco : autoTrack->getTCOs() ) \
		{ \
			tick_t offset = tco->startPosition().getTicks();\
			AutomationPattern * a_tco = dynamic_cast< AutomationPattern * >( tco ); \
			if( a_tco->firstObject() != automodel ) \
				continue; \
			const AutomationPattern::timeMap tp = a_tco->getTimeMap(); \
			AutomationPattern::timeMap::const_iterator i; \
			for( i=tp.constBegin(); i!=tp.constEnd(); ++i ) \
			{\

#define SEARCH_AUTOTRACKS_END \
			}\
		}\
	}

#define PROGRESSION_CALC_START \
	if( i != tp.constBegin() && \
		a_tco->progressionType() != AutomationPattern::DiscreteProgression &&\
		i.value() != (i-1).value() )\
	{\
		for( int ii = 1; ii < i.key() - (i-1).key(); ii++ ) \
		{\

#define PROGRESSION_CALC_END \
		}\
	}\


#define INSERT_CC_EVENT( type, zoom, t, offset, val ) \
	event = new MidiEvent( MidiControlChange );\
	event->setChannel( currentChannel );\
	event->setControllerNumber( type );\
	event->setControllerValue( val * zoom );\
	EventList.insert( t+offset, event );

#define INSERT_PITCH_EVENT( t, offset, val ) \
	event = new MidiEvent( MidiPitchBend );\
	event->setChannel( currentChannel );\
	event->setPitchBend( val / pitchBendMultiply * ( MidiMaxPitchBend / 2 ) / 100 + ( MidiMaxPitchBend / 2 + 1 ) );\
	EventList.insert( t+offset, event );

#define INSERT_PROG_EVENT( t, offset, val ) \
	event = new MidiEvent( MidiProgramChange ); \
	event->setChannel( currentChannel );\
	event->setParam( 0, val ); \
	EventList.insert( t+offset, event );

#define INSERT_TEMPO_EVENT( t, val ) \
	MidiEvent * event = new MidiEvent( MidiMetaEvent ); \
	event->setMetaEvent( MidiSetTempo ); \
	event->setParam( 0, val ); \
	EventList.insert( t, event ); \

#define FIND_PITCH_MULTYPLY( t ) \
	QMap< int, int >::const_iterator j;\
	for( j=PitchBendMultiply.constBegin(); j!=PitchBendMultiply.constEnd(); ++j )\
	{\
		if( j.key() <= t )\
			pitchBendMultiply = j.value();\
		else\
			break;\
	}



midiWriter::midiWriter( const TrackContainer::TrackList &tracks ):
	m_tl( tracks ), m_seq( new drumstick::QSmf( this ) ),
	flagRpnPitchBendRangeSent( false )
{
	tempoPat = Engine::getSong()->tempoAutomationPattern();

	MeterModel & timeSigMM = Engine::getSong()->getTimeSigModel();
	timeSigNumPat = AutomationPattern::globalAutomationPattern(
					&timeSigMM.numeratorModel() );
	timeSigDenPat = AutomationPattern::globalAutomationPattern(
					&timeSigMM.denominatorModel() );

	for( int i=0; i < MidiChannelCount; i++)
		ChannelProg[i] = -1;

	// We need multi-track format, so set format 1.
	m_seq->setFileFormat( 1 );

	int trackIndex = 0;
	for( const Track* track : tracks )
	{
		switch( track->type() )
		{
		case Track::InstrumentTrack:
			if( dynamic_cast< const InstrumentTrack * >( track )->instrumentName() == "Sf2 Player" )
				InstrumentTracks << trackIndex;
			break;
		case Track::AutomationTrack:
			AutomationTracks << trackIndex;
			break;
		default:
			break;
		}
		trackIndex++;
	}

	// Track 0 is to write global events.
	m_seq->setTracks( InstrumentTracks.size()+1 );
	// 1 quarter note = 960 midi ticks.
	m_seq->setDivision( DefaultMidiDivision );

	tickRate = (double)DefaultTicksPerTact / 4.f / DefaultMidiDivision ;

	connect( m_seq, SIGNAL( signalSMFWriteTrack(int) ),
			 this, SLOT( writeTrackEvent(int)) );

}


midiWriter::~midiWriter()
{
	printf( "destroy midiWriter\n" );
	delete m_seq;
}

void midiWriter::writeFile( const QString &fileName )
{

	m_seq->writeToFile( fileName );
}

void midiWriter::writeEventToFile()
{
	int lastTime = 0;
	QMultiMap< long, MidiEvent * >::const_iterator i;

	for( i=EventList.constBegin(); i!=EventList.constEnd(); ++i )
	{
		MidiEvent *event = i.value();
		int deltaTime = i.key() - lastTime;
		double realDeltaTime = deltaTime / tickRate;

		switch( event->type() )
		{
		case MidiMetaEvent:
			switch( event->metaEvent() )
			{
			case MidiTimeSignature:
				m_seq->writeTimeSignature( (long) realDeltaTime, event->param( 0 ),
										   intSqrt( event->param( 1 ) ),
										   DefaultMidiClockPerMetronomeClick * 4 /
										   event->param(1), 8 );

				break;

			case MidiSetTempo:
				m_seq->writeBpmTempo( (int) realDeltaTime, event->param( 0 ) );
				break;

			default:
				break;
			}
			break;

		case MidiControlChange:
			switch( event->controllerNumber() )
			{
			case MidiControllerPan:
				m_seq->writeMidiEvent( (long) realDeltaTime, control_change,
									   event->channel(),
									   MidiControllerPan,
									   event->controllerValue() - 127 );

				break;
			case MidiControllerDataEntry:
				if( !flagRpnPitchBendRangeSent && event->controllerValue() > 2 )
				{
					m_seq->writeMidiEvent( (long) realDeltaTime, control_change,
										   event->channel(),
										   MidiControllerRegisteredParameterNumberMSB,
										   0 );
					m_seq->writeMidiEvent( 0, control_change,
										   event->channel(),
										   MidiControllerRegisteredParameterNumberLSB,
										   MidiPitchBendSensitivityRPN );
					m_seq->writeMidiEvent( 0, control_change,
										   event->channel(),
										   MidiControllerDataEntry,
										   event->controllerValue() );
					m_seq->writeMidiEvent( 0, control_change,
										   event->channel(),
										   38, 0);

					flagRpnPitchBendRangeSent = true;
				}
				else if( event->controllerValue() <= 2 )
					break;
			default:
					m_seq->writeMidiEvent( (long) realDeltaTime, control_change,
										   event->channel(),
										   event->controllerNumber(),
										   event->controllerValue() );

			}
			break;

		case MidiPitchBend:
			m_seq->writeMidiEvent( (long) realDeltaTime, pitch_wheel,
								   event->channel(),
								   event->pitchBend() & 127,
								   (event->pitchBend() & 32512) >> 7 );
			break;

		case MidiProgramChange:
			m_seq->writeMidiEvent( (long) realDeltaTime, program_chng,
								   event->channel(), event->program() );
			break;

		case MidiNoteOff:
		case MidiNoteOn:
			m_seq->writeMidiEvent( (long) realDeltaTime, event->type(),
								   event->channel(), event->param(0),
								   event->param(1) );
			break;

		default:
			break;
		}
		lastTime = i.key();

		// Should release memory use.
		delete event;
	}
}

void midiWriter::insertTempoEvent()
{
	AutomationPattern::timeMap time_map = tempoPat->getTimeMap();
	AutomationPattern::timeMap::const_iterator i;
	AutomationPattern::timeMap::const_iterator last = time_map.constBegin() ;

	for( i = time_map.constBegin(); i != time_map.constEnd(); ++i )
	{
		if( i != time_map.constBegin() &&
			tempoPat->progressionType() != AutomationPattern::DiscreteProgression &&
			i.value() != last.value() )
		{
			for(int ii = 1; ii < i.key() - (i-1).key(); ii++ )
			{
				INSERT_TEMPO_EVENT( (i-1).key()+ii, tempoPat->valueAt( MidiTime( (i-1).key()+ii ) ) );
			}
		}
		INSERT_TEMPO_EVENT( i.key(), i.value() );
		last = i;
	}
}

void midiWriter::insertTimeSigEvent()
{
	// In LMMS, time sig will not always appear at the same time.
	AutomationPattern::timeMap & timeSigNum_map = timeSigNumPat->getTimeMap();
	AutomationPattern::timeMap & timeSigDen_map = timeSigDenPat->getTimeMap();
	AutomationPattern::timeMap::const_iterator i;

	QList< int > TimeList;

	// Step 1, fetch time point from Num and Den.
	for( i=timeSigNum_map.constBegin(); i!=timeSigNum_map.constEnd(); ++i )
		TimeList << i.key();
	for( i=timeSigDen_map.constBegin(); i!=timeSigDen_map.constEnd(); ++i )
		TimeList << i.key();

	// Step 2, unique and sort.
	TimeList = TimeList.toSet().toList();
	sort(TimeList.begin(), TimeList.end());

	// Step 3, insert event.
	uint16_t num=4, den=2;

	for( int i : TimeList )
	{
		if( timeSigDen_map.contains(i) )
			num = (uint16_t) timeSigDen_map[i];
		if( timeSigNum_map.contains(i) )
			den = (uint16_t) timeSigNum_map[i];

		MidiEvent * event = new MidiEvent( MidiMetaEvent );
		event->setMetaEvent( MidiTimeSignature );
		/*
		 * Param 0 => Numerator
		 * Param 1 => Denominator
		 */
		event->setParam( 0, num );
		event->setParam( 1, den );
		EventList.insert( i, event );
	}
}

void midiWriter::insertCCEvent( InstrumentTrack *track )
{
	MidiEvent * event;

	INSERT_CC_EVENT( MidiControllerPan, 127 / 100 + 127, 0, 0, track->panningModel()->value() );
	SEARCH_AUTOTRACKS_START( track->panningModel() );
		PROGRESSION_CALC_START;
			INSERT_CC_EVENT( MidiControllerPan, 127 / 100 + 127, (i-1).key(), offset, a_tco->valueAt( MidiTime( (i-1).key()+ii+offset ) ) );
		PROGRESSION_CALC_END;
		INSERT_CC_EVENT( MidiControllerPan, 127 / 100 + 127, i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END;

	INSERT_CC_EVENT( MidiControllerMainVolume, 127 / 200, 0, 0, track->volumeModel()->value() );
	SEARCH_AUTOTRACKS_START( track->volumeModel() );
		PROGRESSION_CALC_START;
			INSERT_CC_EVENT( MidiControllerMainVolume, 127 / 200, (i-1).key(), offset, a_tco->valueAt( MidiTime( (i-1).key()+ii+offset ) ) );
		PROGRESSION_CALC_END;
		INSERT_CC_EVENT( MidiControllerMainVolume, 127 / 200, i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END;

	PitchBendMultiply.insert( 0, track->pitchRangeModel()->value() );
	INSERT_CC_EVENT( MidiControllerDataEntry, 1, 0, 0, track->pitchRangeModel()->value() );
	SEARCH_AUTOTRACKS_START( track->pitchRangeModel() );
		PROGRESSION_CALC_START;
			PitchBendMultiply.insert( (i-1).key()+ii+offset, a_tco->valueAt( MidiTime( (i-1).key()+ii ) ) );
			INSERT_CC_EVENT( MidiControllerDataEntry, 1, (i-1).key(), offset, a_tco->valueAt( MidiTime( (i-1).key()+ii+offset ) ) );
		PROGRESSION_CALC_END;
		PitchBendMultiply.insert( i.key()+offset, i.value() );
		INSERT_CC_EVENT( MidiControllerDataEntry, 1, i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END;

	INSERT_CC_EVENT( MidiControllerBankSelect, 1, 0, 0, dynamic_cast< IntModel * >( track->instrument()->childModel( "bank" ) )->value() );
	SEARCH_AUTOTRACKS_START( track->instrument()->childModel( "bank" ) );
		PROGRESSION_CALC_START;
			INSERT_CC_EVENT( MidiControllerBankSelect, 1, (i-1).key(), offset, a_tco->valueAt( MidiTime( (i-1).key()+ii+offset ) ) );
		PROGRESSION_CALC_END;
		INSERT_CC_EVENT( MidiControllerBankSelect, 1, i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END;
}

void midiWriter::insertNoteEvent( InstrumentTrack *track )
{
	MidiEvent * event;
	for( TrackContentObject * tco : track->getTCOs() )
	{
		tick_t offset = tco->startPosition().getTicks();
		const NoteVector &notes = dynamic_cast< Pattern * >( tco )->notes();

		for( Note * i : notes )
		{
			flagCurrentTrackHasNotes = true;

			event = new MidiEvent( MidiNoteOn );
			// TODO: How can i set the channel?
			event->setChannel( currentChannel );
			event->setParam( 0, i->key()+12 );
			event->setParam( 1, i->getVolume()*127/200 );
			EventList.insert( i->pos().getTicks()+offset, event );

			event = new MidiEvent( MidiNoteOff );
			event->setChannel( currentChannel );
			event->setParam( 0, i->key()+12 );
			event->setParam( 1, 64 );
			EventList.insert( i->endPos().getTicks()+offset, event );
		}
	}
}

void midiWriter::insertProgramEvent( InstrumentTrack *track )
{
	MidiEvent * event;
	INSERT_PROG_EVENT( 0, 0, dynamic_cast<IntModel * >( track->instrument()->childModel( "patch" ) )->value() );

	SEARCH_AUTOTRACKS_START( track->instrument()->childModel( "patch" ) );
		INSERT_PROG_EVENT( i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END;
}

void midiWriter::insertPitchEvent( InstrumentTrack *track )
{
	MidiEvent * event;
	int pitchBendMultiply;
	pitchBendMultiply = track->pitchRangeModel()->value();
	INSERT_PITCH_EVENT( 0, 0, track->pitchModel()->value() );

	SEARCH_AUTOTRACKS_START( track->pitchModel() )
		PROGRESSION_CALC_START;
			FIND_PITCH_MULTYPLY( (i-1).key() + ii + offset );
			INSERT_PITCH_EVENT( (i-1).key() + ii, offset, a_tco->valueAt( MidiTime( (i-1).key()+ii+offset ) ) );
		PROGRESSION_CALC_END;

		FIND_PITCH_MULTYPLY( i.key() + offset );
		INSERT_PITCH_EVENT( i.key(), offset, i.value() );
	SEARCH_AUTOTRACKS_END
}

void midiWriter::allocateChannel( InstrumentTrack *track )
{
	// patman not supported.
	if( track->instrumentName() == "Sf2 Player" )
	{
		Instrument * tr_inst = track->instrument();

		// Find drumkit.
		if( dynamic_cast< IntModel *>( tr_inst->childModel( "bank" ) )->value() >= 128 )
		{
				currentChannel = DrumChannel;
				return;
		}

		int8_t prog = dynamic_cast< IntModel * >( tr_inst->childModel( "patch" ) )->value();

		// Step 1: Find unused channel
		for( currentChannel = 0; currentChannel < 16; currentChannel++ )
		{
			if ( currentChannel == DrumChannel )
				continue;

			if ( ChannelProg[ currentChannel ] == -1 )
			{
				ChannelProg[ currentChannel ] = prog;
				return;
			}
		}

		// Step 2: If no unused channel found, find channel has the same program.
		for( currentChannel = 0; currentChannel < 16; currentChannel++ )
		{
			if ( currentChannel == DrumChannel )
				continue;

			if( ChannelProg[ currentChannel ] == prog )
				return;
		}

		// Step 3: If no suitable channel, give it randomly.
		do
		{
			currentChannel = qrand() % 16;
		} while( currentChannel == DrumChannel );
	}
}

// Just for 2, 4, 8, 16 etc.
int midiWriter::intSqrt( int n )
{
	int i;
	for( i = 0; n >> i < 1; i++ );
	return i;
}

// Slot
void midiWriter::writeTrackEvent( int track )
{
	DataFile dataFile( DataFile::SongProject );
	flagRpnPitchBendRangeSent = false;
	flagCurrentTrackHasNotes = false;
	currentChannel = -1;

	EventList.clear();
	PitchBendMultiply.clear();

	if( track == 0 )
	{
		// Export by LMMS
		m_seq->writeMetaEvent( 0, copyright_notice, EXPORTED_BY );
		m_seq->writeMidiEvent( 0, system_exclusive, 5, GM_SYSEX );
		insertTempoEvent();
		insertTimeSigEvent();
	}
	else
	{
		// Write track name.
		Track* currentTrack = m_tl[ InstrumentTracks[ track-1 ] ];
		m_seq->writeMetaEvent( 0, sequence_name, currentTrack->name() );

		// TODO: Get tracks and automation pattern.
		InstrumentTrack *instTrack = dynamic_cast< InstrumentTrack* >( currentTrack );
		instTrack->saveState( dataFile, dataFile.content() );
		allocateChannel( instTrack );

		// When read EventList, it will be reverse.
		insertNoteEvent( instTrack );
		insertCCEvent( instTrack );
		insertPitchEvent( instTrack );
		insertProgramEvent( instTrack );
	}

	// Write event to file;
	if( flagCurrentTrackHasNotes || track == 0 )
		writeEventToFile();
	// Do not forget ending this track.
	m_seq->writeMetaEvent( 0, end_of_track );
}
