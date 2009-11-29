/*
 * AudioOutputContext.cpp - centralize all audio output related functionality
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioBackend.h"
#include "AudioOutputContext.h"
#include "Cpu.h"

#include "config_mgr.h"
#include "engine.h"

AudioOutputContext::BufferFifo::BufferFifo( int _size, int _bufferSize ) :
	m_readerSem( _size ),
	m_writerSem( _size ),
	m_readerIndex( 0 ),
	m_writerIndex( 0 ),
	m_size( _size ),
	m_bufferSize( _bufferSize )
{
	m_buffers = new sampleFrameA *[m_size];
	for( int i = 0; i < m_size; ++i )
	{
		m_buffers[i] = CPU::allocFrames( m_bufferSize );
	}

	m_bufferStates = new BufferState[m_size];

	m_readerSem.acquire( _size );
}



AudioOutputContext::BufferFifo::~BufferFifo()
{
	for( int i = 0; i < m_size; ++i )
	{
		CPU::freeFrames( m_buffers[i] );
	}

	delete[] m_buffers;
	delete[] m_bufferStates;

	m_readerSem.release( m_size );
}




void AudioOutputContext::BufferFifo::write( sampleFrameA * _buffer )
{
	m_writerSem.acquire();

	if( _buffer != NULL )
	{
		CPU::memCpy( m_buffers[m_writerIndex], _buffer,
						m_bufferSize * sizeof( sampleFrameA ) );
		m_bufferStates[m_writerIndex] = Running;
	}
	else
	{
		m_bufferStates[m_writerIndex] = NullBuffer;
	}

	m_writerIndex = ( m_writerIndex + 1 ) % m_size;

	m_readerSem.release();
}




void AudioOutputContext::BufferFifo::startRead()
{
	m_readerSem.acquire();
}




void AudioOutputContext::BufferFifo::finishRead()
{
	m_readerIndex = ( m_readerIndex + 1 ) % m_size;
	m_writerSem.release();
}







AudioOutputContext::AudioOutputContext( Mixer * mixer,
						AudioBackend * audioBackend,
						const QualitySettings & qualitySettings ) :
	m_mixer( mixer ),
	m_qualitySettings( qualitySettings ),
	m_audioBackend( audioBackend ),
	m_fifo( NULL ),
	m_fifoWriter( NULL )
{
	int error;
	if( ( m_srcState = src_new(
			qualitySettings.libsrcInterpolation(),
				SURROUND_CHANNELS, &error ) ) == NULL )
	{
		qWarning( "src_new() failed in AudioOutputContext::AudioOutputContext()" );
	}
	//m_audioBackend->applyQualitySettings();

	int framesPerPeriod = m_mixer->framesPerPeriod();

	// just rendering?
	if( !engine::hasGUI() )
	{
		m_fifo = new BufferFifo( 1, framesPerPeriod );
	}
	else if( configManager::inst()->value( "mixer", "framesperaudiobuffer"
						).toInt() >= 32 )
	{
		framesPerPeriod =
			(fpp_t) configManager::inst()->value( "mixer",
					"framesperaudiobuffer" ).toInt();

		if( framesPerPeriod > DEFAULT_BUFFER_SIZE )
		{
			m_fifo = new BufferFifo( framesPerPeriod / DEFAULT_BUFFER_SIZE,
														DEFAULT_BUFFER_SIZE );
		}
		else
		{
			m_fifo = new BufferFifo( 1, framesPerPeriod );
		}
	}
	else
	{
		configManager::inst()->setValue( "mixer",
							"framesperaudiobuffer",
				QString::number( framesPerPeriod ) );
		m_fifo = new BufferFifo( 1, framesPerPeriod );
	}

}




AudioOutputContext::~AudioOutputContext()
{
	while( m_fifo->isEmpty() == false )
	{
		m_fifo->startRead();
		m_fifo->finishRead();
	}
	delete m_fifo;

	src_delete( m_srcState );
}




void AudioOutputContext::startProcessing()
{
	if( !isProcessing() )
	{
		m_fifoWriter = new FifoWriter( this );
		m_fifoWriter->start( QThread::HighPriority );

		m_audioBackend->startProcessing();
	}
}




void AudioOutputContext::stopProcessing()
{
	if( isProcessing() )
	{
		m_fifoWriter->finish();
		m_audioBackend->stopProcessing();
		m_fifoWriter->wait();

		delete m_fifoWriter;
		m_fifoWriter = NULL;
	}
}




bool AudioOutputContext::isProcessing() const
{
	return m_fifoWriter && m_fifoWriter->isRunning();
}




int AudioOutputContext::getCurrentOutputBuffer( sampleFrameA * _destBuf,
										sample_rate_t _destSampleRate )
{
	int frames = mixer()->framesPerPeriod();
	m_fifo->startRead();
	if( m_fifo->currentReadBufferState() == BufferFifo::NullBuffer )
	{
		m_fifo->finishRead();
		return 0;
	}
	sampleFrameA * srcBuf = m_fifo->currentReadBuffer();

	if( mixer()->processingSampleRate() != _destSampleRate )
	{
		if( m_srcState == NULL )
		{
			m_fifo->finishRead();
			return 0;
		}
		m_srcData.input_frames = frames;
		m_srcData.output_frames = frames;
		m_srcData.data_in = (float *) srcBuf;
		m_srcData.data_out = (float *) _destBuf;
		m_srcData.src_ratio = (double) _destSampleRate /
												mixer()->processingSampleRate();
		m_srcData.end_of_input = 0;
		int error;
		if( ( error = src_process( m_srcState, &m_srcData ) ) )
		{
			qWarning( "AudioBackend::resample(): error while resampling: %s",
							src_strerror( error ) );
		}
		frames = frames * _destSampleRate / mixer()->processingSampleRate();
	}
	else
	{
		CPU::memCpy( _destBuf, srcBuf, frames * sizeof( sampleFrameA ) );
	}

	// tell BufferFifo to release current read buffer
	m_fifo->finishRead();

	return frames;
}






AudioOutputContext::FifoWriter::FifoWriter( AudioOutputContext * context ) :
	m_context( context ),
	m_writing( true )
{
}




void AudioOutputContext::FifoWriter::finish()
{
	m_writing = false;
}




void AudioOutputContext::FifoWriter::run()
{
#if 0
#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_PTHREAD_H
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	pthread_setaffinity_np( pthread_self(), sizeof( mask ), &mask );
#endif
#endif
#endif

	while( m_writing )
	{
		m_context->fifo()->write( m_context->mixer()->renderNextBuffer() );
	}

	// write a NULL in order to signal the AudioBackend that the FifoWriter has
	// finished
	m_context->fifo()->write( NULL );
}



