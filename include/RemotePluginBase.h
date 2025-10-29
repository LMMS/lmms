/*
 * RemotePluginBase.h - base class providing RPC like mechanisms
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef LMMS_REMOTE_PLUGIN_BASE_H
#define LMMS_REMOTE_PLUGIN_BASE_H

#include <atomic>  // IWYU pragma: keep
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lmmsconfig.h"

#if !(defined(LMMS_HAVE_SYS_IPC_H) && defined(LMMS_HAVE_SEMAPHORE_H))
#define SYNC_WITH_SHM_FIFO

#ifdef LMMS_HAVE_PROCESS_H
#include <process.h>
#endif
#else // !(LMMS_HAVE_SYS_IPC_H && LMMS_HAVE_SEMAPHORE_H)
#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif // !(LMMS_HAVE_SYS_IPC_H && LMMS_HAVE_SEMAPHORE_H)

#ifdef LMMS_HAVE_LOCALE_H
#include <clocale>  // IWYU pragma: keep
#endif

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif


#ifdef BUILD_REMOTE_PLUGIN_CLIENT
#undef LMMS_EXPORT
#define LMMS_EXPORT

#ifndef SYNC_WITH_SHM_FIFO
#include <sys/socket.h>
#include <sys/un.h>
#endif // SYNC_WITH_SHM_FIFO

#else // BUILD_REMOTE_PLUGIN_CLIENT
#include "lmms_export.h"
#include <QString>

#ifndef SYNC_WITH_SHM_FIFO
#include <poll.h>
#include <unistd.h>  // IWYU pragma: keep
#endif // SYNC_WITH_SHM_FIFO

#endif // BUILD_REMOTE_PLUGIN_CLIENT

#ifdef SYNC_WITH_SHM_FIFO
#include "SharedMemory.h"
#include "SystemSemaphore.h"
#endif

namespace lmms
{


#ifdef SYNC_WITH_SHM_FIFO


// sometimes we need to exchange bigger messages (e.g. for VST parameter dumps)
// so set a usable value here
const int SHM_FIFO_SIZE = 512*1024;


// implements a FIFO inside a shared memory segment
class shmFifo
{
	// need this union to handle different sizes of sem_t on 32 bit
	// and 64 bit platforms
	union sem32_t
	{
		int semKey;
		char fill[32];
	} ;
	struct shmData
	{
		sem32_t dataSem;	// semaphore for locking this
					// FIFO management data
		sem32_t messageSem;	// semaphore for incoming messages
		int32_t startPtr; // current start of FIFO in memory
		int32_t endPtr;   // current end of FIFO in memory
		char data[SHM_FIFO_SIZE];  // actual data
	} ;

public:
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
	// constructor for master-side
	shmFifo() :
		m_invalid( false ),
		m_master( true ),
		m_lockDepth( 0 )
	{
		m_data.create();
		m_data->startPtr = m_data->endPtr = 0;
		static int k = 0;
		m_data->dataSem.semKey = ( getpid()<<10 ) + ++k;
		m_data->messageSem.semKey = ( getpid()<<10 ) + ++k;
		m_dataSem = SystemSemaphore{std::to_string(m_data->dataSem.semKey), 1u};
		m_messageSem = SystemSemaphore{std::to_string(m_data->messageSem.semKey), 0u};
	}
#endif

	// constructor for remote-/client-side - use _shm_key for making up
	// the connection to master
	shmFifo(const std::string& shmKey) :
		m_invalid( false ),
		m_master( false ),
		m_lockDepth( 0 )
	{
		m_data.attach(shmKey);
		m_dataSem = SystemSemaphore{std::to_string(m_data->dataSem.semKey)};
		m_messageSem = SystemSemaphore{std::to_string(m_data->messageSem.semKey)};
	}

	inline bool isInvalid() const
	{
		return m_invalid;
	}

	void invalidate()
	{
		m_invalid = true;
	}

	// do we act as master (i.e. not as remote-process?)
	inline bool isMaster() const
	{
		return m_master;
	}

	// recursive lock
	inline void lock()
	{
		if( !isInvalid() && m_lockDepth.fetch_add( 1 ) == 0 )
		{
			m_dataSem.acquire();
		}
	}

	// recursive unlock
	inline void unlock()
	{
		if( m_lockDepth.fetch_sub( 1 ) <= 1 )
		{
			m_dataSem.release();
		}
	}

	// wait until message-semaphore is available
	inline void waitForMessage()
	{
		if( !isInvalid() )
		{
			m_messageSem.acquire();
		}
	}

	// increase message-semaphore
	inline void messageSent()
	{
		m_messageSem.release();
	}


	inline int32_t readInt()
	{
		int32_t i;
		read( &i, sizeof( i ) );
		return i;
	}

	inline void writeInt( const int32_t & _i )
	{
		write( &_i, sizeof( _i ) );
	}

	inline std::string readString()
	{
		const int len = readInt();
		if( len )
		{
			char * sc = new char[len + 1];
			read( sc, len );
			sc[len] = 0;
			std::string s( sc );
			delete[] sc;
			return s;
		}
		return std::string();
	}


	inline void writeString( const std::string & _s )
	{
		const int len = _s.size();
		writeInt( len );
		write( _s.c_str(), len );
	}


	inline bool messagesLeft()
	{
		if( isInvalid() )
		{
			return false;
		}
		lock();
		const bool empty = ( m_data->startPtr == m_data->endPtr );
		unlock();
		return !empty;
	}


	const std::string& shmKey() const
	{
		return m_data.key();
	}


private:
	static inline void fastMemCpy( void * _dest, const void * _src,
							const int _len )
	{
		// calling memcpy() for just an integer is obsolete overhead
		if( _len == 4 )
		{
			*( (int32_t *) _dest ) = *( (int32_t *) _src );
		}
		else
		{
			memcpy( _dest, _src, _len );
		}
	}

	void read( void * _buf, int _len )
	{
		if( isInvalid() )
		{
			memset( _buf, 0, _len );
			return;
		}
		lock();
		while( isInvalid() == false &&
				_len > m_data->endPtr - m_data->startPtr )
		{
			unlock();
#ifndef LMMS_BUILD_WIN32
			usleep( 5 );
#endif
			lock();
		}
		fastMemCpy( _buf, m_data->data + m_data->startPtr, _len );
		m_data->startPtr += _len;
		// nothing left?
		if( m_data->startPtr == m_data->endPtr )
		{
			// then reset to 0
			m_data->startPtr = m_data->endPtr = 0;
		}
		unlock();
	}

	void write( const void * _buf, int _len )
	{
		if( isInvalid() || _len > SHM_FIFO_SIZE )
		{
			return;
		}
		lock();
		while( _len > SHM_FIFO_SIZE - m_data->endPtr )
		{
			// if no space is left, try to move data to front
			if( m_data->startPtr > 0 )
			{
				memmove( m_data->data,
					m_data->data + m_data->startPtr,
					m_data->endPtr - m_data->startPtr );
				m_data->endPtr = m_data->endPtr -
							m_data->startPtr;
				m_data->startPtr = 0;
			}
			unlock();
#ifndef LMMS_BUILD_WIN32
			usleep( 5 );
#endif
			lock();
		}
		fastMemCpy( m_data->data + m_data->endPtr, _buf, _len );
		m_data->endPtr += _len;
		unlock();
	}

	volatile bool m_invalid;
	bool m_master;
	SharedMemory<shmData> m_data;
	SystemSemaphore m_dataSem;
	SystemSemaphore m_messageSem;
	std::atomic_int m_lockDepth;
};
#endif // SYNC_WITH_SHM_FIFO



enum RemoteMessageIDs
{
	IdUndefined,
	IdHostInfoGotten,
	IdInitDone,
	IdQuit,
	IdSyncKey,
	IdSampleRateInformation,
	IdBufferSizeInformation,
	IdInformationUpdated,
	IdMidiEvent,
	IdStartProcessing,
	IdProcessingDone,
	IdChangeSharedMemoryKey,
	IdChangeInputCount,
	IdChangeOutputCount,
	IdChangeInputOutputCount,
	IdShowUI,
	IdHideUI,
	IdToggleUI,
	IdIsUIVisible,
	IdSaveSettingsToString,
	IdSaveSettingsToFile,
	IdLoadSettingsFromString,
	IdLoadSettingsFromFile,
	IdSavePresetFile,
	IdLoadPresetFile,
	IdDebugMessage,
	IdIdle,
	IdUserBase = 64
} ;



class LMMS_EXPORT RemotePluginBase
{
public:
	struct message
	{
		message() :
			id( IdUndefined ),
			data()
		{
		}

		message( const message & _m ) = default;

		message( int _id ) :
			id( _id ),
			data()
		{
		}

		inline message & addString( const std::string & _s )
		{
			data.push_back( _s );
			return *this;
		}

		message & addInt( int _i )
		{
			char buf[32];
			std::snprintf(buf, 32, "%d", _i);
			data.emplace_back( buf );
			return *this;
		}

		message & addFloat( float _f )
		{
			char buf[32];
			std::snprintf(buf, 32, "%f", _f);
			data.emplace_back( buf );
			return *this;
		}

		inline std::string getString( int _p = 0 ) const
		{
			return data[_p];
		}

#ifndef BUILD_REMOTE_PLUGIN_CLIENT
		inline QString getQString( int _p = 0 ) const
		{
			return QString::fromStdString( getString( _p ) );
		}
#endif

		inline int getInt( int _p = 0 ) const
		{
			return atoi( data[_p].c_str() );
		}

		inline float getFloat( int _p ) const
		{
			return (float) atof( data[_p].c_str() );
		}

		inline bool operator==( const message & _m ) const
		{
			return( id == _m.id );
		}

		int id;

	private:
		std::vector<std::string> data;

		friend class RemotePluginBase;

	} ;

#ifdef SYNC_WITH_SHM_FIFO
	RemotePluginBase( shmFifo * _in, shmFifo * _out );
#else
	RemotePluginBase();
#endif
	virtual ~RemotePluginBase();

#ifdef SYNC_WITH_SHM_FIFO
	void reset( shmFifo *in, shmFifo *out )
	{
		delete m_in;
		delete m_out;
		m_in = in;
		m_out = out;
	}
#endif

	int sendMessage( const message & _m );
	message receiveMessage();

	inline bool isInvalid() const
	{
#ifdef SYNC_WITH_SHM_FIFO
		return m_in->isInvalid() || m_out->isInvalid();
#else
		return m_invalid;
#endif
	}

	message waitForMessage( const message & _m,
						bool _busy_waiting = false );

	inline message fetchAndProcessNextMessage()
	{
		message m = receiveMessage();
		processMessage( m );
		return m;
	}

#ifndef SYNC_WITH_SHM_FIFO
	inline int32_t readInt()
	{
		int32_t i;
		read( &i, sizeof( i ) );
		return i;
	}

	inline void writeInt( const int32_t & _i )
	{
		write( &_i, sizeof( _i ) );
	}

	inline std::string readString()
	{
		const int len = readInt();
		if( len )
		{
			char * sc = new char[len + 1];
			read( sc, len );
			sc[len] = 0;
			std::string s( sc );
			delete[] sc;
			return s;
		}
		return std::string();
	}


	inline void writeString( const std::string & _s )
	{
		const int len = _s.size();
		writeInt( len );
		write( _s.c_str(), len );
	}
#endif // SYNC_WITH_SHM_FIFO

#ifndef BUILD_REMOTE_PLUGIN_CLIENT
	inline bool messagesLeft()
	{
#ifdef SYNC_WITH_SHM_FIFO
		return m_in->messagesLeft();
#else
		struct pollfd pollin;
		pollin.fd = m_socket;
		pollin.events = POLLIN;

		if ( poll( &pollin, 1, 0 ) == -1 )
		{
			qWarning( "Unexpected poll error." );
		}
		return pollin.revents & POLLIN;
#endif
	}

	inline void fetchAndProcessAllMessages()
	{
		while( messagesLeft() )
		{
			fetchAndProcessNextMessage();
		}
	}

	static bool isMainThreadWaiting()
	{
		return waitDepthCounter() > 0;
	}
#endif // BUILD_REMOTE_PLUGIN_CLIENT

	virtual bool processMessage( const message & _m ) = 0;


protected:
#ifdef SYNC_WITH_SHM_FIFO
	inline const shmFifo * in() const
	{
		return m_in;
	}

	inline const shmFifo * out() const
	{
		return m_out;
	}
#endif

	inline void invalidate()
	{
#ifdef SYNC_WITH_SHM_FIFO
		m_in->invalidate();
		m_out->invalidate();
		m_in->messageSent();
#else
		m_invalid = true;
#endif
	}


#ifndef SYNC_WITH_SHM_FIFO
	int m_socket;
#endif


private:
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
	static int & waitDepthCounter()
	{
		static int waitDepth = 0;
		return waitDepth;
	}
#endif

#ifdef SYNC_WITH_SHM_FIFO
	shmFifo * m_in;
	shmFifo * m_out;
#else
	void read( void * _buf, int _len )
	{
		if( isInvalid() )
		{
			memset( _buf, 0, _len );
			return;
		}
		char * buf = (char *) _buf;
		int remaining = _len;
		while ( remaining )
		{
			ssize_t nread = ::read( m_socket, buf, remaining );
			switch ( nread )
			{
				case -1:
					fprintf( stderr,
						"Error while reading.\n" );
				case 0:
					invalidate();
					memset( _buf, 0, _len );
					return;
			}
			buf += nread;
			remaining -= nread;
		}
	}

	void write( const void * _buf, int _len )
	{
		if( isInvalid() )
		{
			return;
		}
		const char * buf = (const char *) _buf;
		int remaining = _len;
		while ( remaining )
		{
			ssize_t nwritten = ::write( m_socket, buf, remaining );
			switch ( nwritten )
			{
				case -1:
					fprintf( stderr,
						"Error while writing.\n" );
				case 0:
					invalidate();
					return;
			}
			buf += nwritten;
			remaining -= nwritten;
		}
	}


	bool m_invalid;

	pthread_mutex_t m_receiveMutex;
	pthread_mutex_t m_sendMutex;
#endif // SYNC_WITH_SHM_FIFO

} ;

} // namespace lmms

#endif // LMMS_REMOTE_PLUGIN_BASE_H
