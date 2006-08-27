/*
 * audio_file_wave.h - Audio-device which encodes wave-stream and writes it
 *                     into an WAVE-file. This is used for song-export.
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_FILE_WAVE_H
#define _AUDIO_FILE_WAVE_H


#include "audio_file_device.h"



class audioFileWave : public audioFileDevice
{
public:
	audioFileWave( const sample_rate_t _sample_rate,
			const ch_cnt_t _channels,
			bool & _success_ful,
			const QString & _file,
			const bool _use_vbr,
			const bitrate_t _nom_bitrate,
			const bitrate_t _min_bitrate,
			const bitrate_t _max_bitrate,
			mixer * _mixer );
	virtual ~audioFileWave();

	static audioFileDevice * getInst( const sample_rate_t _sample_rate,
						const ch_cnt_t _channels,
						bool & _success_ful,
						const QString & _file,
						const bool _use_vbr,
						const bitrate_t _nom_bitrate,
						const bitrate_t _min_bitrate,
						const bitrate_t _max_bitrate,
						mixer * _mixer )
	{
		return( new audioFileWave( _sample_rate, _channels,
						_success_ful, _file, _use_vbr,
						_nom_bitrate, _min_bitrate,
							_max_bitrate,
							_mixer ) );
	}


private:
	virtual void FASTCALL writeBuffer( const surroundSampleFrame * _ab,
						const fpab_t _frames,
						float _master_gain );

	bool startEncoding( void );
	void finishEncoding( void );


	int m_bytesWritten;

	struct waveFileHeader
	{
		char riff_id[4];		// "RIFF"
		Uint32 total_bytes;		// total filesize-8
		char wave_fmt_str[8];		// "WAVEfmt"
		Uint32 bitrate_1;		// bitrate, e.g. 16
		Uint16 uncompressed;		// 1 if PCM
		Uint16 channels;		// 1 = mono  2 = stereo
		Uint32 sample_rate;		// sample-rate e.g. 44100
		Uint32 bytes_per_second;	// sample-rate*channels*
						// (bitrate/8)
		Uint16 block_alignment;		// channels*(bitrate/8)
		Uint16 bitrate_2;		// bitrate, e.g. 16
		char data_chunk_id[4];		// "data"
		Uint32 data_bytes;		// total size of sample-data
	} m_waveFileHeader;

} ;


#endif
