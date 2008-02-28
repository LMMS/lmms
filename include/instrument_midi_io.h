/*
 * instrument_midi_io.h - class instrumentMidiIO
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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


#ifndef _INSTRUMENT_MIDI_IO_H
#define _INSTRUMENT_MIDI_IO_H

#include <QtCore/QList>

#include "automatable_model.h"


class instrumentTrack;
class midiPort;


class instrumentMidiIO : public model, public journallingObject
{
	Q_OBJECT
public:
	typedef QPair<QString, bool> descriptiveMidiPort;
	typedef QList<descriptiveMidiPort> midiPortMap;

	instrumentMidiIO( instrumentTrack * _instrument_track,
							midiPort * _port );
	virtual ~instrumentMidiIO();


	virtual void saveSettings( QDomDocument & _doc, QDomElement & _parent );
	virtual void loadSettings( const QDomElement & _this );
	virtual QString nodeName( void ) const
	{
		return( "midi" );
	}


public slots:
	void midiPortModeChanged( void );


protected slots:
	void inputChannelChanged( void );
	void outputChannelChanged( void );
	void defaultVelInChanged( void );
	void defaultVelOutChanged( void );
	void readablePortsChanged( void );
	void writeablePortsChanged( void );

	void activatedReadablePort( const descriptiveMidiPort & _port );
	void activatedWriteablePort( const descriptiveMidiPort & _port );


private:
	instrumentTrack * m_instrumentTrack;
	midiPort * m_midiPort;

	intModel m_inputChannelModel;
	intModel m_outputChannelModel;
	boolModel m_receiveEnabledModel;
	boolModel m_sendEnabledModel;
	boolModel m_defaultVelocityInEnabledModel;
	boolModel m_defaultVelocityOutEnabledModel;

	midiPortMap m_readablePorts;
	midiPortMap m_writeablePorts;


	friend class instrumentMidiIOView;

} ;


#endif
