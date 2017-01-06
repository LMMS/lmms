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
#include <drumstick.h>

#include "TrackContainer.h"
#include "Engine.h"
#include "Song.h"
#include "AutomationPattern.h"
#include "MidiTime.h"

#include "smfMidiCC.h"
#include "smfMidiChannel.h"

#include "midiReader.h"

midiReader::midiReader( TrackContainer* tc ) :
	pd( QProgressDialog( TrackContainer::tr( "Importing MIDI-file..." ),
		TrackContainer::tr( "Cancel" ), 0, preTrackSteps, gui->mainWindow()) ),
	m_tc( tc )

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

	beatsPerTact = 4;
	ticksPerBeat = DefaultBeatsPerTact / beatsPerTact;


	// Connect to slots.
	connect(m_seq, SIGNAL(signalSMFTimeSig(int,int,int,int)),
			this, SLOT(timeSigEvent(int,int,int,int));

	connect(m_seq, SIGNAL(signalSMFTempo(int)),
			this, SLOT(tempoEvent(int));

	connect(m_seq, SIGNAL(signalSMFError(QString)),
			this, SLOT(errorHandler(QString));

	connect(m_seq, SIGNAL(signalSMFCtlChange(int,int,int)),
			this, SLOT(ctlChangeEvent(int,int,int));

	connect(m_seq, SIGNAL(signalSMFPitchBend(int,int)),
			this, SLOT(pitchBendEvent(int,int));

	connect(m_seq, SIGNAL(signalSMFNoteOn(int,int,int)),
			this, SLOT(noteOnEvent(int,int,int));

	connect(m_seq, SIGNAL(signalSMFNoteOff(int,int,int)),
			this, SLOT(noteOffEvent(int,int,int));

	connect(m_seq, SIGNAL(signalSMFProgram(int,int)),
			this, SLOT(programEvent(int,int));

	connect(m_seq, SIGNAL(signalSMFText(int,QString)),
			this, SLOT(textEvent(int,QString));

	connect(m_seq, SIGNAL(signalSMFTrackStart()),
			this, SLOT(trackStartEvent());

	connect(m_seq, SIGNAL(signalSMFTrackEnd()),
			this, SLOT(trackEndEvent());



}

midiReader::~midiReader()
{
	delete m_seq;
}

void midiReader::read(QString &fileName)
{
	m_seq->readFromFile(fileName);
	pd.setMaximum( m_seq->getTracks() + preTrackSteps );
	pd.setValue(1);
}

void midiReader::CCHandler(int chan, int ctl, int value){
	//TODO: from MidiImport.cpp
}

// Slots below.
void midiReader::timeSigEvent(int b0, int b1, int b2, int b3)
{
	if(/* m_seq->getCurrentTime() == 0*/ true ){
		printf("Another timesig at %f\n", m_seq->getCurrentTime()/ticksPerBeat/10);
		timeSigNumeratorPat->putValue(m_seq->getCurrentTime()/10, b0);
		timeSigDenominatorPat->putValue(0, 1<<b1)
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
	}
}

void midiReader::errorHandler(const QString &errorStr)
{
	// TODO: error handler.
}

void midiReader::ctlChangeEvent(int chan, int ctl, int value)
{
	CCHandler(chan, ctl, value);
}

void midiReader::pitchBendEvent(int chan, int value)
{
	CCHandler(chan, pitchBendEvent, value);
}

void midiReader::programEvent(int chan, int patch)
{
	// TODO：from MidiImport.cpp
}

void midiReader::noteOnEvent(int chan, int pitch, int vol)
{

}

void midiReader::noteOffEvent(int chan, int pitch, int vol)
{

}

void midiReader::textEvent(int typ, const QString &data)
{

}

void midiReader::trackStartEvent()
{

}

void midiReader::trackEndEvent()
{

}

void midiReader::headerEvent(int format, int ntrks, int division) {
	m_division = division;
	m_tracks = ntrks;
}
