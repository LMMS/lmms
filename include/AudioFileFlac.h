/*
 * AudioFileFlac.h - Audio device which encodes a wave stream into a FLAC file.
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

#ifndef AUDIO_FILE_FLAC_H
#define AUDIO_FILE_FLAC_H

#include "lmmsconfig.h"

#include "AudioFileDevice.h"
#include <sndfile.h>

class AudioFileFlac: public AudioFileDevice
{
public:
	AudioFileFlac(OutputSettings const& outputSettings,
			ch_cnt_t const channels,
			bool& successful,
			QString const& file,
			Mixer* mixer
	);

	virtual ~AudioFileFlac();

	static AudioFileDevice* getInst(QString const& outputFilename,
			OutputSettings const& outputSettings,
			ch_cnt_t const channels,
			Mixer* mixer,
			bool& successful)
	{
		return new AudioFileFlac(
			outputSettings,
			channels,
			successful,
			outputFilename,
			mixer
		);
	}

private:

	SF_INFO  m_sfinfo;
	SNDFILE* m_sf;

	virtual void writeBuffer(surroundSampleFrame const* _ab,
						fpp_t const frames,
						float master_gain) override;

	bool startEncoding();
	void finishEncoding();

};

#endif //AUDIO_FILE_FLAC_H
