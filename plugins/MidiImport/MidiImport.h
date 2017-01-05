/*
 * MidiImport.h - support for importing MIDI-files
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

#ifndef _MIDI_IMPORT_H
#define _MIDI_IMPORT_H

#include <QString>
#include <QPair>
#include <QVector>
#include <drumstick.h>

#include "MidiEvent.h"
#include "ImportFilter.h"

const double defaultPitchRange = 2.0f;

class MidiImport : public ImportFilter
{
	Q_OBJECT
public:
	MidiImport( const QString & _file );
	virtual ~MidiImport();

	virtual PluginView * instantiateView( QWidget * )
	{
		return( NULL );
	}

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
	virtual bool tryImport( TrackContainer* tc );

	bool readSMF( TrackContainer* tc );
	bool readRIFF( TrackContainer* tc );
	bool readTrack( int _track_end, QString & _track_name );

	void error( void );


	inline int readInt( int _bytes )
	{
		int c, value = 0;
		do
		{
			c = readByte();
			if( c == -1 )
			{
				return( -1 );
			}
			value = ( value << 8 ) | c;
		} while( --_bytes );
		return( value );
	}
	inline int read32LE()
	{
		int value = readByte();
		value |= readByte() << 8;
		value |= readByte() << 16;
		value |= readByte() << 24;
		return value;
	}
	inline int readVar()
	{
		int c = readByte();
		int value = c & 0x7f;
		if( c & 0x80 )
		{
			c = readByte();
			value = ( value << 7 ) | ( c & 0x7f );
			if( c & 0x80 )
			{
				c = readByte();
				value = ( value << 7 ) | ( c & 0x7f );
				if( c & 0x80 )
				{
					c = readByte();
					value = ( value << 7 ) | c;
					if( c & 0x80 )
					{
						return -1;
					}
				}
			}
	        }
        	return( !file().atEnd() ? value : -1 );
	}

	inline int readID()
	{
		return read32LE();
	}
	inline void skip( int _bytes )
	{
		while( _bytes > 0 )
		{
			readByte();
			--_bytes;
		}
	}


	typedef QVector<QPair<int, MidiEvent> > EventVector;
	EventVector m_events;
	int m_timingDivision;

	drumstick::QSmf *m_seq;
	TrackContainer *m_tc;
} ;


#endif
