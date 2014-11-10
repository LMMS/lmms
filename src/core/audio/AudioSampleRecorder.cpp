/*
 * AudioSampleRecorder.cpp - device-class that implements recording
 *                           audio-buffers into RAM
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioSampleRecorder.h"
#include "SampleBuffer.h"
#include "debug.h"



AudioSampleRecorder::AudioSampleRecorder( const ch_cnt_t _channels,
							bool & _success_ful,
							Mixer * _mixer ) :
	AudioDevice( _channels, _mixer ),
	m_buffers()
{
	_success_ful = true;
}




AudioSampleRecorder::~AudioSampleRecorder()
{
	while( !m_buffers.empty() )
	{
		delete[] m_buffers.front().first;
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




void AudioSampleRecorder::createSampleBuffer( SampleBuffer** sampleBuf )
{
	const f_cnt_t frames = framesRecorded();
	// create buffer to store all recorded buffers in
	sampleFrame * data = new sampleFrame[frames];
	// make sure buffer is cleaned up properly at the end...
	sampleFrame * data_ptr = data;

#ifdef LMMS_DEBUG
	assert( data != NULL );
#endif
	// now copy all buffers into big buffer
	for( BufferList::ConstIterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		memcpy( data_ptr, ( *it ).first, ( *it ).second *
							sizeof( sampleFrame ) );
		data_ptr += ( *it ).second;
	}
	// create according sample-buffer out of big buffer
	*sampleBuf = new SampleBuffer( data, frames );
	( *sampleBuf )->setSampleRate( sampleRate() );
	delete[] data;
}




void AudioSampleRecorder::writeBuffer( const surroundSampleFrame * _ab,
					const fpp_t _frames, const float )
{
	sampleFrame * buf = new sampleFrame[_frames];
	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}



