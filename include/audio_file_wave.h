/*
 * audio_file_wave.h - Audio-device which encodes wave-stream and writes it
 *                     into an WAVE-file. This is used for song-export.
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _AUDIO_FILE_WAVE_H
#define _AUDIO_FILE_WAVE_H


#include "audio_file_device.h"



class audioFileWave : public audioFileDevice
{
public:
	audioFileWave( Uint32 _sample_rate, Uint32 _channels,
			bool & _success_ful, const QString & _file,
			bool _use_vbr, Uint16 _nom_bitrate,
			Uint16 _min_bitrate, Uint16 _max_bitrate );
	virtual ~audioFileWave();

	static audioDevice * getInst( Uint32 _sample_rate, Uint32 _channels,
					bool & _success_ful,
					const QString & _file, bool _use_vbr,
					Uint16 _nom_bitrate,
					Uint16 _min_bitrate,
					Uint16 _max_bitrate )
	{
		return( new audioFileWave( _sample_rate, _channels,
						_success_ful, _file, _use_vbr,
						_nom_bitrate, _min_bitrate,
						_max_bitrate ) );
	}


private:
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame * _ab,
						Uint32 _frames,
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

	//outputSampleType * m_outputBuffer;

} ;


#endif
