/*
 * midi_winmm.cpp - WinMM MIDI client
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


#include "midi_winmm.h"
#include "config_mgr.h"
#include "engine.h"
#include "gui_templates.h"
#include "midi_port.h"
#include "note.h"


#ifdef LMMS_BUILD_WIN32


midiWinMM::midiWinMM( void ) :
	midiClient(),
	m_inputDevices(),
	m_outputDevices(),
	m_inputSubs(),
	m_outputSubs()
{
	openDevices();
}




midiWinMM::~midiWinMM()
{
	closeDevices();
}




void midiWinMM::processOutEvent( const midiEvent & _me,
						const midiTime & _time,
						const midiPort * _port )
{
	const DWORD short_msg = ( _me.m_type + _me.channel() ) +
				( ( _me.m_data.m_param[0] & 0xff ) << 8 ) +
				( ( _me.m_data.m_param[1] & 0xff ) << 16 );

	QStringList out_devs;
	for( subMap::iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
	{
		for( midiPortList::iterator jt = it.value().begin();
						jt != it.value().end(); ++jt )
		{
			if( *jt == _port )
			{
				out_devs += it.key();
				break;
			}
		}
	}

	for( QMap<HMIDIOUT, QString>::iterator it = m_outputDevices.begin();
					it != m_outputDevices.end(); ++it )
	{
		if( out_devs.contains( *it ) )
		{
			midiOutShortMsg( it.key(), short_msg );
		}
	}
}




void midiWinMM::applyPortMode( midiPort * _port )
{
	// make sure no subscriptions exist which are not possible with
	// current port-mode
	if( !_port->inputEnabled() )
	{
		for( subMap::iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}

	if( !_port->outputEnabled() )
	{
		for( subMap::iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
		{
			it.value().removeAll( _port );
		}
	}
}




void midiWinMM::removePort( midiPort * _port )
{
	for( subMap::iterator it = m_inputSubs.begin();
						it != m_inputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	for( subMap::iterator it = m_outputSubs.begin();
						it != m_outputSubs.end(); ++it )
	{
		it.value().removeAll( _port );
	}
	midiClient::removePort( _port );
}




void midiWinMM::subscribeReadablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _subscribe && _port->inputEnabled() == FALSE )
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




void midiWinMM::subscribeWritablePort( midiPort * _port,
						const QString & _dest,
						bool _subscribe )
{
	if( _subscribe && _port->outputEnabled() == FALSE )
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




void WINAPI CALLBACK midiWinMM::inputCallback( HMIDIIN _hm, UINT _msg, DWORD_PTR _inst,
					DWORD_PTR _param1, DWORD_PTR _param2 )
{
	if( _msg == MIM_DATA )
	{
		( (midiWinMM *) _inst )->handleInputEvent( _hm, _param1 );
	}
}




void midiWinMM::handleInputEvent( HMIDIIN _hm, DWORD _ev )
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

	const midiPortList & l = m_inputSubs[d];
	for( midiPortList::const_iterator it = l.begin(); it != l.end(); ++it )
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




void midiWinMM::updateDeviceList( void )
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



void midiWinMM::closeDevices( void )
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




void midiWinMM::openDevices( void )
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




midiWinMM::setupWidget::setupWidget( QWidget * _parent ) :
	midiClient::setupWidget( midiWinMM::name(), _parent )
{
}




midiWinMM::setupWidget::~setupWidget()
{
}




#include "moc_midi_winmm.cxx"


#endif

