/*
 * AudioFileDevice.h - base-class for audio-device-classes which write 
 *                     their output into a file
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _AUDIO_FILE_DEVICE_H
#define _AUDIO_FILE_DEVICE_H

#include <QtCore/QFile>

#include "AudioDevice.h"


class AudioFileDevice : public AudioDevice
{
public:
	AudioFileDevice( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, const QString & _file,
				const bool _use_vbr,
				const bitrate_t _nom_bitrate,
				const bitrate_t _min_bitrate,
				const bitrate_t _max_bitrate,
				const int _depth,
				Mixer* mixer );
	virtual ~AudioFileDevice();

	QString outputFile() const
	{
		return m_outputFile.fileName();
	}


protected:
	int writeData( const void* data, int len );

	inline bool useVBR() const
	{
		return m_useVbr;
	}

	inline bitrate_t nominalBitrate() const
	{
		return m_nomBitrate;
	}

	inline bitrate_t minBitrate() const
	{
		return m_minBitrate;
	}

	inline bitrate_t maxBitrate() const
	{
		return m_maxBitrate;
	}

	inline int depth() const
	{
		return m_depth;
	}

	inline bool outputFileOpened() const
	{
		return m_outputFile.isOpen();
	}


private:
	QFile m_outputFile;

	bool m_useVbr;

	bitrate_t m_nomBitrate;
	bitrate_t m_minBitrate;
	bitrate_t m_maxBitrate;

	int m_depth;

} ;


typedef AudioFileDevice * ( * AudioFileDeviceInstantiaton )
					( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
						bool & _success_ful,
						const QString & _file,
						const bool _use_vbr,
						const bitrate_t _nom_bitrate,
						const bitrate_t _min_bitrate,
						const bitrate_t _max_bitrate,
						const int _depth,
						Mixer* mixer );


#endif
