/*
 * RemotePlugin.h - base class providing RPC like mechanisms
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

#ifndef _REMOTE_PLUGIN_H
#define _REMOTE_PLUGIN_H

#include "export.h"
#include "MidiEvent.h"
#include "VstSyncData.h"

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cassert>

#ifdef LMMS_BUILD_WIN32
#define USE_QT_SEMAPHORES
#define USE_QT_SHMEM
#endif

#ifdef LMMS_BUILD_APPLE
#define USE_QT_SEMAPHORES
#endif


#ifdef USE_QT_SEMAPHORES

#ifdef LMMS_HAVE_PROCESS_H
#include <process.h>
#endif

#include <QtCore/QtGlobal>

#if QT_VERSION >= 0x040400
#include <QtCore/QSystemSemaphore>
#else
#error building LMMS on this platform requires at least Qt 4.4.0
#endif

#else /* USE_QT_SEMAPHORES */

#ifdef LMMS_HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef LMMS_HAVE_SEMAPHORE_H
#include <semaphore.h>
#endif

#endif


#ifdef USE_QT_SHMEM

#include <QtCore/QtGlobal>

#if QT_VERSION >= 0x040400
#include <QtCore/QSharedMemory>
#else
#error building LMMS on this platform requires at least Qt 4.4.0
#endif

typedef int32_t key_t;

#else /* USE_QT_SHMEM */

#ifdef LMMS_HAVE_SYS_SHM_H
#include <sys/shm.h>
#endif

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#endif



#ifdef LMMS_HAVE_LOCALE_H
#include <locale.h>
#endif


#ifdef BUILD_REMOTE_PLUGIN_CLIENT
#undef EXPORT
#define EXPORT
#define COMPILE_REMOTE_PLUGIN_BASE
#else
#include <QtCore/QMutex>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#endif

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
#ifndef USE_QT_SEMAPHORES
		sem_t sem;
#endif
		int semKey;
		char fill[32];
	} ;
	struct shmData
	{
		sem32_t dataSem;	// semaphore for locking this
					// FIFO management data
		sem32_t messageSem;	// semaphore for incoming messages
		volatile int32_t startPtr; // current start of FIFO in memory
		volatile int32_t endPtr;   // current end of FIFO in memory
		char data[SHM_FIFO_SIZE];  // actual data
	} ;

public:
	// constructor for master-side
	shmFifo() :
		m_invalid( false ),
		m_master( true ),
		m_shmKey( 0 ),
#ifdef USE_QT_SHMEM
		m_shmObj(),
#else
		m_shmID( -1 ),
#endif
		m_data( NULL ),
#ifdef USE_QT_SEMAPHORES
		m_dataSem( QString::null ),
		m_messageSem( QString::null ),
#else
		m_dataSem( NULL ),
		m_messageSem( NULL ),
#endif
		m_lockDepth( 0 )
	{
#ifdef USE_QT_SHMEM
		do
		{
			m_shmObj.setKey( QString( "%1" ).arg( ++m_shmKey ) );
			m_shmObj.create( sizeof( shmData ) );
		} while( m_shmObj.error() != QSharedMemory::NoError );

		m_data = (shmData *) m_shmObj.data();
#else
		while( ( m_shmID = shmget( ++m_shmKey, sizeof( shmData ),
					IPC_CREAT | IPC_EXCL | 0600 ) ) == -1 )
		{
		}
		m_data = (shmData *) shmat( m_shmID, 0, 0 );
#endif
		assert( m_data != NULL );
		m_data->startPtr = m_data->endPtr = 0;
#ifdef USE_QT_SEMAPHORES
		static int k = 0;
		m_data->dataSem.semKey = ( getpid()<<10 ) + ++k;
		m_data->messageSem.semKey = ( getpid()<<10 ) + ++k;
		m_dataSem.setKey( QString::number( m_data->dataSem.semKey ),
						1, QSystemSemaphore::Create );
		m_messageSem.setKey( QString::number(
						m_data->messageSem.semKey ),
						0, QSystemSemaphore::Create );
#else
		m_dataSem = &m_data->dataSem.sem;
		m_messageSem = &m_data->messageSem.sem;

		if( sem_init( m_dataSem, 1, 1 ) )
		{
			fprintf( stderr, "could not initialize m_dataSem\n" );
		}
		if( sem_init( m_messageSem, 1, 0 ) )
		{
			fprintf( stderr, "could not initialize "
							"m_messageSem\n" );
		}
#endif
	}

	// constructor for remote-/client-side - use _shm_key for making up
	// the connection to master
	shmFifo( key_t _shm_key ) :
		m_invalid( false ),
		m_master( false ),
		m_shmKey( 0 ),
#ifdef USE_QT_SHMEM
		m_shmObj( QString::number( _shm_key ) ),
#else
		m_shmID( shmget( _shm_key, 0, 0 ) ),
#endif
		m_data( NULL ),
#ifdef USE_QT_SEMAPHORES
		m_dataSem( QString::null ),
		m_messageSem( QString::null ),
#else
		m_dataSem( NULL ),
		m_messageSem( NULL ),
#endif
		m_lockDepth( 0 )
	{
#ifdef USE_QT_SHMEM
		if( m_shmObj.attach() )
		{
			m_data = (shmData *) m_shmObj.data();
		}
#else
		if( m_shmID != -1 )
		{
			m_data = (shmData *) shmat( m_shmID, 0, 0 );
		}
#endif
		assert( m_data != NULL );
#ifdef USE_QT_SEMAPHORES
		m_dataSem.setKey( QString::number( m_data->dataSem.semKey ) );
		m_messageSem.setKey( QString::number(
						m_data->messageSem.semKey ) );
#else
		m_dataSem = &m_data->dataSem.sem;
		m_messageSem = &m_data->messageSem.sem;
#endif
	}

	~shmFifo()
	{
#ifndef USE_QT_SHMEM
		shmdt( m_data );
#endif
		// master?
		if( m_master )
		{
#ifndef USE_QT_SHMEM
			shmctl( m_shmID, IPC_RMID, NULL );
#endif
#ifndef USE_QT_SEMAPHORES
			sem_destroy( m_dataSem );
			sem_destroy( m_messageSem );
#endif
		}
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
		if( !isInvalid() && __sync_add_and_fetch( &m_lockDepth, 1 ) == 1 )
		{
#ifdef USE_QT_SEMAPHORES
			m_dataSem.acquire();
#else
			sem_wait( m_dataSem );
#endif
		}
	}

	// recursive unlock
	inline void unlock()
	{
		if( __sync_sub_and_fetch( &m_lockDepth, 1) <= 0 )
		{
#ifdef USE_QT_SEMAPHORES
			m_dataSem.release();
#else
			sem_post( m_dataSem );
#endif
		}
	}

	// wait until message-semaphore is available
	inline void waitForMessage()
	{
		if( !isInvalid() )
		{
#ifdef USE_QT_SEMAPHORES
			m_messageSem.acquire();
#else
			sem_wait( m_messageSem );
#endif
		}
	}

	// increase message-semaphore
	inline void messageSent()
	{
#ifdef USE_QT_SEMAPHORES
		m_messageSem.release();
#else
		sem_post( m_messageSem );
#endif
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
#ifdef USE_QT_SEMAPHORES
		lock();
		const bool empty = ( m_data->startPtr == m_data->endPtr );
		unlock();
		return !empty;
#else
		int v;
		sem_getvalue( m_messageSem, &v );
		return v > 0;
#endif
	}


	inline int shmKey() const
	{
		return m_shmKey;
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
	key_t m_shmKey;
#ifdef USE_QT_SHMEM
	QSharedMemory m_shmObj;
#else
	int m_shmID;
#endif
	shmData * m_data;
#ifdef USE_QT_SEMAPHORES
	QSystemSemaphore m_dataSem;
	QSystemSemaphore m_messageSem;
#else
	sem_t * m_dataSem;
	sem_t * m_messageSem;
#endif
	volatile int m_lockDepth;

} ;



enum RemoteMessageIDs
{
	IdUndefined,
	IdInitDone,
	IdQuit,
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
	IdSavePresetFile,
	IdLoadPresetFile,
	IdDebugMessage,
	IdUserBase = 64
} ;



class EXPORT RemotePluginBase
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

		inline message & addString( const std::string & _s )
		{
			data.push_back( _s );
			return *this;
		}

		message & addInt( int _i )
		{
			char buf[32];
			sprintf( buf, "%d", _i );
			data.push_back( std::string( buf ) );
			return *this;
		}

		message & addFloat( float _f )
		{
			char buf[32];
			sprintf( buf, "%f", _f );
			data.push_back( std::string( buf ) );
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

	RemotePluginBase( shmFifo * _in, shmFifo * _out );
	virtual ~RemotePluginBase();

	void reset( shmFifo *in, shmFifo *out )
	{
		delete m_in;
		delete m_out;
		m_in = in;
		m_out = out;
	}

	int sendMessage( const message & _m );
	message receiveMessage();

	inline bool isInvalid() const
	{
		return m_in->isInvalid() || m_out->isInvalid();
	}

	inline bool messagesLeft()
	{
		return m_in->messagesLeft();
	}


	message waitForMessage( const message & _m,
						bool _busy_waiting = false );

	inline message fetchAndProcessNextMessage()
	{
		message m = receiveMessage();
		processMessage( m );
		return m;
	}

	inline void fetchAndProcessAllMessages()
	{
		while( messagesLeft() )
		{
			fetchAndProcessNextMessage();
		}
	}

	virtual bool processMessage( const message & _m ) = 0;


protected:
	inline const shmFifo * in() const
	{
		return m_in;
	}

	inline const shmFifo * out() const
	{
		return m_out;
	}

	inline void invalidate()
	{
		m_in->invalidate();
		m_out->invalidate();
		m_in->messageSent();
	}


private:
	shmFifo * m_in;
	shmFifo * m_out;

} ;



#ifndef BUILD_REMOTE_PLUGIN_CLIENT


class RemotePlugin;

class ProcessWatcher : public QThread
{
public:
	ProcessWatcher( RemotePlugin * );
	virtual ~ProcessWatcher()
	{
	}


	void quit()
	{
		m_quit = true;
	}

private:
	virtual void run();

	RemotePlugin * m_plugin;
	volatile bool m_quit;

} ;


class EXPORT RemotePlugin : public RemotePluginBase
{
public:
	RemotePlugin();
	virtual ~RemotePlugin();

	inline bool isRunning()
	{
#ifdef DEBUG_REMOTE_PLUGIN
		return true;
#else
		return m_process.state() != QProcess::NotRunning;
#endif
	}

	bool init( const QString &pluginExecutable, bool waitForInitDoneMsg );

	inline void waitForInitDone( bool _busyWaiting = true )
	{
		m_failed = waitForMessage( IdInitDone, _busyWaiting ).id != IdInitDone;
	}

	virtual bool processMessage( const message & _m );

	bool process( const sampleFrame * _in_buf, sampleFrame * _out_buf );

	void processMidiEvent( const MidiEvent&, const f_cnt_t _offset );

	void updateSampleRate( sample_rate_t _sr )
	{
		lock();
		sendMessage( message( IdSampleRateInformation ).addInt( _sr ) );
		unlock();
	}

	void showUI()
	{
		lock();
		sendMessage( IdShowUI );
		unlock();
	}

	void hideUI()
	{
		lock();
		sendMessage( IdHideUI );
		unlock();
	}

	inline bool failed() const
	{
		return m_failed;
	}

	inline void lock()
	{
		if( !isInvalid() )
		{
			m_commMutex.lock();
		}
	}

	inline void unlock()
	{
		if( !isInvalid() )
		{
			m_commMutex.unlock();
		}
	}


protected:
	inline void setSplittedChannels( bool _on )
	{
		m_splitChannels = _on;
	}


private:
	void resizeSharedProcessingMemory();


	bool m_failed;

	QProcess m_process;
	ProcessWatcher m_watcher;

	QMutex m_commMutex;
	bool m_splitChannels;
#ifdef USE_QT_SHMEM
	QSharedMemory m_shmObj;
#else
	int m_shmID;
#endif
	size_t m_shmSize;
	float * m_shm;

	int m_inputCount;
	int m_outputCount;

	friend class ProcessWatcher;
} ;

#endif


#ifdef BUILD_REMOTE_PLUGIN_CLIENT

class RemotePluginClient : public RemotePluginBase
{
public:
	RemotePluginClient( key_t _shm_in, key_t _shm_out );
	virtual ~RemotePluginClient();
#ifdef USE_QT_SHMEM
	VstSyncData * getQtVSTshm();
#endif
	virtual bool processMessage( const message & _m );

	virtual void process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf ) = 0;

	virtual void processMidiEvent( const MidiEvent&, const f_cnt_t /* _offset */ )
	{
	}

	inline float * sharedMemory()
	{
		return m_shm;
	}

	virtual void updateSampleRate()
	{
	}

	virtual void updateBufferSize()
	{
	}

	inline sample_rate_t sampleRate() const
	{
		return m_sampleRate;
	}

	inline fpp_t bufferSize() const
	{
		return m_bufferSize;
	}

	void setInputCount( int _i )
	{
		m_inputCount = _i;
		sendMessage( message( IdChangeInputCount ).addInt( _i ) );
	}

	void setOutputCount( int _i )
	{
		m_outputCount = _i;
		sendMessage( message( IdChangeOutputCount ).addInt( _i ) );
	}

	virtual int inputCount() const
	{
		return m_inputCount;
	}

	virtual int outputCount() const
	{
		return m_outputCount;
	}

	void debugMessage( const std::string & _s )
	{
		sendMessage( message( IdDebugMessage ).addString( _s ) );
	}


private:
	void setShmKey( key_t _key, int _size );
	void doProcessing();

#ifdef USE_QT_SHMEM
	QSharedMemory m_shmObj;
	QSharedMemory m_shmQtID;
#endif
	VstSyncData * m_vstSyncData;
	float * m_shm;

	int m_inputCount;
	int m_outputCount;

	sample_rate_t m_sampleRate;
	fpp_t m_bufferSize;

} ;

#endif





#ifdef COMPILE_REMOTE_PLUGIN_BASE

#ifndef BUILD_REMOTE_PLUGIN_CLIENT
#include <QtCore/QCoreApplication>
#endif


RemotePluginBase::RemotePluginBase( shmFifo * _in, shmFifo * _out ) :
	m_in( _in ),
	m_out( _out )
{
#ifdef LMMS_HAVE_LOCALE_H
	// make sure, we're using common ways to print/scan
	// floats to/from strings (',' vs. '.' for decimal point etc.)
	setlocale( LC_NUMERIC, "C" );
#endif
}




RemotePluginBase::~RemotePluginBase()
{
	delete m_in;
	delete m_out;
}




int RemotePluginBase::sendMessage( const message & _m )
{
	m_out->lock();
	m_out->writeInt( _m.id );
	m_out->writeInt( _m.data.size() );
	int j = 8;
	for( unsigned int i = 0; i < _m.data.size(); ++i )
	{
		m_out->writeString( _m.data[i] );
		j += 4 + _m.data[i].size();
	}
	m_out->unlock();
	m_out->messageSent();

	return j;
}




RemotePluginBase::message RemotePluginBase::receiveMessage()
{
	m_in->waitForMessage();
	m_in->lock();
	message m;
	m.id = m_in->readInt();
	const int s = m_in->readInt();
	for( int i = 0; i < s; ++i )
	{
		m.data.push_back( m_in->readString() );
	}
	m_in->unlock();
	return m;
}




RemotePluginBase::message RemotePluginBase::waitForMessage(
							const message & _wm,
							bool _busy_waiting )
{
	while( !isInvalid() )
	{
#ifndef BUILD_REMOTE_PLUGIN_CLIENT
		if( _busy_waiting && !messagesLeft() )
		{
			QCoreApplication::processEvents(
				QEventLoop::ExcludeUserInputEvents, 50 );
			continue;
		}
#endif
		message m = receiveMessage();
		processMessage( m );
		if( m.id == _wm.id )
		{
			return m;
		}
		else if( m.id == IdUndefined )
		{
			return m;
		}
	}

	return message();
}


#endif





#ifdef BUILD_REMOTE_PLUGIN_CLIENT


RemotePluginClient::RemotePluginClient( key_t _shm_in, key_t _shm_out ) :
	RemotePluginBase( new shmFifo( _shm_in ), new shmFifo( _shm_out ) ),
#ifdef USE_QT_SHMEM
	m_shmObj(),
	m_shmQtID( "/usr/bin/lmms" ),
#endif
	m_vstSyncData( NULL ),
	m_shm( NULL ),
	m_inputCount( 0 ),
	m_outputCount( 0 ),
	m_sampleRate( 44100 ),
	m_bufferSize( 0 )
{
#ifdef USE_QT_SHMEM
	if( m_shmQtID.attach( QSharedMemory::ReadOnly ) )
	{
		m_vstSyncData = (VstSyncData *) m_shmQtID.data();
		m_bufferSize = m_vstSyncData->m_bufferSize;
		m_sampleRate = m_vstSyncData->m_sampleRate;
		return;
	}
#else
	key_t key;
	int m_shmID;

	if( ( key = ftok( VST_SNC_SHM_KEY_FILE, 'R' ) ) == -1 )
	{
		perror( "RemotePluginClient::ftok" );
	}
	else
	{	// connect to shared memory segment
		if( ( m_shmID = shmget( key, 0, 0 ) ) == -1 )
		{
			perror( "RemotePluginClient::shmget" );
		}
		else
		{	// attach segment
			m_vstSyncData = (VstSyncData *)shmat(m_shmID, 0, 0);
			if( m_vstSyncData == (VstSyncData *)( -1 ) )
			{
				perror( "RemotePluginClient::shmat" );
			}
			else
			{
				m_bufferSize = m_vstSyncData->m_bufferSize;
				m_sampleRate = m_vstSyncData->m_sampleRate;

				// detach segment
				if( shmdt(m_vstSyncData) == -1 )
				{
					perror("RemotePluginClient::shmdt");
				}
				return;
			}
		}
	}
#endif
	// if attaching shared memory fails
	sendMessage( IdSampleRateInformation );
	sendMessage( IdBufferSizeInformation );
}




RemotePluginClient::~RemotePluginClient()
{
#ifdef USE_QT_SHMEM
	m_shmQtID.detach();
#endif
	sendMessage( IdQuit );

#ifndef USE_QT_SHMEM
	shmdt( m_shm );
#endif
}



#ifdef USE_QT_SHMEM
VstSyncData * RemotePluginClient::getQtVSTshm()
{
	return m_vstSyncData;
}
#endif



bool RemotePluginClient::processMessage( const message & _m )
{
	message reply_message( _m.id );
	bool reply = false;
	switch( _m.id )
	{
		case IdUndefined:
			return false;

		case IdSampleRateInformation:
			m_sampleRate = _m.getInt();
			updateSampleRate();
			break;

		case IdBufferSizeInformation:
			m_bufferSize = _m.getInt();
			updateBufferSize();
			break;

		case IdQuit:
			return false;

		case IdMidiEvent:
			processMidiEvent(
				MidiEvent( static_cast<MidiEventTypes>(
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

		case IdInitDone:
			break;

		default:
		{
			char buf[64];
			sprintf( buf, "undefined message: %d\n", (int) _m.id );
			debugMessage( buf );
			break;
		}
	}
	if( reply )
	{
		sendMessage( reply_message );
	}

	return true;
}




void RemotePluginClient::setShmKey( key_t _key, int _size )
{
#ifdef USE_QT_SHMEM
	m_shmObj.setKey( QString::number( _key ) );
	if( m_shmObj.attach() || m_shmObj.error() == QSharedMemory::NoError )
	{
		m_shm = (float *) m_shmObj.data();
	}
	else
	{
		char buf[64];
		sprintf( buf, "failed getting shared memory: %d\n", m_shmObj.error() );
		debugMessage( buf );
	}
#else
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
		debugMessage( "failed getting shared memory\n" );
	}
	else
	{
		m_shm = (float *) shmat( shm_id, 0, 0 );
	}
#endif
}




void RemotePluginClient::doProcessing()
{
	if( m_shm != NULL )
	{
		process( (sampleFrame *)( m_inputCount > 0 ? m_shm : NULL ),
				(sampleFrame *)( m_shm +
					( m_inputCount*m_bufferSize ) ) );
	}
	else
	{
		debugMessage( "doProcessing(): have no shared memory!\n" );
	}
}



#endif

#define QSTR_TO_STDSTR(s)	std::string( s.toUtf8().constData() )

#endif
