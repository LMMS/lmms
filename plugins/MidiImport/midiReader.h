/*
 * midiReader.h - support for importing MIDI files
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

#ifndef MIDI_READER_H
#define MIDI_READER_H

#include <QString>
#include <QProgressDialog>
#include <drumstick.h>

#include "TrackContainer.h"
#include "MeterModel.h"
#include "AutomationPattern.h"

#include "smfMidiCC.h"
#include "smfMidiChannel.h"


const double defaultPitchRange = 2.0f;
const int preTrackSteps = 2;
const int pitchBendEvent = 128;

class midiReader : public QObject
{
	Q_OBJECT
public:
	midiReader(TrackContainer *tc);
	~midiReader();
	void read(QString &fileName );
public slots:
	void headerEvent(int format, int ntrks, int division);
	void trackStartEvent();
	void trackEndEvent();
	void endOfTrackEvent();
	void noteOnEvent(int chan, int pitch, int vol);
	void noteOffEvent(int chan, int pitch, int vol);
	void keyPressEvent(int chan, int pitch, int press);
	void ctlChangeEvent(int chan, int ctl, int value);
	void pitchBendEvent(int chan, int value);
	void programEvent(int chan, int patch);
	void chanPressEvent(int chan, int press);
	void sysexEvent(const QByteArray& data);
	void seqSpecificEvent(const QByteArray& data);
	void metaMiscEvent(int typ, const QByteArray& data);
	void seqNum(int seq);
	void forcedChannel(int channel);
	void forcedPort(int port);
	void textEvent(int typ, const QString& data);
	void smpteEvent(int b0, int b1, int b2, int b3, int b4);
	void timeSigEvent(int b0, int b1, int b2, int b3);
	void keySigEvent(int b0, int b1);
	void tempoEvent(int tempo);
	void errorHandler(const QString& errorStr);

private:
	void CCHandler(int chan, int ctl, int value);

	drumstick::QSmf *m_seq;
	TrackContainer *m_tc;
	QProgressDialog pd;

	// 128 CC + Pitch Bend
	smfMidiCC ccs[129];
	smfMidiChannel chs[256];

	MeterModel & timeSigMM;
	AutomatableModel * timeSigNumeratorPat;
	AutomatableModel * timeSigDenominatorPat;

	double beatsPerTact;
	double ticksPerBeat;

	int m_tracks;
	int m_division;
};

#endif
