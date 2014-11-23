/*
 * MidiWinMM.cpp - WinMM MIDI client
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "MidiWinMM.h"
#include "config_mgr.h"
#include "engine.h"
#include "gui_templates.h"
#include "MidiPort.h"
#include "note.h"


#ifdef LMMS_BUILD_WIN32


MidiWinMM::MidiWinMM() :
	MidiClient(),
	m_inputDevices(),
	m_outputDevices(),
	m_inputSubs(),
	m_outputSubs()
{
	openDevices();
}




MidiWinMM::~MidiWinMM()
{
	closeDevices();
}




void MidiWinMM::processOutEvent( const MidiEvent& event, const MidiTime& time, const MidiPort* port )
{
	const DWORD shortMsg = ( event.type() + event.channel() ) +
				( ( event.param( 0 ) & 0xff ) << 8 ) +
				( ( event.param( 1 ) & 0xff ) << 16 );

	QStringList outDevs;
	for( SubMap::ConstIterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
	{
		for( MidiPortList::ConstIterator jt = it.value().begin(); jt != it.value().end(); ++jt )
		{
			if( *jt == port )
			{
				outDevs += it.key();
				break;
			}
		}
	}

	for( QMap<HMIDIOUT, QString>::Iterator it = m_outputDevices.begin(); it != m_outputDevices.end(); ++it )
	{
		if( outDevs.contains( *it ) )
		{
			midiOutShortMsg( it.key(), shortMsg );
		}
	}
}




void MidiWinMM::applyPortMode( MidiPort* port )
{
	// make sure no subscriptions exist which are not possible with
	// current port-mode
	if( !port->isInputEnabled() )
	{
		for( SubMap::Iterator it = m_inputSubs.begin(); it != m_inputSubs.end(); ++it )
		{
			it.value().removeAll( port );
		}
	}

	if( !port->isOutputEnabled() )
	{
		for( SubMap::Iterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
		{
			it.value().removeAll( port );
		}
	}
}




void MidiWinMM::removePort( MidiPort* port )
{
	for( SubMap::Iterator it = m_inputSubs.begin(); it != m_inputSubs.end(); ++it )
	{
		it.value().removeAll( port );
	}

	for( SubMap::Iterator it = m_outputSubs.begin(); it != m_outputSubs.end(); ++it )
	{
		it.value().removeAll( port );
	}

	MidiClient::removePort( port );
}




QString MidiWinMM::sourcePortName( const MidiEvent& event ) const
{
	if( event.sourcePort() )
	{
		return m_inputDevices.value( *static_cast<const HMIDIIN *>( event.sourcePort() ) );
	}

	return MidiClient::sourcePortName( event );
}




void MidiWinMM::subscribeReadablePort( MidiPort* port, const QString& dest, bool subscribe )
{
	if( subscribe && port->isInputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!\n", port->displayName().toAscii().constData() );
		return;
	}

	m_inputSubs[dest].removeAll( port );
	if( subscribe )
	{
		m_inputSubs[dest].push_back( port );
	}
}




void MidiWinMM::subscribeWritablePort( MidiPort* port, const QString& dest, bool subscribe )
{
	if( subscribe && port->isOutputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!\n", port->displayName().toAscii().constData() );
		return;
	}

	m_outputSubs[dest].removeAll( port );
	if( subscribe )
	{
		m_outputSubs[dest].push_back( port );
	}
}




void WINAPI CALLBACK MidiWinMM::inputCallback( HMIDIIN hm, UINT msg, DWORD_PTR inst, DWORD_PTR param1, DWORD_PTR param2 )
{
	if( msg == MIM_DATA )
	{
		( (MidiWinMM *) inst )->handleInputEvent( hm, param1 );
	}
}




void MidiWinMM::handleInputEvent( HMIDIIN hm, DWORD ev )
{
	const int cmd = ev & 0xff;
	if( cmd == MidiActiveSensing )
	{
		return;
	}
	const int par1 = ( ev >> 8 ) & 0xff;
	const int par2 = ev >> 16;
	const MidiEventTypes cmdtype = static_cast<MidiEventTypes>( cmd & 0xf0 );
	const int chan = cmd & 0x0f;

	const QString d = m_inputDevices.value( hm );
	if( d.isEmpty() || !m_inputSubs.contains( d ) )
	{
		return;
	}

	const MidiPortList & l = m_inputSubs[d];
	for( MidiPortList::ConstIterator it = l.begin(); it != l.end(); ++it )
	{
		switch( cmdtype )
		{
			case MidiNoteOn:
			case MidiNoteOff:
			case MidiKeyPressure:
				( *it )->processInEvent( MidiEvent( cmdtype, chan, par1 - KeysPerOctave, par2 & 0xff, &hm ) );
				break;

			case MidiControlChange:
			case MidiProgramChange:
			case MidiChannelPressure:
				( *it )->processInEvent( MidiEvent( cmdtype, chan, par1, par2 & 0xff, &hm ) );
				break;

			case MidiPitchBend:
				( *it )->processInEvent( MidiEvent( cmdtype, chan, par1 + par2*128, 0, &hm ) );
				break;

			default:
				qWarning( "MidiWinMM: unhandled input event %d\n", cmdtype );
				break;
		}
	}
}




void MidiWinMM::updateDeviceList()
{
	closeDevices();
	openDevices();

	emit readablePortsChanged();
	emit writablePortsChanged();
}



void MidiWinMM::closeDevices()
{
	m_inputSubs.clear();
	m_outputSubs.clear();

	QMapIterator<HMIDIIN, QString> i( m_inputDevices );
	while( i.hasNext() )
	{
		midiInClose( i.next().key() );
	}

	QMapIterator<HMIDIOUT, QString> o( m_outputDevices );
	while( o.hasNext() )
	{
		midiOutClose( o.next().key() );
	}

	m_inputDevices.clear();
	m_outputDevices.clear();
}




void MidiWinMM::openDevices()
{
	m_inputDevices.clear();
	for( unsigned int i = 0; i < midiInGetNumDevs(); ++i )
	{
		MIDIINCAPS c;
		midiInGetDevCaps( i, &c, sizeof( c ) );
		HMIDIIN hm = 0;
		MMRESULT res = midiInOpen( &hm, i, (DWORD_PTR) &inputCallback,
						(DWORD_PTR) this,
							CALLBACK_FUNCTION );
		if( res == MMSYSERR_NOERROR )
		{
			m_inputDevices[hm] = qstrdup( c.szPname );
			midiInStart( hm );
		}
	}

	m_outputDevices.clear();
	for( unsigned int i = 0; i < midiOutGetNumDevs(); ++i )
	{
		MIDIOUTCAPS c;
		midiOutGetDevCaps( i, &c, sizeof( c ) );
		HMIDIOUT hm = 0;
		MMRESULT res = midiOutOpen( &hm, i, 0, 0, CALLBACK_NULL );
		if( res == MMSYSERR_NOERROR )
		{
			m_outputDevices[hm] = qstrdup( c.szPname );
		}
	}
}




MidiWinMM::setupWidget::setupWidget( QWidget* parent ) :
	MidiClient::setupWidget( MidiWinMM::name(), parent )
{
}




MidiWinMM::setupWidget::~setupWidget()
{
}




#include "moc_MidiWinMM.cxx"


#endif

