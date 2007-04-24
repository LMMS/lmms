#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_file_wave.cpp - audio-device which encodes wave-stream and writes it
 *                       into a WAVE-file. This is used for song-export.
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



#include "audio_file_wave.h"
#include "endian_handling.h"

#include <cstring>


audioFileWave::audioFileWave( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, bool & _success_ful,
				const QString & _file,
				const bool _use_vbr,
				const bitrate_t _nom_bitrate,
				const bitrate_t _min_bitrate,
				const bitrate_t _max_bitrate,
				mixer * _mixer ) :
	audioFileDevice( _sample_rate, _channels, _file, _use_vbr,
			_nom_bitrate, _min_bitrate, _max_bitrate, _mixer )
{
	_success_ful = startEncoding();
}




audioFileWave::~audioFileWave()
{
	finishEncoding();
}




bool audioFileWave::startEncoding( void )
{
	if( outputFileOpened() == FALSE )
	{
		return( FALSE );
	}

	m_bytesWritten = 0;

	memcpy( m_waveFileHeader.riff_id, "RIFF", 4 );
	m_waveFileHeader.total_bytes = swap32IfBE( 0 );
	memcpy( m_waveFileHeader.wave_fmt_str, "WAVEfmt ", 8 );
	m_waveFileHeader.bitrate_1 =
		m_waveFileHeader.bitrate_2 =
			swap16IfBE( BYTES_PER_INT_SAMPLE * 8 );
	m_waveFileHeader.uncompressed = swap16IfBE( 1 );
	m_waveFileHeader.channels = swap16IfBE( channels() );
	m_waveFileHeader.sample_rate = swap32IfBE( sampleRate() );
	m_waveFileHeader.bytes_per_second = swap32IfBE( sampleRate() *
					BYTES_PER_INT_SAMPLE * channels() );
	m_waveFileHeader.block_alignment = swap16IfBE( BYTES_PER_INT_SAMPLE *
								channels() );
	memcpy ( m_waveFileHeader.data_chunk_id, "data", 4 );
	m_waveFileHeader.data_bytes = swap32IfBE( 0 );

	writeData( &m_waveFileHeader, sizeof( m_waveFileHeader ) );

	return( TRUE );
}




void FASTCALL audioFileWave::writeBuffer( const surroundSampleFrame * _ab,
						const fpab_t _frames,
						const float _master_gain )
{
	int_sample_t * outbuf = new int_sample_t[_frames * channels()];
	Uint32 bytes = convertToS16( _ab, _frames, _master_gain, outbuf,
							!isLittleEndian() );
	writeData( outbuf, bytes );

	delete[] outbuf;

	m_bytesWritten += bytes;
}




void audioFileWave::finishEncoding( void )
{
	seekToBegin();

	m_waveFileHeader.total_bytes = m_bytesWritten+36;
	m_waveFileHeader.data_bytes = m_bytesWritten;

	// write header again, because total-bytes-field and data-bytes-field
	// have to be updated...
	writeData( &m_waveFileHeader, sizeof( m_waveFileHeader ) );
}


#endif
