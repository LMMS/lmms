/*
 * AudioFileWave.cpp - audio-device which encodes wave-stream and writes it
 *                     into a WAVE-file. This is used for song-export.
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioFileWave.h"

#include <memory>

#include "AudioEngine.h"
#include "endian_handling.h"

namespace lmms
{

AudioFileWave::AudioFileWave( OutputSettings const & outputSettings,
				const ch_cnt_t channels, bool & successful,
				const QString & file,
				AudioEngine* audioEngine ) :
	AudioFileDevice( outputSettings, channels, file, audioEngine ),
	m_sf( nullptr )
{
	successful = outputFileOpened() && startEncoding();
}




AudioFileWave::~AudioFileWave()
{
	finishEncoding();
}




bool AudioFileWave::startEncoding()
{
	m_si.samplerate = sampleRate();
	m_si.channels = channels();
	m_si.frames = audioEngine()->framesPerPeriod();
	m_si.sections = 1;
	m_si.seekable = 0;

	m_si.format = SF_FORMAT_WAV;

	switch( getOutputSettings().getBitDepth() )
	{
	case OutputSettings::BitDepth::Depth32Bit:
		m_si.format |= SF_FORMAT_FLOAT;
		break;
	case OutputSettings::BitDepth::Depth24Bit:
		m_si.format |= SF_FORMAT_PCM_24;
		break;
	case OutputSettings::BitDepth::Depth16Bit:
	default:
		m_si.format |= SF_FORMAT_PCM_16;
		break;
	}

	// Use file handle to handle unicode file name on Windows
	m_sf = sf_open_fd( outputFileHandle(), SFM_WRITE, &m_si, false );

	if (!m_sf)
	{
		qWarning("Error: AudioFileWave::startEncoding: %s", sf_strerror(nullptr));
		return false;
	}

	// Prevent fold overs when encountering clipped data
	sf_command(m_sf, SFC_SET_CLIPPING, nullptr, SF_TRUE);

	sf_set_string ( m_sf, SF_STR_SOFTWARE, "LMMS" );

	return true;
}

void AudioFileWave::writeBuffer(PlanarBufferView<const float> buffer)
{
	const auto frames = static_cast<sf_count_t>(buffer.frames());
	const auto bitDepth = getOutputSettings().getBitDepth();

	if (bitDepth == OutputSettings::BitDepth::Depth32Bit || bitDepth == OutputSettings::BitDepth::Depth24Bit)
	{
		auto interleaved = std::make_unique_for_overwrite<float[]>(buffer.frames() * channels());
		toInterleaved(buffer, InterleavedBufferSpan{interleaved.get(), channels(), buffer.frames()});

		sf_writef_float(m_sf, interleaved.get(), frames);
	}
	else
	{
		auto interleaved = std::make_unique_for_overwrite<int_sample_t[]>(buffer.frames() * channels());
		convertToS16(buffer, interleaved.get(), isBigEndian());

		sf_writef_short(m_sf, interleaved.get(), frames);
	}
}




void AudioFileWave::finishEncoding()
{
	if( m_sf )
	{
		sf_close( m_sf );
	}
}

} // namespace lmms