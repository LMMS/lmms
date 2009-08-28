/*
 * AudioFileFlac.cpp - Audio-device which encodes a flac stream and writes it
 *                     into a flac file. This is used for song-export.
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

#include "AudioFileFlac.h"

#ifdef LMMS_HAVE_FLAC

#include <QtDebug>

#include "lmms_basics.h"



AudioFileFlac::AudioFileFlac( const sample_rate_t _sample_rate,
	const ch_cnt_t _channels, bool & _success_ful, const QString & _file,
	const bool _use_vbr, const bitrate_t _nom_bitrate,
	const bitrate_t _min_bitrate, const bitrate_t _max_bitrate,
	const int _depth, mixer * _mixer ) :
	AudioFileDevice( _sample_rate, _channels, _file, _use_vbr, _nom_bitrate,
		_min_bitrate, _max_bitrate, _depth, _mixer )
{
	_success_ful = startEncoding();
}


bool AudioFileFlac::startEncoding()
{
	// check the encoder
	if( ! m_encoder )
	{
		qWarning() << "AudioFileFlac: unable to allocate encoder";
		return false;
	}

	bool ok = true;

	ok &= m_encoder.set_verify(true);
	ok &= m_encoder.set_compression_level(5);
	ok &= m_encoder.set_channels(channels());
	ok &= m_encoder.set_bits_per_sample(16);
	ok &= m_encoder.set_sample_rate(sampleRate());
	//ok &= encoder.set_total_samples_estimate(??);

	if( ! ok ) return false;

	// TODO: set metadata: http://flac.cvs.sourceforge.net/viewvc/flac/flac/examples/cpp/encode/file/main.cpp?view=markup

	
	// initialize encoder
	FLAC__StreamEncoderInitStatus init_status = m_encoder.init(
		outputFile().toAscii());
	if( init_status != FLAC__STREAM_ENCODER_INIT_STATUS_OK )
	{
		qWarning() << "AudioFileFlac: unable to initialize encoder:" << 
			FLAC__StreamEncoderInitStatusString[init_status];
		return false;
	}

	return true;

}


Sint16 AudioFileFlac::rescale(float sample) {
	return (qMax<float>(qMin<float>(sample, 1), -1) / 1) 
		* std::numeric_limits<Sint16>::max();
}


// encode data and write to file
void AudioFileFlac::writeBuffer( const surroundSampleFrame * _ab,
	const fpp_t _frames, const float _master_gain )
{
	// scale to short int instead of float
	FLAC__int32 * in = new FLAC__int32[_frames*channels()];

	for(int i=0; i < _frames; ++i)
	{
		for(int c=0; c < channels(); ++c)
		{
			in[i*channels()+c] = (FLAC__int32) rescale( _ab[i][c] 
				* _master_gain );
		}
	}

	// feed to the encoder
	m_encoder.process_interleaved(in, _frames);

	// clean up
	delete[] in;
}


void AudioFileFlac::finishEncoding()
{
	m_encoder.finish();
}


AudioFileFlac::~AudioFileFlac()
{
	finishEncoding();
}

#endif

/* vim: set tw=0 noexpandtab: */
