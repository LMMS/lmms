/*
 * sample_record_handle.cpp - implementation of class sampleRecordHandle
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#include "sample_record_handle.h"
#include "bb_track.h"
#include "engine.h"
#include "InstrumentTrack.h"
#include "pattern.h"
#include "sample_buffer.h"
#include "sample_track.h"



sampleRecordHandle::sampleRecordHandle( sampleTCO * _tco ) :
	playHandle( SamplePlayHandle ),
	m_framesRecorded( 0 ),
	m_minLength( _tco->length() ),
	m_track( _tco->getTrack() ),
	m_bbTrack( NULL ),
	m_tco( _tco )
{
}




sampleRecordHandle::~sampleRecordHandle()
{
	if( !m_buffers.empty() )
	{
		sampleBuffer * sb;
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




void sampleRecordHandle::play( sampleFrame * /*_working_buffer*/ )
{
	const sampleFrame * recbuf = engine::getMixer()->inputBuffer();
	const f_cnt_t frames = engine::getMixer()->inputBufferFrames();
	writeBuffer( recbuf, frames );
	m_framesRecorded += frames;

	midiTime len = (tick_t)( m_framesRecorded / engine::framesPerTick() );
	if( len > m_minLength )
	{
//		m_tco->changeLength( len );
		m_minLength = len;
	}
}




bool sampleRecordHandle::done() const
{
	return false;
}




bool sampleRecordHandle::isFromTrack( const track * _track ) const
{
	return( m_track == _track || m_bbTrack == _track );
}




f_cnt_t sampleRecordHandle::framesRecorded() const
{
	return( m_framesRecorded );
}




void sampleRecordHandle::createSampleBuffer( sampleBuffer * * _sample_buf )
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
	*_sample_buf = new sampleBuffer( data, frames );
	( *_sample_buf )->setSampleRate( engine::getMixer()->inputSampleRate() );
	delete[] data;
}




void sampleRecordHandle::writeBuffer( const sampleFrame * _ab,
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



