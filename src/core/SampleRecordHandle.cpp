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
#include "BBTrack.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Mixer.h"
#include "SampleBuffer.h"
#include "debug.h"


SampleRecordHandle::SampleRecordHandle(SampleTCO* tco , MidiTime startRecordTimeOffset) :
	PlayHandle( TypeSamplePlayHandle ),
	m_framesRecorded( 0 ),
	m_minLength( tco->length() ),
	m_track( tco->getTrack() ),
	m_bbTrack( NULL ),
	m_tco( tco ),
	m_recordingChannel{dynamic_cast<SampleTrack*>(m_track)->recordingChannel ()},
	m_startRecordTimeOffset{startRecordTimeOffset}
{
}




SampleRecordHandle::~SampleRecordHandle()
{
	m_tco->updateLength ();

	// If this is an automatically created tco,
	// enable resizing.
	m_tco->setAutoResize (false);
	m_tco->setRecord( false );
}




void SampleRecordHandle::play( sampleFrame * /*_working_buffer*/ )
{
	const sampleFrame * recbuf = Engine::mixer()->inputBuffer();
	const f_cnt_t frames = Engine::mixer()->inputBufferFrames();
	m_currentBuffer.clear ();
	writeBuffer( recbuf, frames );

	// It is the first buffer.
	if (m_framesRecorded == 0) {
		// Make sure we don't have the previous data.
		m_tco->sampleBuffer ()->resetData (std::move (m_currentBuffer),
										   Engine::mixer ()->inputSampleRate (),
										   false);
		m_tco->setStartTimeOffset (m_startRecordTimeOffset);
	} else {
		if (! m_currentBuffer.empty ()) {
			m_tco->sampleBuffer ()->addData (m_currentBuffer,
											 Engine::mixer ()->inputSampleRate (),
											 false);
		}
	}

	m_framesRecorded += frames;

	MidiTime len = (tick_t)( m_framesRecorded / Engine::framesPerTick() );
	if( len > m_minLength )
	{
		m_minLength = len;
	}
}




bool SampleRecordHandle::isFinished() const
{
	return false;
}




bool SampleRecordHandle::isFromTrack( const Track * _track ) const
{
	return( m_track == _track || m_bbTrack == _track );
}




f_cnt_t SampleRecordHandle::framesRecorded() const
{
	return( m_framesRecorded );
}

void SampleRecordHandle::copyBufferFromMonoLeft(const sampleFrame *inputBuffer,
												sampleFrame *outputBuffer,
												const f_cnt_t _frames)
{
	for( f_cnt_t frame = 0; frame < _frames; ++frame ) {
		// Copy every first sample to the first and the second in the output buffer.
		outputBuffer[frame][LEFT_CHANNEL_INDEX]  = inputBuffer[frame][LEFT_CHANNEL_INDEX];
		outputBuffer[frame][RIGHT_CHANNEL_INDEX] = inputBuffer[frame][LEFT_CHANNEL_INDEX];
	}
}

void SampleRecordHandle::copyBufferFromMonoRight(const sampleFrame *inputBuffer,
												 sampleFrame *outputBuffer,
												 const f_cnt_t _frames)
{
	for( f_cnt_t frame = 0; frame < _frames; ++frame ) {
		// Copy every second sample to the first and the second in the output buffer.
		outputBuffer[frame][LEFT_CHANNEL_INDEX]  = inputBuffer[frame][RIGHT_CHANNEL_INDEX];
		outputBuffer[frame][RIGHT_CHANNEL_INDEX] = inputBuffer[frame][RIGHT_CHANNEL_INDEX];
	}
}

void SampleRecordHandle::copyBufferFromStereo(const sampleFrame *inputBuffer,
											  sampleFrame *outputBuffer,
											  const f_cnt_t _frames)
{
	for( f_cnt_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			outputBuffer[frame][chnl] = inputBuffer[frame][chnl];
		}
	}
}




void SampleRecordHandle::writeBuffer( const sampleFrame * _ab,
					const f_cnt_t _frames )
{
	m_currentBuffer.resize (_frames);

	// Depending on the recording channel, copy the buffer as a
	// mono-right, mono-left or stereo.

	// Note that mono doesn't mean single channel, it means
	// empty other channel. Therefore, we would just duplicate
	// every frame from the mono channel
	// to the empty channel.

	switch(m_recordingChannel) {
	case SampleTrack::RecordingChannel::MonoLeft:
		copyBufferFromMonoLeft(_ab, m_currentBuffer.data (), _frames);
		break;
	case SampleTrack::RecordingChannel::MonoRight:
		copyBufferFromMonoRight(_ab, m_currentBuffer.data (), _frames);
		break;
	case SampleTrack::RecordingChannel::Stereo:
		copyBufferFromStereo(_ab, m_currentBuffer.data (), _frames);
		break;
	default:
		Q_ASSERT(false);
		break;
	}
}



