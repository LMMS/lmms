/*
 * RemotePluginClient.h
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

#ifndef LMMS_REMOTE_PLUGIN_CLIENT_H
#define LMMS_REMOTE_PLUGIN_CLIENT_H

#include "RemotePluginBase.h"

#include <stdexcept>

#ifndef LMMS_BUILD_WIN32
#	include <condition_variable>
#	include <mutex>
#	include <thread>

#	include <signal.h>
#	include <unistd.h>
#endif

#include "LmmsTypes.h"
#include "MidiEvent.h"
#include "SharedMemory.h"
#include "VstSyncData.h"

namespace lmms
{

class SampleFrame;

class RemotePluginClient : public RemotePluginBase
{
public:
#ifdef SYNC_WITH_SHM_FIFO
	RemotePluginClient( const std::string& _shm_in, const std::string& _shm_out );
#else
	RemotePluginClient( const char * socketPath );
#endif
	~RemotePluginClient() override;

	const VstSyncData* getVstSyncData();

	bool processMessage( const message & _m ) override;

	virtual void process( const SampleFrame* _in_buf,
					SampleFrame* _out_buf ) = 0;

	virtual void processMidiEvent( const MidiEvent&, const f_cnt_t /* _offset */ )
	{
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

	void setInputOutputCount( int i, int o )
	{
		m_inputCount = i;
		m_outputCount = o;
		sendMessage( message( IdChangeInputOutputCount )
				.addInt( i )
				.addInt( o ) );
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
	void setShmKey(const std::string& key);
	void doProcessing();

	SharedMemory<float[]> m_audioBuffer;
	SharedMemory<const VstSyncData> m_vstSyncData;

	int m_inputCount;
	int m_outputCount;

	sample_rate_t m_sampleRate;
	fpp_t m_bufferSize;
} ;

#ifndef LMMS_BUILD_WIN32
class PollParentThread
{
public:
	PollParentThread() :
		m_stop{false},
		m_thread{
			[this]
			{
				using namespace std::literals::chrono_literals;
				auto lock = std::unique_lock{m_mutex};
				while (!m_cv.wait_for(lock, 500ms, [this] { return m_stop; }))
				{
					if (getppid() == 1)
					{
						kill(getpid(), SIGHUP);
						break;
					}
				}
			}
		}
	{ }

	~PollParentThread()
	{
		{
			const auto lock = std::unique_lock{m_mutex};
			m_stop = true;
		}
		m_cv.notify_all();
		m_thread.join();
	}

private:
	bool m_stop;
	std::mutex m_mutex;
	std::condition_variable m_cv;
	std::thread m_thread;
};
#endif // LMMS_BUILD_WIN32

#ifdef SYNC_WITH_SHM_FIFO
RemotePluginClient::RemotePluginClient( const std::string& _shm_in, const std::string& _shm_out ) :
	RemotePluginBase( new shmFifo( _shm_in ), new shmFifo( _shm_out ) ),
#else
RemotePluginClient::RemotePluginClient( const char * socketPath ) :
	RemotePluginBase(),
#endif
	m_inputCount( 0 ),
	m_outputCount( 0 ),
	m_sampleRate( 44100 ),
	m_bufferSize( 0 )
{
#ifndef SYNC_WITH_SHM_FIFO
	struct sockaddr_un sa;
	sa.sun_family = AF_LOCAL;

	size_t length = strlen( socketPath );
	if ( length >= sizeof sa.sun_path )
	{
		length = sizeof sa.sun_path - 1;
		fprintf( stderr, "Socket path too long.\n" );
	}
	memcpy( sa.sun_path, socketPath, length );
	sa.sun_path[length] = '\0';

	m_socket = socket( PF_LOCAL, SOCK_STREAM, 0 );
	if ( m_socket == -1 )
	{
		fprintf( stderr, "Could not connect to local server.\n" );
	}
	if ( ::connect( m_socket, (struct sockaddr *) &sa, sizeof sa ) == -1 )
	{
		fprintf( stderr, "Could not connect to local server.\n" );
	}
#endif
}




RemotePluginClient::~RemotePluginClient()
{
	sendMessage( IdQuit );

#ifndef SYNC_WITH_SHM_FIFO
	if ( close( m_socket ) == -1)
	{
		fprintf( stderr, "Error freeing resources.\n" );
	}
#endif
}




const VstSyncData* RemotePluginClient::getVstSyncData()
{
	return m_vstSyncData.get();
}




bool RemotePluginClient::processMessage( const message & _m )
{
	message reply_message( _m.id );
	bool reply = false;
	switch( _m.id )
	{
		case IdUndefined:
			return false;

		case IdSyncKey:
			try
			{
				m_vstSyncData.attach(_m.getString(0));
			}
			catch (const std::runtime_error& error)
			{
				debugMessage(std::string{"Failed to attach sync data: "} + error.what() + '\n');
				std::exit(EXIT_FAILURE);
			}
			m_bufferSize = m_vstSyncData->bufferSize;
			m_sampleRate = m_vstSyncData->sampleRate;
			reply_message.id = IdHostInfoGotten;
			reply = true;
			break;

		case IdSampleRateInformation:
			m_sampleRate = _m.getInt();
			updateSampleRate();
			reply_message.id = IdInformationUpdated;
			reply = true;
			break;

		case IdBufferSizeInformation:
			// Should LMMS gain the ability to change buffer size
			// without a restart, it must wait for this message to
			// complete processing or else risk VST crashes
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
			setShmKey(_m.getString(0));
			break;

		case IdInitDone:
			break;

		default:
		{
			char buf[64];
			std::snprintf(buf, 64, "undefined message: %d\n", _m.id);
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




void RemotePluginClient::setShmKey(const std::string& key)
{
	try
	{
		m_audioBuffer.attach(key);
	}
	catch (const std::runtime_error& error)
	{
		debugMessage(std::string{"failed getting shared memory: "} + error.what() + '\n');
	}
}




void RemotePluginClient::doProcessing()
{
	if (m_audioBuffer)
	{
		process( (SampleFrame*)( m_inputCount > 0 ? m_audioBuffer.get() : nullptr ),
				(SampleFrame*)( m_audioBuffer.get() +
					( m_inputCount*m_bufferSize ) ) );
	}
	else
	{
		debugMessage( "doProcessing(): have no shared memory!\n" );
	}
}


} // namespace lmms

#endif // LMMS_REMOTE_PLUGIN_CLIENT_H
