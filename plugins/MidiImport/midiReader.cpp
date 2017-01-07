/*
 * midiReader.cpp - support for importing MIDI files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
#include <QProgressDialog>
#include <QApplication>
#include <drumstick.h>

#include "TrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "AutomationPattern.h"
#include "MidiTime.h"

#include "smfMidiCC.h"
#include "smfMidiChannel.h"

#include "midiReader.h"

#define PITCH_RANGE_RPN_CODE {chan, 0}

midiReader::midiReader( TrackContainer* tc ) :
	pd( QProgressDialog( TrackContainer::tr( "Importing MIDI-file..." ),
		TrackContainer::tr( "Cancel" ), 0, preTrackSteps, gui->mainWindow()) ),
	m_tc( tc ),
	beatsPerTact( 4 ),
	m_divistion( 120 ),
	pitchBendMultiply( defaultPitchRange );

{

	m_seq = new drumstick::QSmf(this);

	pd.setWindowTitle( TrackContainer::tr( "Please wait..." ) );
	pd.setWindowModality(Qt::WindowModal);
	pd.setMinimumDuration( 0 )
	pd.setValue(0);


	timeSigMM = Engine::getSong()->getTimeSigModel();
	timeSigNumeratorPat = AutomationPattern::globalAutomationPattern(
				&timeSigMM.numeratorModel());
	timeSigDenominatorPat = AutomationPattern::globalAutomationPattern(
				&timeSigMM.denominatorModel());

	ticksPerBeat = DefaultBeatsPerTact / beatsPerTact;

	if(note_list.size() != 0)
		note_list.clear();

	if(rpn_msbs.size() != 0)
		rpn_msbs.clear();

	if(rpn_lsbs.size() != 0)
		rpn_lsbs.clear();

	// Connect to slots.
	connect(m_seq, SIGNAL(signalSMFTimeSig(int,int,int,int)),
			this, SLOT(timeSigEvent(int,int,int,int)));

	connect(m_seq, SIGNAL(signalSMFTempo(int)),
			this, SLOT(tempoEvent(int)));

	connect(m_seq, SIGNAL(signalSMFError(QString)),
			this, SLOT(errorHandler(QString));

	connect(m_seq, SIGNAL(signalSMFCtlChange(int,int,int)),
			this, SLOT(ctlChangeEvent(int,int,int))));

	connect(m_seq, SIGNAL(signalSMFPitchBend(int,int)),
			this, SLOT(pitchBendEvent(int,int)));

	connect(m_seq, SIGNAL(signalSMFNoteOn(int,int,int)),
			this, SLOT(noteOnEvent(int,int,int)));

	connect(m_seq, SIGNAL(signalSMFNoteOff(int,int,int)),
			this, SLOT(noteOffEvent(int,int,int)));

	connect(m_seq, SIGNAL(signalSMFProgram(int,int)),
			this, SLOT(programEvent(int,int)));

	connect(m_seq, SIGNAL(signalSMFText(int,QString)),
			this, SLOT(textEvent(int,QString)));

	connect(m_seq, SIGNAL(signalSMFTrackStart()),
			this, SLOT(trackStartEvent());

	connect(m_seq, SIGNAL(signalSMFTrackEnd()),
			this, SLOT(trackEndEvent()));

	connect(m_seq, SIGNAL(signalSMFProgram(int,int)),
			this, SLOT(programEvent(int,int)));



}

midiReader::~midiReader()
{
	printf("destroy midiReader\n")
	delete m_seq;

	for( int c=0; c<256; c++){
		if( !chs[c].hasNotes && chs[c].it ) {
			printf(" Should remove empty track\n");
			// must delete trackView first - but where is it?
			//tc->removeTrack( chs[c].it );
			//it->deleteLater();
		}
	}
}

void midiReader::read(QString &fileName)
{
	m_seq->readFromFile(fileName);
	pd.setMaximum( m_seq->getTracks() + preTrackSteps );
	pd.setValue(1);
}

void midiReader::CCHandler(int chan, int ctl, int value)
{
	QString trackName = QString( tr( "Track" ) + " %1").arg(chan);
	smfMidiChannel * ch = chs[chan].create( tc, trackName);
	if( ctl <= 129 )
	{
		AutomatableModel * objModel = NULL;
		switch( ctl )
		{
		case 0:
			if( ch->isSF2 && ch->it_inst )
			{
				objModel = ch->it_inst->childModel( "bank" );
				printf("Tick=%d: BANK SELECT %d\n", m_seq->getCurrentTime(), value);
			}
			break;

		case 6:
			int[2] rpn_msb = PITCH_RANGE_RPN_CODE;
			int[2] rpn_lsb = PITCH_RANGE_RPN_CODE;

			int rpn_msb_i = rpn_msbs.indexOf(rpn_msb);
			int rpn_lsb_i = rpn_lsbs.indexOf(rpn_lsb);

			if(rpn_msb_i != -1 && rpn_lsb_i != -1)
			{
				objModel = ch->it->pitchRangeModel();
				pitchBendMultiply = value;
			}
			break;
		case 7:
			objModel = ch->it->volumeModel();
			value = value * 100 / 127;
			break;
		case 10:
			objModel = ch->it->panningModel();
			// value may be nagetive.
			value = value * 100 / 127;
			break;

		// RPN LSB
		case 100:
			int[2] data = {chan, value};
			rpn_lsbs << data;
			break;

		// RPN MSB
		case 101:
			int[2] data = {chan, value};
			rpn_msbs << data;
			break;

		case 128:
			objModel = ch->it->pianoModel();
			value = value * 100 / 8192 * pitchBendMultiply;
			break;

		case 129:
			if( ch->isSF2 && ch->it_inst )
				objModel = ch->it_inst->childModel( "patch" );
			break;
		default:
			// TODO: something useful for other CCs
			printf("Tick=%d: Unused CC %d with value=%d\n",
				   m_seq->getCurrentTime(), ctl, value);
			break;
		}

		if( objModel )
		{
			if( m_seq->getCurrentTime() )
				objModel->setInitValue( value );
			else
			{
				if( ccs[ctl].at == NULL )
				{
					ccs[ctl].create( tc, trackName + " > " + objModel->displayName());
				}
				ccs[ccid].putValue( m_seq->getCurrentTime()*tickRate, objModel, value);
			}
		}
	}
}

void midiReader::addNoteEvent(int chan, int pitch, int vol=0)
{
	const int time = 0;
	const int channel = time + 1;
	const int note_pitch = channel + 1;
	const int note_vol = channel + 1;

	QString trackName = QString( tr( "Track" ) + " %1").arg(chan);
	smfMidiChannel * ch = chs[chan].create( tc, trackName );

	for(int c=0; c<note_list.size(); c++)
	{
		int[4] note = note_list[c];
		if(note[channel] == chan && note[note_pitch] == pitch
				&& m_seq->getCurrentTime() >= note[time])
		{
			int ticks = (m_seq->getCurrentTime() - note[time]) * tickRate;
			Note n( (ticks < 1 ? 1 : ticks ),
					note[time] * tickRate,
					note[note_pitch] - 12,
					note[note_vol] );
			note_list.removeAt(c);
			ch->addNote( n );
			break;
		}
	}
}

// Slots below.

// b0: Numerator
// b1: Denominator (exponent in a power of two)
// b2: Number of MIDI clocks per metronome click
// b3: Number of notated 32nd notes per 24 MIDI clocks
void midiReader::timeSigEvent(int b0, int b1, int b2, int b3)
{
	if(/* m_seq->getCurrentTime() == 0*/ true )
	{
		printf("Another timesig at %f\n", m_seq->getCurrentTime() * tickRate);
		timeSigNumeratorPat->putValue(m_seq->getCurrentTime() * tickRate, b0);
		timeSigDenominatorPat->putValue(m_seq->getCurrentTime() * tickRate, 1<<b1);
	}
	else
	{
	}
	pd.setValue(2);
}

void midiReader::tempoEvent(int tempo)
{
	AutomationPattern * tap = m_tc->tempoAutomationPattern();
	if( tap ) {
		tap->clear();
		tap->putValue(m_seq->getCurrentTime() * tickRate, 60000000/tempo);
	}
}

void midiReader::errorHandler(const QString &errorStr)
{
	printf( "MidiImport::readSMF(): got error %s at %d\n",
			errorStr.toStdString(), m_seq->getCurrentTime() );
}

void midiReader::ctlChangeEvent(int chan, int ctl, int value)
{
	CCHandler(chan, ctl, value);
}

void midiReader::pitchBendEvent(int chan, int value)
{
	CCHandler(chan, pitchBendEventId, value);
}

void midiReader::noteOnEvent(int chan, int pitch, int vol)
{
	if(vol != 0){
		int[4] note = {m_seq->getCurrentTime(), chan, pitch};
		note_list << note;
		return;
	}
	else
	{
		addNoteEvent(chan, pitch);
	}
}

void midiReader::noteOffEvent(int chan, int pitch, int vol)
{
	addNoteEvent(chan, pitch);
}

void midiReader::headerEvent(int format, int ntrks, int division)
{
	m_tracks = ntrks;
	m_division = division;

	tickRate = ticksPerBeat / m_division;
}

void midiReader::programEvent(int chan, int patch)
{
	QString trackName = QString( tr( "Track" ) + " %1").arg(chan);
	smfMidiChannel * ch = chs[chan].create( m_tc, trackName );

	if( ch->isSF2 )
	{
		// AFAIK, 128 should be the standard bank for drums in SF2.
		// If not, this has to be made configurable.
		ch->it_inst->childModel( "bank" )->setValue(chan!=9 ? 0 : 128);
		if(m_seq->getCurrentTime() == 0)
			ch->it_inst->childModel( "patch" )->setValue(patch);
		else
			CCHandler(chan, programEventId, patch);
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
			ch->it_inst->loadFile( dir+files.front() );
		}
	}
}
