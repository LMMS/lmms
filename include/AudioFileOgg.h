/*
 * AudioFileOgg.h - Audio-device which encodes wave-stream and writes it
 *                  into an OGG-file. This is used for song-export.
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

#ifndef LMMS_AUDIO_FILE_OGG_H
#define LMMS_AUDIO_FILE_OGG_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_OGGVORBIS

#include <vorbis/codec.h>

#include "AudioFileDevice.h"

namespace lmms
{

class AudioFileOgg : public AudioFileDevice
{
public:
	AudioFileOgg( OutputSettings const & outputSettings,
			const ch_cnt_t _channels,
			bool & _success_ful,
			const QString & _file,
			AudioEngine* audioEngine );
	~AudioFileOgg() override;

	static AudioFileDevice * getInst( const QString & outputFilename,
					  OutputSettings const & outputSettings,
					  const ch_cnt_t channels,
					  AudioEngine* audioEngine,
					  bool & successful )
	{
		return new AudioFileOgg( outputSettings, channels, successful, outputFilename, audioEngine );
	}

private:
	void writeBuffer(const SampleFrame* _ab, const fpp_t _frames) override;
	vorbis_info m_vi;
	vorbis_dsp_state m_vds;
	vorbis_comment m_vc;
	vorbis_block m_vb;
	ogg_stream_state m_oss;
	ogg_packet m_packet;
	ogg_page m_page;
};

} // namespace lmms

#endif // LMMS_HAVE_OGGVORBIS

#endif // LMMS_AUDIO_FILE_OGG_H
