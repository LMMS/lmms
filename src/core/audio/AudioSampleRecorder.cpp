/*
 * AudioSampleRecorder.cpp - device-class that implements recording
 *                           audio-buffers into RAM
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioSampleRecorder.h"
#include "sample_buffer.h"
#include "Cpu.h"



AudioSampleRecorder::AudioSampleRecorder( const ch_cnt_t _channels,
							bool & _success_ful,
							AudioOutputContext * context ) :
	AudioBackend( _channels, context ),
	m_buffers()
{
	_success_ful = true;
}




AudioSampleRecorder::~AudioSampleRecorder()
{
	while( !m_buffers.empty() )
	{
		CPU::freeFrames( m_buffers.front().first );
		m_buffers.erase( m_buffers.begin() );
	}
}




f_cnt_t AudioSampleRecorder::framesRecorded() const
{
	f_cnt_t frames = 0;
	for( BufferList::ConstIterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		frames += ( *it ).second;
	}
	return frames;
}




void AudioSampleRecorder::createSampleBuffer( sampleBuffer * * _sample_buf )
{
	const f_cnt_t frames = framesRecorded();
	// create buffer to store all recorded buffers in
	sampleFrameA * data = CPU::allocFrames( frames );
	// make sure buffer is cleaned up properly at the end...
	sampleFrameA * data_ptr = data;

#ifdef LMMS_DEBUG
	assert( data != NULL );
#endif
	// now copy all buffers into big buffer
	for( BufferList::ConstIterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		CPU::memCpy( data_ptr, ( *it ).first, ( *it ).second *
							sizeof( sampleFrameA ) );
		data_ptr += ( *it ).second;
	}
	// create according sample-buffer out of big buffer
	*_sample_buf = new sampleBuffer( data, frames );
	( *_sample_buf )->setSampleRate( sampleRate() );
	CPU::freeFrames( data );
}




void AudioSampleRecorder::writeBuffer( const sampleFrameA * srcBuf,
					const fpp_t _frames, const float )
{
	sampleFrameA * buf = CPU::allocFrames( _frames );
	CPU::memCpy( buf, srcBuf, _frames*sizeof( sampleFrameA ) );
	m_buffers.push_back( qMakePair( buf, _frames ) );
}



