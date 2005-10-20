/*
 * audio_file_wave.cpp - Audio-device which encodes wave-stream and writes it
 *                       into a WAVE-file. This is used for song-export.
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



#include "audio_file_wave.h"
#include "endian_handling.h"
#include "buffer_allocator.h"

#include <cstring>


audioFileWave::audioFileWave( Uint32 _sample_rate, Uint32 _channels,
				bool & _success_ful, const QString & _file,
				bool _use_vbr, Uint16 _nom_bitrate,
				Uint16 _min_bitrate, Uint16 _max_bitrate ) :
	audioFileDevice( _sample_rate, _channels, _file, _use_vbr,
				_nom_bitrate, _min_bitrate, _max_bitrate )
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
			swap16IfBE( BYTES_PER_OUTPUT_SAMPLE * 8 );
	m_waveFileHeader.uncompressed = swap16IfBE( 1 );
	m_waveFileHeader.channels = swap16IfBE( channels() );
	m_waveFileHeader.sample_rate = swap32IfBE( sampleRate() );
	m_waveFileHeader.bytes_per_second = swap32IfBE( sampleRate() *
						BYTES_PER_OUTPUT_SAMPLE *
								channels() );
	m_waveFileHeader.block_alignment = swap16IfBE( BYTES_PER_OUTPUT_SAMPLE *
								channels() );
	memcpy ( m_waveFileHeader.data_chunk_id, "data", 4 );
	m_waveFileHeader.data_bytes = swap32IfBE( 0 );

	writeData( &m_waveFileHeader, sizeof( m_waveFileHeader ) );

	return( TRUE );
}




void FASTCALL audioFileWave::writeBufferToDev( surroundSampleFrame * _ab,
							Uint32 _frames,
							float _master_gain )
{
	outputSampleType * outbuf = bufferAllocator::alloc<outputSampleType>(
							_frames * channels() );
	int bytes = convertToS16( _ab, _frames, _master_gain, outbuf,
							!isLittleEndian() );
	writeData( outbuf, bytes );

	bufferAllocator::free( outbuf );

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

