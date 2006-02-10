/*
 * audio_sample_recorder.cpp - device-class that implements recording
 *                             surround-audio-buffers into RAM, maybe later
 *                             also harddisk
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */



#include "audio_sample_recorder.h"
#include "sample_buffer.h"
#include "buffer_allocator.h"
#include "debug.h"



audioSampleRecorder::audioSampleRecorder( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
							bool & _success_ful,
							mixer * _mixer ) :
	audioDevice( _sample_rate, _channels, _mixer ),
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




f_cnt_t audioSampleRecorder::framesRecorded( void ) const
{
	f_cnt_t frames = 0;
	for( bufferList::const_iterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		frames += ( *it ).second;
	}
	return( frames );
}




void audioSampleRecorder::createSampleBuffer( sampleBuffer * * _sample_buf )
{
	const f_cnt_t frames = framesRecorded();
	// create buffer to store all recorded buffers in
	sampleFrame * data = bufferAllocator::alloc<sampleFrame>( frames );
	// make sure buffer is cleaned up properly at the end...
	bufferAllocator::autoCleaner<sampleFrame> ac( data );

#ifdef LMMS_DEBUG
	assert( data != NULL );
#endif
	// now copy all buffers into big buffer
	for( bufferList::const_iterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		memcpy( data, ( *it ).first, ( *it ).second *
							sizeof( sampleFrame ) );
		data += ( *it ).second;
	}
	// create according sample-buffer out of big buffer
	*_sample_buf = new sampleBuffer( ac.ptr(), frames, getMixer()->eng() );
}




void audioSampleRecorder::writeBuffer( const surroundSampleFrame * _ab,
					const fpab_t _frames, const float )
{
	sampleFrame * buf = bufferAllocator::alloc<sampleFrame>( _frames );
	for( fpab_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}


