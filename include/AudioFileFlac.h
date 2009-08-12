/*
 * AudioFileFlac.h - Audio-device which encodes a flac stream and writes it
 *					 into a flac file. This is used for song-export.
 *
 * Copyright (c) 2009 Andrew Kelley <superjoe30@gmail.com>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#ifndef _AUDIO_FILE_FLAC_H_
#define _AUDIO_FILE_FLAC_H_

#include <QFile>
#include "AudioFileDevice.h"

#include "FLAC++/metadata.h"
#include "FLAC++/encoder.h"

class AudioFileFlac : public AudioFileDevice
{
public:
	AudioFileFlac( const sample_rate_t _sample_rate,
			const ch_cnt_t _channels,
			bool & _success_ful,
			const QString & _file,
			const bool _use_vbr,
			const bitrate_t _nom_bitrate,
			const bitrate_t _min_bitrate,
			const bitrate_t _max_bitrate,
			const int _depth,
			mixer * _mixer );
	virtual ~AudioFileFlac();

	static AudioFileDevice * getInst( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
						bool & _success_ful,
						const QString & _file,
						const bool _use_vbr,
						const bitrate_t _nom_bitrate,
						const bitrate_t _min_bitrate,
						const bitrate_t _max_bitrate,
						const int _depth,
						mixer * _mixer )
	{
		return new AudioFileFlac( _sample_rate, _channels,
						_success_ful, _file, _use_vbr,
						_nom_bitrate, _min_bitrate,
							_max_bitrate, _depth,
							_mixer );
	}


private:
	short int rescale(float sample); // convert float flame to short int frame

	// overloaded functions
	virtual void writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames,
						float _master_gain );

	bool startEncoding( void );
	void finishEncoding( void );


	FLAC::Encoder::File m_encoder;

} ;


#endif //_AUDIO_FILE_FLAC_H_

/* vim: set tw=0 noexpandtab: */
