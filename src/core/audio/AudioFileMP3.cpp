/*
 * AudioFileMP3.cpp - Audio-device which encodes a wave stream into
 *                    an MP3 file. This is used for song export.
 *
 * Copyright (c) 2017 to present Michael Gregorius <michael.gregorius.git/at/arcor[dot]de>
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

#include "AudioFileMP3.h"

#ifdef LMMS_HAVE_MP3LAME

#include "Mixer.h"

#include <cassert>


AudioFileMP3::AudioFileMP3(	OutputSettings const & outputSettings,
				const ch_cnt_t channels,
				bool & successful,
				const QString & file,
				Mixer* mixer ) :
	AudioFileDevice( outputSettings, channels, file, mixer )
{
	successful = true;
	// For now only accept stereo sources
	successful &= channels == 2;
	successful &= initEncoder();
	successful &= outputFileOpened();
}

AudioFileMP3::~AudioFileMP3()
{
	flushRemainingBuffers();
	tearDownEncoder();
}

void AudioFileMP3::writeBuffer( const surroundSampleFrame * _buf,
					const fpp_t _frames,
					const float _master_gain )
{
	if (_frames < 1)
	{
		return;
	}

	// TODO Why isn't the gain applied by the driver but inside the device?
	std::vector<float> interleavedDataBuffer(_frames * 2);
	for (fpp_t i = 0; i < _frames; ++i)
	{
		interleavedDataBuffer[2*i] = _buf[i][0] * _master_gain;
		interleavedDataBuffer[2*i + 1] = _buf[i][1] * _master_gain;
	}

	size_t minimumBufferSize = 1.25 * _frames + 7200;
	std::vector<unsigned char> encodingBuffer(minimumBufferSize);

	int bytesWritten = lame_encode_buffer_interleaved_ieee_float(m_lame, &interleavedDataBuffer[0], _frames, &encodingBuffer[0], static_cast<int>(encodingBuffer.size()));
	assert (bytesWritten >= 0);

	writeData(&encodingBuffer[0], bytesWritten);
}

void AudioFileMP3::flushRemainingBuffers()
{
	// The documentation states that flush should have at least 7200 bytes. So let's be generous.
	std::vector<unsigned char> encodingBuffer(7200 * 4);

	int bytesWritten = lame_encode_flush(m_lame, &encodingBuffer[0], static_cast<int>(encodingBuffer.size()));
	assert (bytesWritten >= 0);

	writeData(&encodingBuffer[0], bytesWritten);
}

MPEG_mode mapToMPEG_mode(OutputSettings::StereoMode stereoMode)
{
	switch (stereoMode)
	{
	case OutputSettings::StereoMode_Stereo:
		return STEREO;
	case OutputSettings::StereoMode_JointStereo:
		return JOINT_STEREO;
	case OutputSettings::StereoMode_Mono:
		return MONO;
	default:
		return NOT_SET;
	}
}

bool AudioFileMP3::initEncoder()
{
	m_lame = lame_init();

	// Handle stereo/joint/mono settings
	OutputSettings::StereoMode stereoMode = getOutputSettings().getStereoMode();
	lame_set_mode(m_lame, mapToMPEG_mode(stereoMode));

	// Handle bit rate settings
	OutputSettings::BitRateSettings bitRateSettings = getOutputSettings().getBitRateSettings();
	int bitRate = static_cast<int>(bitRateSettings.getBitRate());

	lame_set_VBR(m_lame, vbr_off);
	lame_set_brate(m_lame, bitRate);

	// Add a comment
	id3tag_set_comment(m_lame, "Created with LMMS");

	return lame_init_params(m_lame) != -1;
}

void AudioFileMP3::tearDownEncoder()
{
	lame_close(m_lame);
}

#endif
