/*
 * remote_plugin.cpp - base class providing RPC like mechanisms
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


#define COMPILE_REMOTE_PLUGIN_BASE

#include "remote_plugin.h"
#include "lmmsconfig.h"
#include "engine.h"
#include "config_mgr.h"

#include <QtCore/QDir>
#include <QtCore/QTime>

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef LMMS_HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <cstdio>

#ifdef LMMS_HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef LMMS_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef LMMS_HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif







remotePlugin::remotePlugin( const QString & _plugin_executable ) :
	remotePluginBase(),
	m_initialized( false ),
	m_failed( true ),
	m_pluginPID( -1 ),
	m_commMutex( QMutex::Recursive ),
#ifdef USE_NATIVE_SHM_API
	m_shmID( 0 ),
#else
	m_shmObj(),
#endif
	m_shmSize( 0 ),
	m_shm( NULL ),
	m_inputCount( DEFAULT_CHANNELS ),
	m_outputCount( DEFAULT_CHANNELS )
{
	QString fe = configManager::inst()->pluginDir() +
					QDir::separator() + _plugin_executable;
	if( pipe( m_pipes[0] ) || pipe( m_pipes[1] ) )
	{
		printf( "error while creating pipes!\n" );
	}

	if( ( m_pluginPID = fork() ) < 0 )
	{
		printf( "fork() failed!\n" );
		return;
	}
	else if( m_pluginPID == 0 )
	{
		dup2( m_pipes[0][0], 0 );
		dup2( m_pipes[1][1], 1 );
		execlp( fe.toAscii().constData(), fe.toAscii().constData(),
									NULL );
		exit( 0 );
	}
	setInFD( m_pipes[1][0] );
	setOutFD( m_pipes[0][1] );

	resizeSharedMemory();

	lock();
	m_failed = waitForMessage( IdInitDone ).id != IdInitDone;
	unlock();
}




remotePlugin::~remotePlugin()
{
	if( m_failed == false )
	{
		sendMessage( IdClosePlugin );

		// wait for acknowledge
		QTime t;
		t.start();
		while( t.elapsed() < 1000 )
		{
			if( messagesLeft() &&
				receiveMessage().id == IdClosePlugin )
			{
				//m_pluginPID = 0;
				break;
			}
			usleep( 10 );
		}
		// timeout?
/*		if( m_pluginPID != 0 )
		{*/
			kill( m_pluginPID, SIGTERM );
			kill( m_pluginPID, SIGKILL );
		//}
		// remove process from PCB
		waitpid( m_pluginPID, NULL, 0 );

		// close all sides of our pipes
		close( m_pipes[0][0] );
		close( m_pipes[0][1] );
		close( m_pipes[1][0] );
		close( m_pipes[1][1] );

#ifdef USE_NATIVE_SHM_API
		shmdt( m_shm );
		shmctl( m_shmID, IPC_RMID, NULL );
#endif
	}
}





bool remotePlugin::process( const sampleFrame * _in_buf,
					sampleFrame * _out_buf, bool _wait )
{
	const fpp_t frames = engine::getMixer()->framesPerPeriod();

	if( m_shm == NULL )
	{
		// m_shm being zero means we didn't initialize everything so
		// far so process one message each time (and hope we get
		// information like SHM-key etc.) until we process messages
		// in a later stage of this procedure
		if( m_shmSize == 0 )
		{
			lock();
			fetchAndProcessAllMessages();
			unlock();
		}
		if( _out_buf != NULL )
		{
			engine::getMixer()->clearAudioBuffer( _out_buf,
								frames );
		}
		return( false );
	}

	memset( m_shm, 0, m_shmSize );

	ch_cnt_t inputs = tMax<ch_cnt_t>( m_inputCount, DEFAULT_CHANNELS );

	if( _in_buf != NULL && inputs > 0 )
	{
		for( ch_cnt_t ch = 0; ch < inputs; ++ch )
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				m_shm[ch * frames + frame] = _in_buf[frame][ch];
			}
		}
	}

	lock();
	sendMessage( IdStartProcessing );

	m_initialized = TRUE;
	if( _wait )
	{
		waitForProcessingFinished( _out_buf );
	}
	unlock();
	return( TRUE );
}




bool remotePlugin::waitForProcessingFinished( sampleFrame * _out_buf )
{
	if( !m_initialized || _out_buf == NULL || m_outputCount == 0 )
	{
		return( false );
	}

	lock();
	waitForMessage( IdProcessingDone );
	unlock();

	const fpp_t frames = engine::getMixer()->framesPerPeriod();
	const ch_cnt_t outputs = tMax<ch_cnt_t>( m_outputCount,
							DEFAULT_CHANNELS );
	if( outputs != DEFAULT_CHANNELS )
	{
		// clear buffer, if plugin didn't fill up both channels
		engine::getMixer()->clearAudioBuffer( _out_buf, frames );
	}

	sampleFrame * o = (sampleFrame *) ( m_shm + m_inputCount*frames );
	for( ch_cnt_t ch = 0; ch < outputs; ++ch )
	{
		for( fpp_t frame = 0; frame < frames; ++frame )
		{
			_out_buf[frame][ch] = o[frame][ch];
		}
	}

	return( TRUE );
}




void remotePlugin::processMidiEvent( const midiEvent & _e,
							const f_cnt_t _offset )
{
	message m( IdMidiEvent );
	m.addInt( _e.m_type );
	m.addInt( _e.m_channel );
	m.addInt( _e.m_data.m_param[0] );
	m.addInt( _e.m_data.m_param[1] );
	m.addInt( _offset );
	lock();
	sendMessage( m );
	unlock();
}




void remotePlugin::resizeSharedMemory( void )
{
	const size_t s = ( m_inputCount+m_outputCount ) *
				engine::getMixer()->framesPerPeriod() *
							sizeof( float );
	if( m_shm != NULL )
	{
#ifdef USE_NATIVE_SHM_API
		shmdt( m_shm );
		shmctl( m_shmID, IPC_RMID, NULL );
#else
		m_shmObj.detach();
#endif
	}

	int shm_key = 0;
#ifdef USE_NATIVE_SHM_API
	while( ( m_shmID = shmget( ++shm_key, s, IPC_CREAT | IPC_EXCL |
								0600 ) ) == -1 )
	{
	}

	m_shm = (float *) shmat( m_shmID, 0, 0 );
#else
	do
	{
		m_shmObj.setKey( QString( "%1" ).arg( ++shm_key ) );
		m_shmObj.create( s );
	} while( m_shmObj.error() != QSharedMemory::NoError );

	m_shm = (float *) m_shmObj.data();
#endif
	m_shmSize = s;
	message m( IdChangeSharedMemoryKey );
	m.addInt( shm_key );
	m.addInt( m_shmSize );
	sendMessage( m );
}







bool remotePlugin::processMessage( const message & _m )
{
	lock();
	message reply_message( _m.id );
	bool reply = false;
	switch( _m.id )
	{
		case IdGeneralFailure:
			return( false );

		case IdDebugMessage:
			printf( "debug message from client: %s\n",
						_m.data[0].c_str() );
			break;

		case IdInitDone:
			reply = true;
			break;

		case IdSampleRateInformation:
			reply = true;
			reply_message.addInt(
				engine::getMixer()->processingSampleRate() );
			break;

		case IdBufferSizeInformation:
			reply = true;
			reply_message.addInt(
					engine::getMixer()->framesPerPeriod() );
			break;

		case IdChangeInputCount:
			m_inputCount = _m.getInt( 0 );
			resizeSharedMemory();
			break;

		case IdChangeOutputCount:
			m_outputCount = _m.getInt( 0 );
			resizeSharedMemory();
			break;

		case IdProcessingDone:
		case IdClosePlugin:
		case IdUndefined:
		default:
			break;
	}
	if( reply )
	{
		sendMessage( reply_message );
	}
	unlock();

	return( true );
}






