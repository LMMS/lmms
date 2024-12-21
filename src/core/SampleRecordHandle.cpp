/*
 * SampleRecordHandle.cpp - implementation of class SampleRecordHandle
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#include "SampleRecordHandle.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "PatternTrack.h"
#include "SampleBuffer.h"
#include "SampleClip.h"
#include "debug.h"


namespace lmms
{


SampleRecordHandle::SampleRecordHandle( SampleClip* clip ) :
	PlayHandle( Type::SamplePlayHandle ),
	m_framesRecorded( 0 ),
	m_minLength( clip->length() ),
	m_track( clip->getTrack() ),
	m_patternTrack( nullptr ),
	m_clip( clip )
{
}




SampleRecordHandle::~SampleRecordHandle()
{
	if (!m_buffers.empty()) { m_clip->setSampleBuffer(createSampleBuffer()); }

	while( !m_buffers.empty() )
	{
		delete[] m_buffers.front().first;
		m_buffers.erase( m_buffers.begin() );
	}
	m_clip->setRecord( false );
}




void SampleRecordHandle::play(CoreAudioDataMut /*buffer*/)
{
	const SampleFrame* recbuf = Engine::audioEngine()->inputBuffer();
	const f_cnt_t frames = Engine::audioEngine()->inputBufferFrames();
	writeBuffer( recbuf, frames );
	m_framesRecorded += frames;

	TimePos len = (tick_t)( m_framesRecorded / Engine::framesPerTick() );
	if( len > m_minLength )
	{
//		m_clip->changeLength( len );
		m_minLength = len;
	}
}




bool SampleRecordHandle::isFinished() const
{
	return false;
}




bool SampleRecordHandle::isFromTrack( const Track * _track ) const
{
	return (m_track == _track || m_patternTrack == _track);
}




f_cnt_t SampleRecordHandle::framesRecorded() const
{
	return( m_framesRecorded );
}




std::shared_ptr<const SampleBuffer> SampleRecordHandle::createSampleBuffer()
{
	const f_cnt_t frames = framesRecorded();
	// create buffer to store all recorded buffers in
	auto bigBuffer = std::vector<SampleFrame>(frames);

	// now copy all buffers into big buffer
	auto framesCopied = 0;
	for (const auto& [buf, numFrames] : m_buffers)
	{
		std::copy_n(buf, numFrames, bigBuffer.begin() + framesCopied);
		framesCopied += numFrames;
	}

	// create according sample-buffer out of big buffer
	return std::make_shared<const SampleBuffer>(std::move(bigBuffer), Engine::audioEngine()->inputSampleRate());
}




void SampleRecordHandle::writeBuffer( const SampleFrame* _ab, const f_cnt_t _frames )
{
	auto buf = new SampleFrame[_frames];
	for( f_cnt_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}


} // namespace lmms
