/*
 * audio_file_ogg.cpp - Audio-device which encodes wave-stream and writes it
 *                      into an OGG-file. This is used for song-export.
 *
 * This file is based on encode.c from vorbis-tools-source, for more information
 * see below.
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000-2002, Michael Smith <msmith@labyrinth.net.au>
 **
 ** Portions from Vorbize, (c) Kenneth Arnold <kcarnold@yahoo.com>
 ** and libvorbis examples, (c) Monty <monty@xiph.org>
 **/


#include <qpair.h>

#include "audio_file_ogg.h"

#ifdef HAVE_VORBIS_CODEC_H


#include <vorbis/vorbisenc.h>
#include <cstring>


audioFileOgg::audioFileOgg( Uint32 _sample_rate, Uint32 _channels,
				bool & _success_ful, const QString & _file,
				bool _use_vbr, Uint16 _nom_bitrate,
				Uint16 _min_bitrate, Uint16 _max_bitrate ) :
	audioFileDevice( _sample_rate, _channels, _file, _use_vbr,
				_nom_bitrate, _min_bitrate, _max_bitrate )
{
	_success_ful = startEncoding();
}




audioFileOgg::~audioFileOgg()
{
	finishEncoding();
}




inline int audioFileOgg::writePage( void )
{
	int written = writeData( m_og.header, m_og.header_len );
	written += writeData( m_og.body, m_og.body_len );
	return( written );
}




bool audioFileOgg::startEncoding( void )
{
	vorbis_comment vc;
	char * comments = "Cool=This song was written with Linux "
							"MultiMedia Studio";
	int comment_length = strlen( comments );

	vc.user_comments = &comments;
	vc.comment_lengths = &comment_length;
	vc.comments = 1;
	vc.vendor = NULL;

	m_channels = channels();
	// vbr enabled?
	if( useVBR() == 0 )
	{
		m_managed 	= -1;		// we don't want vbr
		m_minBitrate 	= nominalBitrate();	// min for vbr
		m_maxBitrate 	= nominalBitrate();	// max for vbr
	}
	else
	{
		m_managed 	= 0;		// let's use vbr
		m_minBitrate 	= minBitrate();	// min for vbr
		m_maxBitrate 	= maxBitrate();	// max for vbr
	}

	m_bitrate 	= nominalBitrate();	// nominal bitrate
	m_rate 		= sampleRate();		// default-samplerate
	m_serialNo 	= 0;			// track-num?
	m_comments 	= &vc;			// comments for ogg-file

	// Have vorbisenc choose a mode for us
	vorbis_info_init( &m_vi );

	if( vorbis_encode_setup_managed( &m_vi, m_channels, m_rate,
			( m_maxBitrate > 0 )? m_maxBitrate * 1000 : -1,
						m_bitrate * 1000, 
			( m_minBitrate > 0 )? m_minBitrate * 1000 : -1 ) )
	{
		printf( "Mode initialization failed: invalid parameters for "
								"bitrate\n" );
		vorbis_info_clear( &m_vi );
		return( FALSE );
	}
    
	if( m_managed && m_bitrate < 0 )
	{
		vorbis_encode_ctl( &m_vi, OV_ECTL_RATEMANAGE_AVG, NULL );
	}
	else if( !m_managed )
	{
		// Turn off management entirely (if it was turned on).
		vorbis_encode_ctl( &m_vi, OV_ECTL_RATEMANAGE_SET, NULL );
	}
    
	vorbis_encode_setup_init( &m_vi );

	// Now, set up the analysis engine, stream encoder, and other
	// preparation before the encoding begins.
	vorbis_analysis_init( &m_vd, &m_vi );
	vorbis_block_init( &m_vd, &m_vb );

	ogg_stream_init( &m_os, m_serialNo );

	// Now, build the three header packets and send through to the stream
	// output stage (but defer actual file output until the main encode
	// loop)

	ogg_packet header_main;
	ogg_packet header_comments;
	ogg_packet header_codebooks;
	int result;

	// Build the packets
	vorbis_analysis_headerout( &m_vd, m_comments, &header_main,
					&header_comments, &header_codebooks );

	// And stream them out
	ogg_stream_packetin( &m_os, &header_main );
	ogg_stream_packetin( &m_os, &header_comments );
	ogg_stream_packetin( &m_os, &header_codebooks );

	while( ( result = ogg_stream_flush( &m_os, &m_og ) ) )
	{
		if( !result )
		{
			break;
		}
		int ret = writePage();
		if( ret != m_og.header_len + m_og.body_len )
		{
			// clean up
			finishEncoding();
			return( FALSE );
		}
	}

	return( TRUE );
}




void FASTCALL audioFileOgg::writeBufferToDev( surroundSampleFrame * _ab,
						Uint32 _frames,
						float _master_gain )
{
	int eos = 0;

	float * * buffer = vorbis_analysis_buffer( &m_vd, _frames *
							BYTES_PER_SAMPLE *
								channels() );
	for( Uint32 frame = 0; frame < _frames; ++frame )
	{
		for( Uint8 chnl = 0; chnl < channels(); ++chnl )
		{
			buffer[chnl][frame] = _ab[frame][chnl] * _master_gain;
		}
	}

	vorbis_analysis_wrote( &m_vd, _frames );

	// While we can get enough data from the library to analyse,
	// one block at a time...
	while( vorbis_analysis_blockout( &m_vd, &m_vb ) == 1 )
	{
		// Do the main analysis, creating a packet
		vorbis_analysis( &m_vb, NULL );
		vorbis_bitrate_addblock( &m_vb );

		while( vorbis_bitrate_flushpacket( &m_vd, &m_op ) )
		{
			// Add packet to bitstream
			ogg_stream_packetin( &m_os, &m_op );

			// If we've gone over a page boundary, we can do
			// actual output, so do so (for however many pages
			// are available)
			while( !eos )
			{
				int result = ogg_stream_pageout( &m_os,
								&m_og );
				if( !result )
				{
					break;
				}

				int ret = writePage();
				if( ret != m_og.header_len +
							m_og.body_len )
				{
					printf( "failed writing to "
								"outstream\n" );
					return;
				}
	
				if( ogg_page_eos( &m_og ) )
				{
					eos = 1;
				}
			}
		}
	}
}




void audioFileOgg::finishEncoding( void )
{
	// just for flushing buffers...
	writeBufferToDev( NULL, 0, 0.0f );

	// clean up
	ogg_stream_clear( &m_os );

	vorbis_block_clear( &m_vb );
	vorbis_dsp_clear( &m_vd );
	vorbis_info_clear( &m_vi );
}


#endif

