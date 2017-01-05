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

#include <drumstick.h>

#include "commonReader.h"
class midiReader : public commonReader
{
	Q_OBJECT
public:
	midiReader(TrackContainer *tc);
	~midiReader();
	void read(QString &fileName);
public slots:
	void headerEvent(int format, int ntrks, int division);
	void noteOnEvent(int chan, int pitch, int vol);
	void noteOffEvent(int chan, int pitch, int vol);
	void ctlChangeEvent(int chan, int ctl, int value);
	void pitchBendEvent(int chan, int value);
	void programEvent(int chan, int patch);
	void timeSigEvent(int b0, int b1, int b2, int b3);
	void tempoEvent(int tempo);
	void errorHandler(const QString& errorStr);
private:
	drumstick::QSmf *m_seq;
};

#endif
