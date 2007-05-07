/*
 * midi_port.h - abstraction of MIDI-ports which are part of LMMS's MIDI-
 *               sequencing system
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIDI_PORT_H
#define _MIDI_PORT_H

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QString>

#else

#include <qstring.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "types.h"
#include "midi.h"


class midiClient;
class midiEventProcessor;
class midiTime;


// class for abstraction of MIDI-port
class midiPort
{
public:
	enum modes
	{
		DUMMY,		// don't route any MIDI-events (default)
		INPUT,		// from MIDI-client to MIDI-event-processor
		OUTPUT,		// from MIDI-event-processor to MIDI-client
		DUPLEX		// both directions
	} ;

	midiPort( midiClient * _mc, midiEventProcessor * _mep,
				const QString & _name, modes _mode = DUMMY );
	~midiPort();

	inline const QString & name( void ) const
	{
		return( m_name );
	}

	void FASTCALL setName( const QString & _name );

	inline modes mode( void ) const
	{
		return( m_mode );
	}

	void FASTCALL setMode( modes _mode );

	inline Sint8 inputChannel( void ) const
	{
		return( m_inputChannel );
	}

	inline void setInputChannel( Sint8 _chnl )
	{
		m_inputChannel = _chnl;
	}

	inline void enableDefaultVelocityForInEvents( const bool _on )
	{
		m_defaultVelocityForInEventsEnabled = _on;
	}

	inline Sint8 outputChannel( void ) const
	{
		return( m_outputChannel );
	}

	inline void setOutputChannel( Sint8 _chnl )
	{
		m_outputChannel = _chnl;
	}

	inline void enableDefaultVelocityForOutEvents( const bool _on )
	{
		m_defaultVelocityForOutEventsEnabled = _on;
	}



	void FASTCALL processInEvent( const midiEvent & _me,
						const midiTime & _time );
	void FASTCALL processOutEvent( const midiEvent & _me,
						const midiTime & _time );


private:
	midiClient * m_midiClient;
	midiEventProcessor * m_midiEventProcessor;
	QString m_name;
	modes m_mode;
	Sint8 m_inputChannel;
	Sint8 m_outputChannel;
	bool m_defaultVelocityForInEventsEnabled;
	bool m_defaultVelocityForOutEventsEnabled;

} ;



#endif
