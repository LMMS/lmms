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

#include "midiReader.h"

midiReader::midiReader( TrackContainer* tc ) :
	commonReader(tc, "Importing MIDI-file..." ),
	m_seq(new drumstick::QSmf)
{
	// Connect to slots.
	connect(m_seq, SIGNAL(signalSMFTimeSig(int,int,int,int)),
			this, SLOT(timeSigEvent(int,int,int,int)));

	connect(m_seq, SIGNAL(signalSMFTempo(int)),
			this, SLOT(tempoEvent(int)));

	connect(m_seq, SIGNAL(signalSMFError(QString)),
			this, SLOT(errorHandler(QString)));

	connect(m_seq, SIGNAL(signalSMFCtlChange(int,int,int)),
			this, SLOT(ctlChangeEvent(int,int,int)));

	connect(m_seq, SIGNAL(signalSMFPitchBend(int,int)),
			this, SLOT(pitchBendEvent(int,int)));

	connect(m_seq, SIGNAL(signalSMFNoteOn(int,int,int)),
			this, SLOT(noteOnEvent(int,int,int)));

	connect(m_seq, SIGNAL(signalSMFNoteOff(int,int,int)),
			this, SLOT(noteOffEvent(int,int,int)));

	connect(m_seq, SIGNAL(signalSMFProgram(int,int)),
			this, SLOT(programEvent(int,int)));

	connect(m_seq, SIGNAL(signalSMFHeader(int,int,int)),
			this, SLOT(headerEvent(int,int,int)));

}

midiReader::~midiReader()
{
	printf("destroy midiReader\n");
	delete m_seq;
}

void midiReader::read(QString &fileName)
{
	m_seq->readFromFile(fileName);
}

// Slots below.

// b0: Numerator
// b1: Denominator (exponent in a power of two)
// b2: Number of MIDI clocks per metronome click
// b3: Number of notated 32nd notes per 24 MIDI clocks
void midiReader::timeSigEvent(int b0, int b1, int b2, int b3)
{
	timeSigHandler(m_seq->getCurrentTime(), b0, 1<<b1);
}

void midiReader::tempoEvent(int tempo)
{
	tempoHandler(m_seq->getCurrentTime(), 60000000/tempo);
}

void midiReader::errorHandler(const QString &errorStr)
{
	printf( "MidiImport::readSMF(): got error %s at %ld\n",
			errorStr.toStdString().c_str(), m_seq->getCurrentTime() );
}

void midiReader::ctlChangeEvent(int chan, int ctl, int value)
{
	CCHandler(m_seq->getCurrentTime(), chan, ctl, value);
}

void midiReader::pitchBendEvent(int chan, int value)
{
	CCHandler(m_seq->getCurrentTime(), chan, pitchBendEventId, value);
}

void midiReader::noteOnEvent(int chan, int pitch, int vol)
{
	if(vol != 0){
		insertNoteEvent(m_seq->getCurrentTime(), chan, pitch, vol);
	}
	else
	{
		addNoteEvent(m_seq->getCurrentTime(), chan, pitch);
	}
}

void midiReader::noteOffEvent(int chan, int pitch, int vol)
{
	addNoteEvent(m_seq->getCurrentTime(), chan, pitch);
}

void midiReader::headerEvent(int format, int ntrks, int division)
{
	timeBaseHandler(division);
	pd.setMaximum( ntrks + preTrackSteps );
}

void midiReader::programEvent(int chan, int patch)
{
	programHandler(m_seq->getCurrentTime(), chan, patch);
}
