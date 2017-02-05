/*
 * commonReader.h - import backend.
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

#ifndef COMMON_READER_H
#define COMMON_READER_H

#include <QObject>
#include <QString>
#include <QProgressDialog>

#include "TrackContainer.h"
#include "MeterModel.h"
#include "AutomationPattern.h"

#include "SmfMidiCC.h"
#include "SmfMidiChannel.h"

#define makeID(_c0, _c1, _c2, _c3) \
		( 0 | \
		( ( _c0 ) | ( ( _c1 ) << 8 ) | ( ( _c2 ) << 16 ) | ( ( _c3 ) << 24 ) ) )

const int defaultPitchRange = 2;
const int preTrackSteps = 2;

const int bankEventId = 0;
const int volumeEventId = 7;
const int panEventId = 10;
const int pitchBendEventId = 128;
const int programEventId = 129;

const int noTrack = -1;

class commonReader : public QObject
{
	Q_OBJECT
public:
	commonReader( TrackContainer *tc, const QString hintText );
	~commonReader();
protected:
	virtual void errorHandler( const QString &errorStr );
	void CCHandler( long tick, int track, int ctl, int value );
	void programHandler( long tick, int chan, int patch, int track = noTrack );
	void timeSigHandler( long tick, int num, int den );
	void tempoHandler( long tick, int tempo );
	void textHandler( int text_type, const QString& data , int track = -10 );
	void timeBaseHandler( int timebase );
	void trackStartHandler();

	void insertNoteEvent( long tick, int chan, int pitch, int vol, int track = noTrack );

	void addNoteEvent( long tick, int chan, int pitch, int vol, int track = noTrack );
	void addNoteEvent( long tick, int chan, int pitch, int vol, int dur, int track );


	TrackContainer *m_tc;

	// 128 CC + Pitch Bend + Program
	SmfMidiCC ccs[256][130];
	SmfMidiChannel chs[256];

	AutomationPattern * timeSigNumeratorPat;
	AutomationPattern * timeSigDenominatorPat;

	double beatsPerTact;
	double ticksPerBeat;
	double tickRate;  // convert ove tick to lmms tick.

	int pitchBendMultiply;
	int m_currentTrack = -2;
	QPair<int, QString> m_currentTrackName;

	QProgressDialog pd;

	/*
	 * record note event.
	 *  tick, channel, pitch, vol.
	 */
	QList<int *> note_list;

	/*
	 * record rpn event.
	 * 	chan, value
	 */
	QList<int*> rpn_msbs;
	QList<int*> rpn_lsbs;


};

#endif

