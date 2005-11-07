/*
 * audio_file_ogg.h - Audio-device which encodes wave-stream and writes it
 *                    into an OGG-file. This is used for song-export.
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_FILE_OGG_H
#define _AUDIO_FILE_OGG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_VORBIS_CODEC_H

#include <vorbis/codec.h>

#include "audio_file_device.h"



class audioFileOgg : public audioFileDevice
{
public:
	audioFileOgg( Uint32 _sample_rate, Uint32 _channels,
			bool & _success_ful, const QString & _file,
			bool _use_vbr, Uint16 _nom_bitrate,
			Uint16 _min_bitrate, Uint16 _max_bitrate );
	~audioFileOgg();

	static audioDevice * getInst( Uint32 _sample_rate, Uint32 _channels,
							bool & _success_ful,
							const QString & _file,
							bool _use_vbr,
							Uint16 _nom_bitrate,
							Uint16 _min_bitrate,
							Uint16 _max_bitrate )
	{
		return( new audioFileOgg( _sample_rate, _channels, _success_ful,
						_file, _use_vbr, _nom_bitrate,
						_min_bitrate, _max_bitrate ) );
	}


private:
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame * _ab,
							Uint32 _frames,
							float _master_gain );

	bool startEncoding( void );
	void finishEncoding( void );
	inline int writePage( void );



	int m_channels;
	long m_rate;

	// Various bitrate/quality options
	int m_managed;
	int m_bitrate;
	int m_minBitrate;
	int m_maxBitrate;

	unsigned int m_serialNo;

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
