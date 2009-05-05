/*
 * midi_control_listener.cpp - implementation of the MIDI listener that
 *                             controls LMMS' transportation and other things
 *
 * Copyright (c) 2009 Achim Settelmeier <lmms/at/m1.sirlab.de>
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


#include "midi_control_listener.h"
#include "mixer.h"
#include "midi_client.h"
#include "midi_port.h"
#include "engine.h"
#include "note.h"
#include "song.h"


MidiControlListener::MidiControlListener() :
	m_port( "unnamed_midi_controller",
	       engine::getMixer()->getMidiClient(), this, NULL,
	       midiPort::Input ),
	m_controlKeyPressed( false )
{
	// default settings
	m_useControlKey = true;   // use control key
	m_controlKey = 60;        // C5
	m_controlChannel = -1;    // listen on all channels

#warning TODO replace hard-coded defaults
	// test config
	m_port.subscribeReadablePort( "24:0", true );
	m_actionMapKeys[57] = ActionPlay;
	m_actionMapKeys[59] = ActionStop;
	m_actionMapControllers[24] = ActionPlay;
	m_actionMapControllers[23] = ActionStop;
}




MidiControlListener::~MidiControlListener()
{
}




void MidiControlListener::processInEvent( const midiEvent & _me,
					 const midiTime & _time )
{
	// pre-check whether this MIDI packet suits our configuration
	switch( _me.m_type )
	{
		case MidiNoteOn:
		case MidiNoteOff:
		case MidiControlChange:
			// ignore commands for other channels
			if( m_controlChannel != -1 &&
				m_controlChannel != _me.channel() )
			{
				return;
			}
			break;
		default:
			// ignore commands other than note on/off and
			// control change
			return;
	}

	// check MIDI packet type and act upon
	switch( _me.m_type )
	{
		case MidiNoteOn:
		if( _me.key() == m_controlKey)
		{
			if( _me.velocity() == 0 )
			{
				// special case: key press with velocity 0
				// means key release
				m_controlKeyPressed = false;
				break;
			}
			m_controlKeyPressed = true;
			break;
		}
		else if( !m_useControlKey || m_controlKeyPressed )
		{
			if( _me.velocity() > 0 &&
				m_actionMapKeys.contains( _me.key() ) )
			{
				act( m_actionMapKeys.value( _me.key(),
								ActionNone ) );
			}
		}
		break;

	case MidiNoteOff:
		if( _me.key() == m_controlKey )
		{
			m_controlKeyPressed = false;
		}
		break;

	case MidiControlChange:
		// controller changed to a value other than zero
		if( _me.m_data.m_param[1] > 0 )
		{
			switch( m_actionMapControllers.value(
					_me.m_data.m_param[0], ActionNone ) )
			{
				case ActionNone:
					break;
				case ActionPlay:
					engine::getSong()->play();
					break;
				case ActionStop:
					engine::getSong()->stop();
					break;
			}
		}
		break;
	default:
		// nop
		break;
	}
}




void MidiControlListener::act( EventAction _action )
{
	switch( _action )
	{
		case ActionNone:
			break;
		case ActionPlay:
			engine::getSong()->play();
			break;
		case ActionStop:
			engine::getSong()->stop();
			break;
	}
}

