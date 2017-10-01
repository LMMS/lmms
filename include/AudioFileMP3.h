/*
 * AudioFileMP3.h - Audio-device which encodes a wave stream into
 *                  an MP3 file. This is used for song export.
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

#ifndef AUDIO_FILE_MP3_H
#define AUDIO_FILE_MP3_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_MP3LAME

#include "AudioFileDevice.h"

#include "lame/lame.h"


class AudioFileMP3 : public AudioFileDevice
{
public:
	AudioFileMP3( OutputSettings const & outputSettings,
			const ch_cnt_t _channels,
			bool & successful,
			const QString & _file,
			Mixer* mixer );
	virtual ~AudioFileMP3();

	static AudioFileDevice * getInst( const QString & outputFilename,
					  OutputSettings const & outputSettings,
					  const ch_cnt_t channels,
					  Mixer* mixer,
					  bool & successful )
	{
		return new AudioFileMP3( outputSettings, channels, successful,
					 outputFilename, mixer );
	}

protected:
	virtual void writeBuffer( const surroundSampleFrame * /* _buf*/,
				  const fpp_t /*_frames*/,
				  const float /*_master_gain*/ );

private:
	void flushRemainingBuffers();
	bool initEncoder();
	void tearDownEncoder();

private:
	lame_t m_lame;
};

#endif

#endif
