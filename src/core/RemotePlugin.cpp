/*
 * RemotePlugin.cpp - base class providing RPC like mechanisms
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

#define COMPILE_REMOTE_PLUGIN_BASE
//#define DEBUG_REMOTE_PLUGIN
#ifdef DEBUG_REMOTE_PLUGIN
#include <QtCore/QDebug>
#endif

#include "RemotePlugin.h"
#include "mixer.h"
#include "engine.h"
#include "config_mgr.h"

#include <QtCore/QDir>

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif


// simple helper thread monitoring our RemotePlugin - if process terminates
// unexpectedly invalidate plugin so LMMS doesn't lock up
ProcessWatcher::ProcessWatcher( RemotePlugin * _p ) :
	QThread(),
	m_plugin( _p ),
	m_quit( false )
{
}


void ProcessWatcher::run()
{
	while( !m_quit && m_plugin->isRunning() )
	{
		msleep( 200 );
	}
	if( !m_quit )
	{
		fprintf( stderr,
				"remote plugin died! invalidating now.\n" );
		m_plugin->invalidate();
	}
}





RemotePlugin::RemotePlugin( const QString & _plugin_executable,
						bool _wait_for_init_done ) :
	RemotePluginBase( new shmFifo(), new shmFifo() ),
	m_failed( true ),
	m_watcher( this ),
	m_commMutex( QMutex::Recursive ),
	m_splitChannels( false ),
#ifdef USE_QT_SHMEM
	m_shmObj(),
#else
	m_shmID( 0 ),
#endif
	m_shmSize( 0 ),
	m_shm( NULL ),
	m_inputCount( DEFAULT_CHANNELS ),
	m_outputCount( DEFAULT_CHANNELS )
{
	lock();
	QString exec = configManager::inst()->pluginDir() +
					QDir::separator() + _plugin_executable;
	QStringList args;
	// swap in and out for bidirectional communication
	args << QString::number( out()->shmKey() );
	args << QString::number( in()->shmKey() );
	m_process.setProcessChannelMode( QProcess::MergedChannels );
#ifndef DEBUG_REMOTE_PLUGIN
	m_process.start( exec, args );

	m_watcher.start( QThread::LowestPriority );
#else
	qDebug() << exec << args;
#endif

	resizeSharedProcessingMemory();

	if( _wait_for_init_done )
	{
		waitForInitDone();
	}
	unlock();
}




RemotePlugin::~RemotePlugin()
{
	m_watcher.quit();
	m_watcher.wait();

	if( m_failed == false )
	{
		if( isRunning() )
		{
			lock();
			sendMessage( IdQuit );

			m_process.waitForFinished( 1000 );
			if( m_process.state() != QProcess::NotRunning )
			{
				m_process.terminate();
				m_process.kill();
			}
			unlock();
		}

#ifndef USE_QT_SHMEM
		shmdt( m_shm );
		shmctl( m_shmID, IPC_RMID, NULL );
#endif
	}
}





bool RemotePlugin::process( const sampleFrame * _in_buf,
						sampleFrame * _out_buf )
{
	const fpp_t frames = engine::getMixer()->framesPerPeriod();

	if( m_failed || !isRunning() )
	{
		if( _out_buf != NULL )
		{
			engine::getMixer()->clearAudioBuffer( _out_buf,
								frames );
		}
		return false;
	}

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
		return false;
	}

	memset( m_shm, 0, m_shmSize );

	ch_cnt_t inputs = qMin<ch_cnt_t>( m_inputCount, DEFAULT_CHANNELS );

	if( _in_buf != NULL && inputs > 0 )
	{
		if( m_splitChannels )
		{
			for( ch_cnt_t ch = 0; ch < inputs; ++ch )
			{
				for( fpp_t frame = 0; frame < frames; ++frame )
				{
					m_shm[ch * frames + frame] =
							_in_buf[frame][ch];
				}
			}
		}
		else if( inputs == DEFAULT_CHANNELS )
		{
			memcpy( m_shm, _in_buf, frames * BYTES_PER_FRAME );
		}
		else
		{
			sampleFrame * o = (sampleFrame *) m_shm;
			for( ch_cnt_t ch = 0; ch < inputs; ++ch )
			{
				for( fpp_t frame = 0; frame < frames; ++frame )
				{
					o[frame][ch] = _in_buf[frame][ch];
				}
			}
		}
	}

	lock();
	sendMessage( IdStartProcessing );
	unlock();

	if( m_failed || _out_buf == NULL || m_outputCount == 0 )
	{
		return false;
	}

	lock();
	waitForMessage( IdProcessingDone );
	unlock();

	const ch_cnt_t outputs = qMin<ch_cnt_t>( m_outputCount,
							DEFAULT_CHANNELS );
	if( m_splitChannels )
	{
		for( ch_cnt_t ch = 0; ch < outputs; ++ch )
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				_out_buf[frame][ch] = m_shm[( m_inputCount+ch )*
								frames + frame];
			}
		}
	}
	else if( outputs == DEFAULT_CHANNELS )
	{
		memcpy( _out_buf, m_shm + m_inputCount * frames,
						frames * BYTES_PER_FRAME );
	}
	else
	{
		sampleFrame * o = (sampleFrame *) ( m_shm +
							m_inputCount*frames );
		// clear buffer, if plugin didn't fill up both channels
		engine::getMixer()->clearAudioBuffer( _out_buf, frames );

		for( ch_cnt_t ch = 0; ch <
				qMin<int>( DEFAULT_CHANNELS, outputs ); ++ch )
		{
			for( fpp_t frame = 0; frame < frames; ++frame )
			{
				_out_buf[frame][ch] = o[frame][ch];
			}
		}
	}

	return true;
}




void RemotePlugin::processMidiEvent( const midiEvent & _e,
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




void RemotePlugin::resizeSharedProcessingMemory()
{
	const size_t s = ( m_inputCount+m_outputCount ) *
				engine::getMixer()->framesPerPeriod() *
							sizeof( float );
	if( m_shm != NULL )
	{
#ifdef USE_QT_SHMEM
		m_shmObj.detach();
#else
		shmdt( m_shm );
		shmctl( m_shmID, IPC_RMID, NULL );
#endif
	}

	int shm_key = 0;
#ifdef USE_QT_SHMEM
	do
	{
		m_shmObj.setKey( QString( "%1" ).arg( ++shm_key ) );
		m_shmObj.create( s );
	} while( m_shmObj.error() != QSharedMemory::NoError );

	m_shm = (float *) m_shmObj.data();
#else
	while( ( m_shmID = shmget( ++shm_key, s, IPC_CREAT | IPC_EXCL |
								0600 ) ) == -1 )
	{
	}

	m_shm = (float *) shmat( m_shmID, 0, 0 );
#endif
	m_shmSize = s;
	sendMessage( message( IdChangeSharedMemoryKey ).
				addInt( shm_key ).addInt( m_shmSize ) );
}




bool RemotePlugin::processMessage( const message & _m )
{
	lock();
	message reply_message( _m.id );
	bool reply = false;
	switch( _m.id )
	{
		case IdUndefined:
			return false;

		case IdInitDone:
			reply = true;
			break;

		case IdSampleRateInformation:
			reply = true;
			reply_message.addInt( engine::getMixer()->processingSampleRate() );
			break;

		case IdBufferSizeInformation:
			reply = true;
			reply_message.addInt( engine::getMixer()->framesPerPeriod() );
			break;

		case IdChangeInputCount:
			m_inputCount = _m.getInt( 0 );
			resizeSharedProcessingMemory();
			break;

		case IdChangeOutputCount:
			m_outputCount = _m.getInt( 0 );
			resizeSharedProcessingMemory();
			break;

		case IdDebugMessage:
			fprintf( stderr, "RemotePlugin::DebugMessage: %s",
						_m.getString( 0 ).c_str() );
			break;

		case IdProcessingDone:
		case IdQuit:
		default:
			break;
	}
	if( reply )
	{
		sendMessage( reply_message );
	}
	unlock();

	return true;
}




#include "moc_RemotePlugin.cxx"

