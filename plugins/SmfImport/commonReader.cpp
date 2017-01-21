/*
 * commonReader.cpp - import backend.
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

#include <QString>
#include <QDir>
#include <QApplication>
#include <QMessageBox>
#include <QProgressDialog>
#include <drumstick.h>

#include "GuiApplication.h"
#include "TrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "AutomationPattern.h"
#include "MidiTime.h"
#include "MainWindow.h"

#include "SmfMidiCC.h"
#include "SmfMidiChannel.h"

#include "commonReader.h"

#define CHECK_TRACK track = ( track == noTrack ? chan : track );
#define PITCH_RANGE_RPN_CODE {track, 0}
#define CC_RPN_SEND rpn_data[0] = track; \
	rpn_data[1] = value;

#define NOTE_EVENT_DEFINITION const int time = 0; \
	const int channel = time + 1; \
	const int note_pitch = channel + 1; \
	const int note_vol = note_pitch + 1;

commonReader::commonReader( TrackContainer *tc, const QString hintText ):
	m_tc( tc ),
	beatsPerTact( 4 ),
	pitchBendMultiply( defaultPitchRange ),
	pd( hintText, QGuiApplication::tr( "Cancel" ),
	    0, preTrackSteps, gui->mainWindow() )
{
	m_currentTrackName.first = -1;
	pd.setWindowTitle( TrackContainer::tr( "Please wait..." ) );
	pd.setWindowModality( Qt::WindowModal );
	pd.setMinimumDuration( 0 );
	pd.setValue( 0 );


	MeterModel & timeSigMM = Engine::getSong()->getTimeSigModel();
	timeSigNumeratorPat = AutomationPattern::globalAutomationPattern(
				      &timeSigMM.numeratorModel() );
	timeSigDenominatorPat = AutomationPattern::globalAutomationPattern(
					&timeSigMM.denominatorModel() );

	ticksPerBeat = DefaultTicksPerTact / beatsPerTact;

	if( note_list.size() )
		note_list.clear();

	if( rpn_msbs.size() )
		rpn_msbs.clear();

	if( rpn_lsbs.size() )
		rpn_lsbs.clear();

	AutomationPattern * tap = m_tc->tempoAutomationPattern();
	tap->clear();
}

commonReader::~commonReader()
{
	printf( "destory fileReader\n" );

	for( int c = 0; c < 256; c++ )
	{
		if( !chs[c].hasNotes && chs[c].it )
		{
			printf( " Should remove empty track\n" );
			// must delete trackView first - but where is it?
			//m_tc->removeTrack( chs[c].it );
			//chs[c].it->deleteLater();
		}
	}

}

void commonReader::CCHandler( long tick, int track, int ctl, int value )
{
	QString trackName;

	if( m_currentTrackName.first == m_currentTrack )
	{
		trackName = m_currentTrackName.second;
	}
	else
	{
		trackName = TrackContainer::tr( "Track %1" ).arg( track + 1 );
	}

	SmfMidiChannel * ch = chs[track].create( m_tc, trackName );
	AutomatableModel * objModel = NULL;
	int * rpn_data = new int[2];

	bool flag_rpn_msb = false;
	bool flag_rpn_lsb = false;

	if( ctl <= 129 )
	{
		switch( ctl )
		{
			case bankEventId:
				if( ch->isSF2 && ch->it_inst )
				{
					objModel = ch->it_inst->childModel( "bank" );
					printf( "Tick=%ld: BANK SELECT %d\n", tick, value );
				}

				break;

			case 6:
				for( int c = 0; c < rpn_msbs.size(); c++ )
				{
					rpn_data = rpn_msbs[c];

					if( rpn_data[0] == track && rpn_data[1] == 0 )
					{
						flag_rpn_msb = true;
						rpn_msbs.removeAt( c );
						delete rpn_data;
					}
				}

				for( int c = 0; c < rpn_lsbs.size(); c++ )
				{
					rpn_data = rpn_lsbs[c];

					if( rpn_data[0] == track && rpn_data[1] == 0 )
					{
						flag_rpn_lsb = true;
						rpn_lsbs.removeAt( c );
						delete rpn_data;
					}
				}

				if( flag_rpn_lsb && flag_rpn_msb )
				{
					objModel = ch->it->pitchRangeModel();
					pitchBendMultiply = value;
				}

				break;

			case volumeEventId:
				objModel = ch->it->volumeModel();
				value = value * 100 / 127;
				break;

			case panEventId:
				objModel = ch->it->panningModel();
				// value may be nagetive.
				value = value * 100 / 127;
				break;

			// RPN LSB
			case 100:
				CC_RPN_SEND
				rpn_lsbs << rpn_data;
				break;

			// RPN MSB
			case 101:
				CC_RPN_SEND
				rpn_msbs << rpn_data;
				break;

			case pitchBendEventId:
				objModel = ch->it->pitchModel();
				value = value * 100 / 8192 * pitchBendMultiply;
				break;

			case programEventId:
				if( ch->isSF2 && ch->it_inst )
					objModel = ch->it_inst->childModel( "patch" );

				break;

			default:
				// TODO: something useful for other CCs
				printf( "Tick=%ld: Unused CC %d with value=%d\n",
					tick, ctl, value );
				break;
		}

		if( objModel )
		{
			if( tick == 0 )
				objModel->setInitValue( value );
			else
			{
				if( ccs[track][ctl].at == NULL )
				{
					ccs[track][ctl].create( m_tc, trackName + " > " + objModel->displayName() );
				}

				ccs[track][ctl].putValue( tick * tickRate , objModel, value );
			}
		}
	}

}

void commonReader::programHandler( long tick, int chan, int patch,
				   int track )
{
	CHECK_TRACK
	QString trackName = TrackContainer::tr( "Track %1" ).arg( chan + 1 );
	SmfMidiChannel * ch = chs[track].create( m_tc, trackName );

	if( ch->isSF2 )
	{
		// AFAIK, 128 should be the standard bank for drums in SF2.
		// If not, this has to be made configurable.
		ch->it_inst->childModel( "bank" )->setValue( chan != 9 ? 0 : 128 );

		if( tick == 0 )
			ch->it_inst->childModel( "patch" )->setValue( patch );
		else
			CCHandler( tick, track, programEventId, patch );
	}
	else
	{
		const QString num = QString::number( patch );
		const QString filter = QString().fill( '0', 3 - num.length() ) + num + "*.pat";
		const QString dir = "/usr/share/midi/"
				    "freepats/Tone_000/";
		const QStringList files = QDir( dir ).
					  entryList( QStringList( filter ) );

		if( ch->it_inst && !files.empty() )
		{
			ch->it_inst->loadFile( dir + files.front() );
		}
	}

}

void commonReader::timeSigHandler( long tick, int num, int den )
{
	printf( "Another timesig at %f\n", tick * tickRate );
	timeSigNumeratorPat->putValue( tick * tickRate, num );
	timeSigDenominatorPat->putValue( tick * tickRate, den );
	pd.setValue( preTrackSteps );
}

void commonReader::tempoHandler( long tick, int tempo )
{
	AutomationPattern * tap = m_tc->tempoAutomationPattern();

	if( tap )
	{
		tap->putValue( static_cast<double>( tick ) * tickRate, tempo );
	}
}

void commonReader::textHandler( int text_type, const QString &data , int track )
{
	switch ( text_type )
	{
		case 3:

			// Track Name
			m_currentTrack = ( track == -10 ? m_currentTrack : track );

			if ( m_currentTrack == noTrack )  // Meta name, i.e. The real title/name of the current song file
			{
				return;  // Don't know how to handle this in LMMS
			}

			m_currentTrackName.first = m_currentTrack;
			m_currentTrackName.second = data;

			if( chs[m_currentTrack].it )
			{
				chs[m_currentTrack].setName( data );
			}
			else
			{
				chs[m_currentTrack].create( m_tc, data );
			}

		case 5:

		// Lyrics
		// How to handle lyrics?
		case 6:

		// MIDI Marker Description
		// How to handle markers & cue points?

		default:
			printf( "Unknown Text Event: %d %s", text_type, data.toStdString().c_str() );
	}

}

void commonReader::timeBaseHandler( int timebase )
{
	tickRate = ticksPerBeat / static_cast<double>( timebase );
}

void commonReader::trackStartHandler()
{
	m_currentTrack ++;
}

// when use this, addNoteEvent() should be called later, otherwise it will cause memeory leak.
void commonReader::insertNoteEvent( long tick, int chan, int pitch,
				    int vol, int track )
{
	NOTE_EVENT_DEFINITION
	CHECK_TRACK

	int *note = new int[4];
	note[time] = tick;
	note[channel] = track;
	note[note_pitch] = pitch;
	note[note_vol] = vol;
	note_list << note;
}

void commonReader::addNoteEvent(long tick, int chan,
				 int pitch, int vol, int track )
{
	NOTE_EVENT_DEFINITION
	CHECK_TRACK

	if( !note_list.size() && vol )
	{
		addNoteEvent( tick, chan, pitch, vol, 1, track );
	}

	for( int c = 0; c < note_list.size(); c++ )
	{
		int *note;
		note = note_list[c];

		if( note[channel] == track && note[note_pitch] == pitch
				&& tick >= note[time] )
		{
			int dur = tick - note[time];
			addNoteEvent( note[time], chan, pitch, note[note_vol], dur, track );
			note_list.removeAt( c );
			delete note;
			break;
		}
	}

	if( pd.maximum() <= chan + preTrackSteps )
		pd.setMaximum( pd.maximum() + preTrackSteps );

	if( pd.value() <= chan + preTrackSteps )
		pd.setValue( chan + preTrackSteps );

}

void commonReader::addNoteEvent( long tick, int chan, int pitch, int vol,
				 int dur, int track )
{
	CHECK_TRACK

	QString trackName = TrackContainer::tr( "Track %1" ).arg( track + 1 );
	SmfMidiChannel * ch = chs[track].create( m_tc, trackName );
	double realDur = dur * tickRate;
	Note n( ( realDur < 1 ? 1 : realDur ), static_cast<double>( tick ) * tickRate, pitch - 12, vol * 200 / 127 );
	ch->addNote( n );

}

void commonReader::errorHandler( const QString &errorStr )
{
	printf( "SmfImport::readFile(): got error %s\n",
		errorStr.toStdString().c_str() );
}
