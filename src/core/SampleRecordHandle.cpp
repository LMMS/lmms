/*
 * SampleRecordHandle.cpp - implementation of class SampleRecordHandle
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#include "SampleRecordHandle.h"
#include "bb_track.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "Pattern.h"
#include "SampleBuffer.h"
#include "SampleTrack.h"



SampleRecordHandle::SampleRecordHandle( SampleTCO* tco ) :
	PlayHandle( TypeSamplePlayHandle ),
	m_framesRecorded( 0 ),
	m_minLength( tco->length() ),
	m_track( tco->getTrack() ),
	m_bbTrack( NULL ),
	m_tco( tco )
{
}




SampleRecordHandle::~SampleRecordHandle()
{
	if( !m_buffers.empty() )
	{
		SampleBuffer* sb;
		createSampleBuffer( &sb );
		m_tco->setSampleBuffer( sb );
	}
	
	while( !m_buffers.empty() )
	{
		delete[] m_buffers.front().first;
		m_buffers.erase( m_buffers.begin() );
	}
	m_tco->setRecord( false );
}




void SampleRecordHandle::play( sampleFrame * /*_working_buffer*/ )
{
	const sampleFrame * recbuf = engine::mixer()->inputBuffer();
	const f_cnt_t frames = engine::mixer()->inputBufferFrames();
	writeBuffer( recbuf, frames );
	m_framesRecorded += frames;

	MidiTime len = (tick_t)( m_framesRecorded / engine::framesPerTick() );
	if( len > m_minLength )
	{
//		m_tco->changeLength( len );
		m_minLength = len;
	}
}




bool SampleRecordHandle::isFinished() const
{
	return false;
}




bool SampleRecordHandle::isFromTrack( const track * _track ) const
{
	return( m_track == _track || m_bbTrack == _track );
}




f_cnt_t SampleRecordHandle::framesRecorded() const
{
	return( m_framesRecorded );
}




void SampleRecordHandle::createSampleBuffer( SampleBuffer** sampleBuf )
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
	for( bufferList::const_iterator it = m_buffers.begin();
						it != m_buffers.end(); ++it )
	{
		memcpy( data_ptr, ( *it ).first, ( *it ).second *
							sizeof( sampleFrame ) );
		data_ptr += ( *it ).second;
	}
	// create according sample-buffer out of big buffer
	*sampleBuf = new SampleBuffer( data, frames );
	( *sampleBuf)->setSampleRate( engine::mixer()->inputSampleRate() );
	delete[] data;
}




void SampleRecordHandle::writeBuffer( const sampleFrame * _ab,
					const f_cnt_t _frames )
{
	sampleFrame * buf = new sampleFrame[_frames];
	for( f_cnt_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}



