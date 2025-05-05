/*
 * AudioSampleRecorder.cpp - device-class that implements recording
 *                           audio-buffers into RAM
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioSampleRecorder.h"
#include "SampleBuffer.h"


namespace lmms
{


AudioSampleRecorder::AudioSampleRecorder( const ch_cnt_t _channels,
							bool & _success_ful,
							AudioEngine * _audioEngine ) :
	AudioDevice( _channels, _audioEngine ),
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
	for (const auto& buffer : m_buffers)
	{
		frames += buffer.second;
	}
	return frames;
}

std::shared_ptr<const SampleBuffer> AudioSampleRecorder::createSampleBuffer()
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
	return std::make_shared<const SampleBuffer>(std::move(bigBuffer), sampleRate());
}

void AudioSampleRecorder::writeBuffer(const SampleFrame* _ab, const fpp_t _frames)
{
	auto buf = new SampleFrame[_frames];
	for( fpp_t frame = 0; frame < _frames; ++frame )
	{
		for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
		{
			buf[frame][chnl] = _ab[frame][chnl];
		}
	}
	m_buffers.push_back( qMakePair( buf, _frames ) );
}


} // namespace lmms
