/*
 * MidiWinMM.cpp - WinMM MIDI client
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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




void MidiWinMM::processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const MidiPort * _port )
{
	const DWORD short_msg = ( _me.m_type + _me.channel() ) +
				( ( _me.m_data.m_param[0] & 0xff ) << 8 ) +
				( ( _me.m_data.m_param[1] & 0xff ) << 16 );

	QStringList out_devs;
	for( SubMap::ConstIterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
	{
		for( MidiPortList::ConstIterator jt = it.value().begin();
						jt != it.value().end(); ++jt )
		{
			if( *jt == _port )
			{
				out_devs += it.key();
				break;
			}
		}
	}

	for( QMap<HMIDIOUT, QString>::Iterator it = m_outputDevices.begin();
					it != m_outputDevices.end(); ++it )
	{
		if( out_devs.contains( *it ) )
		{
			midiOutShortMsg( it.key(), short_msg );
		}
	}
}




void MidiWinMM::applyPortMode( MidiPort * _port )
{
	// make sure no subscriptions exist which are not possible with
	// current port-mode
	if( !_port->inputEnabled() )
	{
		for( SubMap::Iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}

	if( !_port->outputEnabled() )
	{
		for( SubMap::Iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}
}




void MidiWinMM::removePort( MidiPort * _port )
{
	for( SubMap::Iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	for( SubMap::Iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	MidiClient::removePort( _port );
}




void MidiWinMM::subscribeReadablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _subscribe && _port->inputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!\n",
				_port->displayName().toAscii().constData() );
		return;
	}

	m_inputSubs[_dest].removeAll( _port );
	if( _subscribe )
	{
		m_inputSubs[_dest].push_back( _port );
	}
}




void MidiWinMM::subscribeWritablePort( MidiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _subscribe && _port->outputEnabled() == false )
	{
		qWarning( "port %s can't be (un)subscribed!\n",
				_port->displayName().toAscii().constData() );
		return;
	}

	m_outputSubs[_dest].removeAll( _port );
	if( _subscribe )
	{
		m_outputSubs[_dest].push_back( _port );
	}
}




void WINAPI CALLBACK MidiWinMM::inputCallback( HMIDIIN _hm, UINT _msg, DWORD_PTR _inst,
					DWORD_PTR _param1, DWORD_PTR _param2 )
{
	if( _msg == MIM_DATA )
	{
		( (MidiWinMM *) _inst )->handleInputEvent( _hm, _param1 );
	}
}




void MidiWinMM::handleInputEvent( HMIDIIN _hm, DWORD _ev )
{
	const int cmd = _ev & 0xff;
	if( cmd == MidiActiveSensing )
	{
		return;
	}
	const int par1 = ( _ev >> 8 ) & 0xff;
	const int par2 = _ev >> 16;
	const MidiEventTypes cmdtype =
				static_cast<MidiEventTypes>( cmd & 0xf0 );
	const int chan = cmd & 0x0f;

	const QString d = m_inputDevices.value( _hm );
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
				( *it )->processInEvent(
					midiEvent( cmdtype, chan,
							par1 - KeysPerOctave,
							par2 & 0xff ),
								midiTime() );
				break;

			case MidiControlChange:
			case MidiProgramChange:
			case MidiChannelPressure:
				( *it )->processInEvent(
					midiEvent( cmdtype, chan, par1,
							par2 & 0xff ),
								midiTime() );
				break;

			case MidiPitchBend:
				( *it )->processInEvent(
					midiEvent( cmdtype, chan,
							par1 + par2*128, 0 ),
								midiTime() );
				break;

			default:
				qWarning( "WinMM-MIDI: unhandled input "
							"event %d\n", cmdtype );
				break;
		}
	}
}




void MidiWinMM::updateDeviceList()
{
	closeDevices();
	openDevices();
//	if( m_readablePorts != readable_ports )
	{
//		m_readablePorts = readable_ports;
		emit readablePortsChanged();
	}

//	if( m_writablePorts != writable_ports )
	{
//		m_writablePorts = writable_ports;
		emit writablePortsChanged();
	}
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
		MMRESULT res = midiOutOpen( &hm, i, NULL, NULL, CALLBACK_NULL );
		if( res == MMSYSERR_NOERROR )
		{
			m_outputDevices[hm] = qstrdup( c.szPname );
		}
	}
}




MidiWinMM::setupWidget::setupWidget( QWidget * _parent ) :
	MidiClient::setupWidget( MidiWinMM::name(), _parent )
{
}




MidiWinMM::setupWidget::~setupWidget()
{
}




#include "moc_MidiWinMM.cxx"


#endif

