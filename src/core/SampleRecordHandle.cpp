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
	m_track( tco->getTrack() ),
	m_bbTrack( NULL ),
	m_tco( tco ),
	m_recordingChannel{dynamic_cast<SampleTrack*>(m_track)->recordingChannel ()},
	m_startRecordTimeOffset{startRecordTimeOffset}
{
	m_tco->setIsRecording(true);
}




SampleRecordHandle::~SampleRecordHandle()
{	
	// No problem to block here.
	// this is not a renderer thread anymore
	// so it will not cause a deadlock
	// with requestChangesInModel.

	// Wait for the current async rendering job
	// to finish.
	m_lastAsyncWork.waitForFinished();

	if (! m_currentBuffer.empty()) {
		// We have data that has not been written into the buffer.
		// force-write it into the buffer.

		// Add a new rendering job and wait for it to finish.
		addOrCreateBuffer();
		m_lastAsyncWork.waitForFinished();
	}

	m_tco->updateLength ();

	// If this is an automatically created tco,
	// enable resizing.
	if (m_framesRecorded != 0) {
		m_tco->setAutoResize (false);
		m_tco->setRecord( false );
	}

	m_tco->setIsRecording(false);
}




void SampleRecordHandle::play( sampleFrame * /*_working_buffer*/ )
{
	const sampleFrame * recbuf = Engine::mixer()->inputBuffer();
	const f_cnt_t frames = Engine::mixer()->inputBufferFrames();

	writeBuffer( recbuf, frames );

	// Add data to the buffer.
	if (m_lastAsyncWork.isFinished())
		addOrCreateBuffer();

	m_framesRecorded += frames;
	m_timeRecorded = m_framesRecorded / Engine::framesPerTick (Engine::mixer ()->inputSampleRate ());
	m_currentBuffer.clear();
}




bool SampleRecordHandle::isFinished() const
{
	return !m_tco->getAutoResize () && (m_startRecordTimeOffset + m_timeRecorded) >= m_tco->length ();
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

void SampleRecordHandle::addOrCreateBuffer() {
	auto sampleBuffer = m_tco->sampleBuffer ();
	auto currentBufferCopy = m_currentBuffer;
	m_lastAsyncWork = runAsync(std::bind([this, currentBufferCopy, sampleBuffer] () mutable{
		if (m_framesRecorded == 0) {
			// Protect m_tco->setStartTimeOffset;
			auto guard = Engine::mixer()->requestChangesGuard();
			// Make sure we don't have the previous data.
			sampleBuffer->resetData(std::move (currentBufferCopy),
														 Engine::mixer ()->inputSampleRate ());
			m_tco->setStartTimeOffset (m_startRecordTimeOffset);
		} else {
			if (! currentBufferCopy.empty ()) {
				sampleBuffer->addData(currentBufferCopy,
										Engine::mixer ()->inputSampleRate ());
			}
		}
	}));

}




void SampleRecordHandle::writeBuffer( const sampleFrame * _ab,
					const f_cnt_t _frames )
{
	auto framesInBuffer = m_currentBuffer.size();
	// Add _frames elements to the buffer.
	m_currentBuffer.resize (framesInBuffer + _frames);

	// Depending on the recording channel, copy the buffer as a
	// mono-right, mono-left or stereo.

	// Note that mono doesn't mean single channel, it means
	// empty other channel. Therefore, we would just duplicate
	// every frame from the mono channel
	// to the empty channel.

	switch(m_recordingChannel) {
	case SampleTrack::RecordingChannel::MonoLeft:
		copyBufferFromMonoLeft(_ab, m_currentBuffer.data () + framesInBuffer, _frames);
		break;
	case SampleTrack::RecordingChannel::MonoRight:
		copyBufferFromMonoRight(_ab, m_currentBuffer.data () + framesInBuffer, _frames);
		break;
	case SampleTrack::RecordingChannel::Stereo:
		copyBufferFromStereo(_ab, m_currentBuffer.data () + framesInBuffer, _frames);
		break;
	default:
		Q_ASSERT(false);
		break;
	}
}



