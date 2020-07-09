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

#ifndef AUDIO_FILE_OGG_H
#define AUDIO_FILE_OGG_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_OGGVORBIS

#include <vorbis/codec.h>

#include "AudioFileDevice.h"


class AudioFileOgg : public AudioFileDevice
{
public:
	AudioFileOgg( OutputSettings const & outputSettings,
			const ch_cnt_t _channels,
			bool & _success_ful,
			const QString & _file,
			Mixer* mixer );
	virtual ~AudioFileOgg();

	static AudioFileDevice * getInst( const QString & outputFilename,
					  OutputSettings const & outputSettings,
					  const ch_cnt_t channels,
					  Mixer* mixer,
					  bool & successful )
	{
		return new AudioFileOgg( outputSettings, channels, successful,
						outputFilename, mixer );
	}


private:
	virtual void writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames,
						const float _master_gain ) override;

	bool startEncoding();
	void finishEncoding();
	inline int writePage();

	inline bitrate_t nominalBitrate() const
	{
		return getOutputSettings().getBitRateSettings().getBitRate();
	}

	inline bitrate_t minBitrate() const
	{
		if (nominalBitrate() > 64)
		{
			return nominalBitrate() - 64;
		}
		else
		{
			return 64;
		}
	}

	inline bitrate_t maxBitrate() const
	{
		return nominalBitrate() + 64;
	}

private:
	bool m_ok;
	ch_cnt_t m_channels;
	sample_rate_t m_rate;

	uint32_t m_serialNo;

	vorbis_comment * m_comments;

	// encoding setup - init by init_ogg_encoding
	ogg_stream_state	m_os;
	ogg_page 	 	m_og;
	ogg_packet	 	m_op;

	vorbis_dsp_state 	m_vd;
	vorbis_block     	m_vb;
	vorbis_info      	m_vi;

} ;


#endif

#endif
