/*
 * remote_plugin.h - base class providing RPC like mechanisms
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _REMOTE_PLUGIN_H
#define _REMOTE_PLUGIN_H

#include "lmmsconfig.h"
#include "mixer.h"
#include "midi.h"

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <vector>
#include <string>

#ifdef LMMS_HOST_X86_64
#define USE_NATIVE_SHM_API
#else
#if QT_VERSION >= 0x040400
#include <QtCore/QSharedMemory>
#else
#define USE_NATIVE_SHM_API
#endif
#endif

#ifdef USE_NATIVE_SHM_API
#ifdef LMMS_HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef LMMS_HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif
#endif


#ifdef BUILD_REMOTE_PLUGIN_CLIENT
#define COMPILE_REMOTE_PLUGIN_BASE
#endif


inline int32_t readInt( int _fd )
{
	int32_t i;
	if( read( _fd, &i, sizeof( i ) ) == sizeof( i ) )
	{
		return( i );
	}
	return( 0 );
}




inline bool writeInt( const int32_t & _i, int _fd )
{
	if( write( _fd, &_i, sizeof( _i ) ) != sizeof( _i ) )
	{
		return false;
	}
	return true;
}




static inline std::string readString( int _fd )
{
	const int len = readInt( _fd );
	if( len )
	{
		char * sc = new char[len + 1];
		if( read( _fd, sc, len ) == len )
		{
			sc[len] = 0;
			std::string s( sc );
			delete[] sc;
			return( s );
		}
		delete[] sc;
	}
	return std::string();
}




static inline bool writeString( const std::string & _s, int _fd )
{
	const int len = _s.size();
	writeInt( len, _fd );
	if( write( _fd, _s.c_str(), len ) != len )
	{
		return false;
	}
	return true;
}


enum RemoteMessageIDs
{
	IdUndefined,
	IdGeneralFailure,
	IdInitDone,
	IdClosePlugin,
	IdDebugMessage,
	IdSampleRateInformation,
	IdBufferSizeInformation,
	IdMidiEvent,
	IdStartProcessing,
	IdProcessingDone,
	IdChangeSharedMemoryKey,
	IdChangeInputCount,
	IdChangeOutputCount,
	IdShowUI,
	IdHideUI,
	IdSaveSettingsToString,
	IdSaveSettingsToFile,
	IdLoadSettingsFromString,
	IdLoadSettingsFromFile,
	IdLoadPresetFromFile,
	IdUserBase = 64
} ;



class remotePluginBase
{
public:
	struct message
	{
		message() :
			id( IdUndefined ),
			data()
		{
		}

		message( const message & _m ) :
			id( _m.id ),
			data( _m.data )
		{
		}

		message( int _id ) :
			id( _id ),
			data()
		{
		}

		void addInt( int _i )
		{
			char buf[128];
			buf[0] = 0;
			sprintf( buf, "%d", _i );
			data.push_back( std::string( buf ) );
		}

		int getInt( int _p ) const
		{
			return( atoi( data[_p].c_str() ) );
		}

		bool operator==( const message & _m )
		{
			return( id == _m.id );
		}

		int id;
		std::vector<std::string> data;
	} ;

	remotePluginBase( void );
	virtual ~remotePluginBase();

	void sendMessage( const message & _m );
	message receiveMessage( void );

	bool messagesLeft( void ) const;
	message waitForMessage( const message & _m,
						bool _busy_waiting = FALSE );

	inline message fetchAndProcessNextMessage( void )
	{
		message m = receiveMessage();
		processMessage( m );
		return m;
	}

	inline bool fetchAndProcessAllMessages( void )
	{
		while( messagesLeft() )
		{
			fetchAndProcessNextMessage();
		}
	}

	virtual bool processMessage( const message & _m ) = 0;


protected:
	void setInFD( int _fd )
	{
		m_inFD = _fd;
	}

	void setOutFD( int _fd )
	{
		m_outFD = _fd;
	}

private:
	int m_inFD;
	int m_outFD;

} ;



#ifndef BUILD_REMOTE_PLUGIN_CLIENT

class remotePlugin : public remotePluginBase
{
public:
	remotePlugin( const QString & _plugin_executable );
	virtual ~remotePlugin();

	virtual bool processMessage( const message & _m );

	bool process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf, bool _wait );
	bool waitForProcessingFinished( sampleFrame * _out_buf );

	void processMidiEvent( const midiEvent &, const f_cnt_t _offset );

	void updateSampleRate( sample_rate_t _sr )
	{
		message m( IdSampleRateInformation );
		m.addInt( _sr );
		sendMessage( m );
	}

	void showUI( void )
	{
		sendMessage( IdShowUI );
	}

	void hideUI( void )
	{
		sendMessage( IdHideUI );
	}


protected:
	inline void lock( void )
	{
		m_commMutex.lock();
	}

	inline void unlock( void )
	{
		m_commMutex.unlock();
	}


private:
	void resizeSharedMemory( void );


	bool m_initialized;
	bool m_failed;

	int m_pluginPID;
	int m_pipes[2][2];

	QMutex m_commMutex;
#ifdef USE_NATIVE_SHM_API
	int m_shmID;
#else
	QSharedMemory m_shmObj;
#endif
	size_t m_shmSize;
	float * m_shm;

	int m_inputCount;
	int m_outputCount;

} ;

#endif


#ifdef BUILD_REMOTE_PLUGIN_CLIENT

class remotePluginClient : public remotePluginBase
{
public:
	remotePluginClient( void );
	virtual ~remotePluginClient();

	virtual bool processMessage( const message & _m );

	virtual bool process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf ) = 0;

	virtual void processMidiEvent( const midiEvent &,
						const f_cnt_t /* _offset */ )
	{
	}

	inline float * sharedMemory( void )
	{
		return( m_shm );
	}

	virtual void updateSampleRate( sample_rate_t )
	{
	}

	virtual void updateBufferSize( fpp_t )
	{
	}

	inline fpp_t bufferSize( void ) const
	{
		return( m_bufferSize );
	}

	void debugMessage( const char * _fmt, ... )
	{
		va_list ap;
		char buffer[512];

		va_start( ap, _fmt );
		buffer[0] = 0;
		vsnprintf( buffer, sizeof( buffer ), _fmt, ap );
		message m( IdDebugMessage );
		m.data.push_back( std::string( buffer ) );
		sendMessage( m );
		va_end( ap );
	}

	void setInputCount( int _i )
	{
		m_inputCount = _i;
		message m( IdChangeInputCount );
		m.addInt( _i );
		sendMessage( m );
	}

	void setOutputCount( int _i )
	{
		m_outputCount = _i;
		message m( IdChangeOutputCount );
		m.addInt( _i );
		sendMessage( m );
	}


private:
	void setShmKey( int _key, int _size );
	void doProcessing( void );

#ifndef USE_NATIVE_SHM_API
	QSharedMemory m_shmObj;
#endif
	float * m_shm;

	int m_inputCount;
	int m_outputCount;

	fpp_t m_bufferSize;

} ;

#endif





#ifdef COMPILE_REMOTE_PLUGIN_BASE

#ifndef BUILD_REMOTE_PLUGIN_CLIENT
#include <QtCore/QCoreApplication>
#endif


remotePluginBase::remotePluginBase( void ) :
	m_inFD( -1 ),
	m_outFD( -1 )
{
}




remotePluginBase::~remotePluginBase()
{
}




void remotePluginBase::sendMessage( const message & _m )
{
	writeInt( _m.id, m_outFD );
	writeInt( _m.data.size(), m_outFD );
	for( int i = 0; i < _m.data.size(); ++i )
	{
		writeString( _m.data[i], m_outFD );
	}
}




remotePluginBase::message remotePluginBase::receiveMessage( void )
{
	message m;
	m.id = readInt( m_inFD );
	const int s = readInt( m_inFD );
	for( int i = 0; i < s; ++i )
	{
		m.data.push_back( readString( m_inFD ) );
	}
	return( m );
}




inline bool remotePluginBase::messagesLeft( void ) const
{
	fd_set rfds;
	FD_ZERO( &rfds );
	FD_SET( m_inFD, &rfds );
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1;	// can we use 0 here?
	return( select( m_inFD + 1, &rfds, NULL, NULL, &tv ) > 0 );
}




remotePluginBase::message remotePluginBase::waitForMessage(
							const message & _wm,
							bool _busy_waiting )
{
	while( 1 )
	{
		if( messagesLeft() )
		{
			message m;
			m = receiveMessage();
			processMessage( m );
			if( m.id == _wm.id )
			{
				return( m );
			}
			else if( m.id == IdGeneralFailure )
			{
				return( m );
			}
		}
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
		if( _busy_waiting )
		{
			QCoreApplication::processEvents(
						QEventLoop::AllEvents, 50 );
		}
		else
		{
			usleep( 10 );
		}
#else
		usleep( 10 );
#endif
	}
}


#endif





#ifdef BUILD_REMOTE_PLUGIN_CLIENT


remotePluginClient::remotePluginClient( void ) :
	remotePluginBase(),
#ifndef USE_NATIVE_SHM_API
	m_shmObj(),
#endif
	m_shm( NULL ),
	m_inputCount( DEFAULT_CHANNELS ),
	m_outputCount( DEFAULT_CHANNELS ),
	m_bufferSize( DEFAULT_BUFFER_SIZE )
{
	setInFD( 0 );
	setOutFD( 1 );
	sendMessage( IdSampleRateInformation );
	sendMessage( IdBufferSizeInformation );
}




remotePluginClient::~remotePluginClient()
{
	sendMessage( IdClosePlugin );

#ifdef USE_NATIVE_SHM_API
	shmdt( m_shm );
#endif
}




bool remotePluginClient::processMessage( const message & _m )
{
	message reply_message( _m.id );
	bool reply = false;
	switch( _m.id )
	{
		case IdGeneralFailure:
			return( false );

		case IdSampleRateInformation:
			updateSampleRate( _m.getInt( 0 ) );
			break;

		case IdBufferSizeInformation:
			m_bufferSize = _m.getInt( 0 );
			updateBufferSize( m_bufferSize );
			break;

		case IdClosePlugin:
			return( false );

		case IdMidiEvent:
			processMidiEvent(
				midiEvent( static_cast<MidiEventTypes>(
							_m.getInt( 0 ) ),
						_m.getInt( 1 ),
						_m.getInt( 2 ),
						_m.getInt( 3 ) ),
							_m.getInt( 4 ) );
			break;

		case IdStartProcessing:
			doProcessing();
			reply_message.id = IdProcessingDone;
			reply = true;
			break;

		case IdChangeSharedMemoryKey:
			setShmKey( _m.getInt( 0 ), _m.getInt( 1 ) );
			break;

		case IdUndefined:
		default:
			break;
	}
	if( reply )
	{
		sendMessage( reply_message );
	}

	return( true );
}




void remotePluginClient::setShmKey( int _key, int _size )
{
#ifdef USE_NATIVE_SHM_API
	if( m_shm != NULL )
	{
		shmdt( m_shm );
		m_shm = NULL;
	}

	// only called for detaching SHM?
	if( _key == 0 )
	{
		return;
	}

	int shm_id = shmget( _key, _size, 0 );
	if( shm_id == -1 )
	{
		debugMessage( "failed getting shared memory" );
	}
	else
	{
		m_shm = (float *) shmat( shm_id, 0, 0 );
	}
#else
	m_shmObj.setKey( QString::number( _key ) );
	if( m_shmObj.attach() )
	{
		m_shm = (float *) m_shmObj.data();
	}
	else
	{
		debugMessage( "failed getting shared memory" );
	}
#endif
}




void remotePluginClient::doProcessing( void )
{
	if( m_shm != NULL )
	{
		process( (sampleFrame *)( m_inputCount > 0 ? m_shm : NULL ),
				(sampleFrame *)( m_shm +
					( m_inputCount*m_bufferSize ) ) );
	}
}



#endif

#endif
