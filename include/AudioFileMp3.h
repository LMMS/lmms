/*
 * AudioFileMp3.h - Audio-device which encodes mp3-stream and writes it
 *                  into an mp3-file. This is used for song-export.
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

#ifndef _AUDIO_FILE_MP3_H
#define _AUDIO_FILE_MP3_H

#include <QFile>

#include "lame_library.h"
#include "lmmsconfig.h"
#include "AudioFileDevice.h"

class AudioFileMp3 : public AudioFileDevice
{
public:
	AudioFileMp3( const sample_rate_t _sample_rate,
			const ch_cnt_t _channels,
			bool & _success_ful,
			const QString & _file,
			const bool _use_vbr,
			const bitrate_t _nom_bitrate,
			const bitrate_t _min_bitrate,
			const bitrate_t _max_bitrate,
			const int _depth,
			AudioOutputContext * context );
	virtual ~AudioFileMp3();

	static AudioFileDevice * getInst( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
						bool & _success_ful,
						const QString & _file,
						const bool _use_vbr,
						const bitrate_t _nom_bitrate,
						const bitrate_t _min_bitrate,
						const bitrate_t _max_bitrate,
						const int _depth,
						AudioOutputContext * context )
	{
		return new AudioFileMp3( _sample_rate, _channels,
						_success_ful, _file, _use_vbr,
						_nom_bitrate, _min_bitrate,
							_max_bitrate, _depth, context );
	}


private:
	short int rescale(float sample); // convert float flame to short int frame

	// overloaded functions
	virtual void writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames,
						float _master_gain );

	bool startEncoding();
	void finishEncoding();


	// handle to lame
	lame_global_flags *m_lgf;
	LameLibrary m_lame;

	QFile * m_outfile;
	bool m_hq_mode; // true if we want really high quality

} ;


#endif

/* vim: set tw=0 noexpandtab: */
