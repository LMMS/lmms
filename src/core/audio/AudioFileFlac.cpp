/*
 * AudioFileFlac.cpp - Audio device which encodes a wave stream into a FLAC file (Implementation).
 *
 * Copyright (c) 2017 to present Levin Oehlmann <irrenhaus3/at/gmail[dot]com> et al.
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


#include <cmath>

#include "AudioFileFlac.h"
#include "endian_handling.h"
#include "AudioEngine.h"

namespace lmms
{

AudioFileFlac::AudioFileFlac(OutputSettings const& outputSettings, ch_cnt_t const channels, bool& successful, QString const& file, AudioEngine* audioEngine):
	AudioFileDevice(outputSettings,channels,file,audioEngine),
	m_sf(nullptr)
{
	successful = outputFileOpened() && startEncoding();
}

AudioFileFlac::~AudioFileFlac()
{
	finishEncoding();
}

bool AudioFileFlac::startEncoding()
{
	m_sfinfo.samplerate=sampleRate();
	m_sfinfo.channels=channels();
	m_sfinfo.frames = audioEngine()->framesPerPeriod();
	m_sfinfo.sections=1;
	m_sfinfo.seekable=0;

	m_sfinfo.format = SF_FORMAT_FLAC;

	switch (getOutputSettings().getBitDepth())
	{
		case OutputSettings::BitDepth::Depth24Bit:
		case OutputSettings::BitDepth::Depth32Bit:
			// FLAC does not support 32bit sampling, so take it as 24.
			m_sfinfo.format |= SF_FORMAT_PCM_24;
			break;
		default:
			m_sfinfo.format |= SF_FORMAT_PCM_16;
	}

#ifdef LMMS_HAVE_SF_COMPLEVEL
	double compression = getOutputSettings().getCompressionLevel();
	sf_command(m_sf, SFC_SET_COMPRESSION_LEVEL, &compression, sizeof(double));
#endif

	m_sf = sf_open(
#ifdef LMMS_BUILD_WIN32
		outputFile().toLocal8Bit().constData(),
#else
		outputFile().toUtf8().constData(),
#endif
		SFM_WRITE,
		&m_sfinfo
	);

	sf_command(m_sf, SFC_SET_CLIPPING, nullptr, SF_TRUE);

	sf_set_string(m_sf, SF_STR_SOFTWARE, "LMMS");

	return true;
}

void AudioFileFlac::writeBuffer(const SampleFrame* _ab, fpp_t const frames)
{
	OutputSettings::BitDepth depth = getOutputSettings().getBitDepth();
	float clipvalue = std::nextafterf( -1.0f, 0.0f );

	if (depth == OutputSettings::BitDepth::Depth24Bit || depth == OutputSettings::BitDepth::Depth32Bit) // Float encoding
	{
		auto buf = std::vector<sample_t>(frames * channels());
		for(fpp_t frame = 0; frame < frames; ++frame)
		{
			for(ch_cnt_t channel=0; channel<channels(); ++channel)
			{
				// Clip the negative side to just above -1.0 in order to prevent it from changing sign
				// Upstream issue: https://github.com/erikd/libsndfile/issues/309
				// When this commit is reverted libsndfile-1.0.29 must be made a requirement for FLAC
				buf[frame*channels() + channel] = std::max(clipvalue, _ab[frame][channel]);
			}
		}
		sf_writef_float(m_sf, static_cast<float*>(buf.data()), frames);
	}
	else // integer PCM encoding
	{
		auto buf = std::vector<int_sample_t>(frames * channels());
		convertToS16(_ab, frames, buf.data(), !isLittleEndian());
		sf_writef_short(m_sf, static_cast<short*>(buf.data()), frames);
	}

}


void AudioFileFlac::finishEncoding()
{
	if (m_sf)
	{
		sf_write_sync(m_sf);
		sf_close(m_sf);
	}
}

} // namespace lmms