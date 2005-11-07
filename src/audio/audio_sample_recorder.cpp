/*
 * audio_sample_recorder.cpp - device-class that implements recording
 *                             surround-audio-buffers into RAM, maybe later
 *                             also harddisk
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */



#include "audio_sample_recorder.h"
#include "sample_buffer.h"
#include "buffer_allocator.h"
#include "debug.h"



audioSampleRecorder::audioSampleRecorder( Uint32 _sample_rate, Uint32 _channels,
							bool & _success_ful ) :
	audioDevice( _sample_rate, _channels ),
	m_buffers()
{
	_success_ful = TRUE;
}




audioSampleRecorder::~audioSampleRecorder()
{
	while( !m_buffers.empty() )
	{
		bufferAllocator::free( m_buffers.front().first );
		m_buffers.erase( m_buffers.begin() );
	}
}




Uint32 audioSampleRecorder::framesRecorded( void ) const
{
	Uint32 frames = 0;
	for( bufferVector::const_iterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		frames += it->second;
	}
	return( frames );
}




void audioSampleRecorder::createSampleBuffer( sampleBuffer * * _sample_buf )
									const
{
	Uint32 frames = framesRecorded();
	// create buffer to store all recorded buffers in
	sampleFrame * data = bufferAllocator::alloc<sampleFrame>( frames );
	// make sure buffer is cleaned up properly at the end...
	bufferAllocator::autoCleaner<sampleFrame> ac( data );

#ifdef LMMS_DEBUG
	assert( data != NULL );
#endif
	// now copy all buffers into big buffer
	for( bufferVector::const_iterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		memcpy( data, it->first, it->second * sizeof( sampleFrame ) );
		data += it->second;
	}
	// create according sample-buffer out of big buffer
	*_sample_buf = new sampleBuffer( ac.ptr(), frames );
}




void audioSampleRecorder::writeBufferToDev( surroundSampleFrame * _ab,
							Uint32 _frames, float )
{
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( _frames );
	for( Uint32 frame = 0; frame < _frames; ++frame )
	{
		for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}


