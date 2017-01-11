/*
 * oveReader.h - support for importing Overture files
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

#ifndef OVE_READER_H
#define OVE_READER_H

#include "commonReader.h"

class oveReader : public commonReader
{
	Q_OBJECT
public:
	oveReader( TrackContainer *tc );
	~oveReader();
	void read( QString &fileName );
public slots:
	void errorHandler( const QString& errorStr );
	void fileHeader( int quarter, int trackCount );
	void noteOnEvent( int track, long tick, int chan, int pitch, int vol );
	void noteOffEvent( int track, long tick, int chan, int pitch, int vol );
	void ctlChangeEvent( int track, long tick, int chan, int ctl, int value );
	void pitchBendEvent( int track, long tick, int chan, int value );
	void programEvent( int track, long tick, int chan, int patch );
	void timeSigEvent( int bar, long tick, int num, int den );
	void tempoEvent( long tick, int tempo );
	void trackPatch( int track, int chan, int patch );
	void trackVol( int track, int chan, int vol );
	void trackBank( int track, int chan, int bank );
	void trackStart( const QString& name, int track, int channel, int pitch, int velocity,
			 int port, bool selected, bool muted, bool loop );

private:
	drumstick::QOve *m_seq;
};

#endif
