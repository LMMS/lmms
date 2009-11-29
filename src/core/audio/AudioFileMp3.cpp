/*
 * AudioFileMp3.cpp - audio-device which encodes mp3-stream and writes it
 *                    into an mp3-file. This is used for song-export.
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

#include "lame_library.h"
#include "AudioFileMp3.h"

#include <limits>
#include <QtDebug>

#include "lmms_basics.h"
#include "config_mgr.h"

using namespace std;

AudioFileMp3::AudioFileMp3( const sample_rate_t _sample_rate,
	const ch_cnt_t _channels, bool & _success_ful, const QString & _file,
	const bool _use_vbr, const bitrate_t _nom_bitrate,
	const bitrate_t _min_bitrate, const bitrate_t _max_bitrate,
	const int _depth, AudioOutputContext * context ) :
	AudioFileDevice( _sample_rate, _channels, _file, _use_vbr, _nom_bitrate,
		_min_bitrate, _max_bitrate, _depth, context ),
	m_lgf( NULL ),
	m_lame( LameLibrary() ),
	m_outfile( NULL ),
	m_hq_mode( false )
{
	_success_ful = m_lame.isLoaded() && startEncoding();
}



AudioFileMp3::~AudioFileMp3()
{
	finishEncoding();

	if( m_outfile )
	{
		if( m_outfile->isOpen() )
			m_outfile->close();

		delete m_outfile;
	}
}


void AudioFileMp3::finishEncoding()
{
	if( m_lgf )
	{
		// flush
		int bufSize = 7200;
		unsigned char * out = new unsigned char[bufSize];
		int rc = m_lame.lame_encode_flush(m_lgf, out, bufSize);

		if( m_outfile && m_outfile->isOpen() ){
			m_outfile->write( (const char *) out, rc );

			m_outfile->close();
			delete m_outfile;
			m_outfile = NULL;
		}

		// cleanup
		delete[] out;

		// close any open handles we may have
		m_lame.lame_close(m_lgf);
		m_lgf = NULL;
	}
}


bool AudioFileMp3::startEncoding()
{
	// open any handles, files, etc
	m_lgf = m_lame.lame_init();
	if( m_lgf == NULL ){
		qWarning() << "AudioFileMp3: Unable to initialize lame";
		return false;
	}

	if( channels() > 2 )
		qWarning() << "I don't think lame can do more than 2 channels";

	m_lame.lame_set_in_samplerate(m_lgf, sampleRate() );
	m_lame.lame_set_num_channels(m_lgf, channels() );

	if( m_hq_mode )
		m_lame.lame_set_quality(m_lgf, 0); // best, very slow
	else
		m_lame.lame_set_quality(m_lgf, 2); // near-best, not too slow
	
	m_lame.lame_set_mode(m_lgf, STEREO); 
	m_lame.lame_set_findReplayGain(m_lgf, 1); // perform ReplayGain analysis

	if( useVBR() == 0 )
	{
		m_lame.lame_set_VBR(m_lgf, vbr_off);
		m_lame.lame_set_brate(m_lgf, nominalBitrate() );
	}
	else
	{
		m_lame.lame_set_VBR(m_lgf, vbr_abr);
		m_lame.lame_set_VBR_quality(m_lgf, 2);
		m_lame.lame_set_VBR_mean_bitrate_kbps(m_lgf, nominalBitrate() );
		m_lame.lame_set_VBR_min_bitrate_kbps(m_lgf, minBitrate() );
		m_lame.lame_set_VBR_max_bitrate_kbps(m_lgf, maxBitrate() );
	}

	if( m_lame.lame_init_params( m_lgf ) < 0 )
		return false;

	// open the file
	m_outfile = new QFile( outputFile() );
	if( ! m_outfile->open( QIODevice::WriteOnly ) )
	{
		qWarning() << "AudioFileMp3: unable to open file for output";
		return false;
	}

	// write the headers and such
	// TODO: add meta information with artist, title, etc, and
	//       add a comment "created with LMMS"

	return true;

}


Sint16 AudioFileMp3::rescale(float sample) {
	return (qMax<float>(qMin<float>(sample, 1), -1) / 1) 
		* std::numeric_limits<Sint16>::max();
}

// encode data and write to file
void AudioFileMp3::writeBuffer( const surroundSampleFrame * _ab,
	const fpp_t _frames, const float _master_gain )
{
	// encode with lame
	int bufSize = 1.25 * _frames + 7200;
	Sint16 * in = new Sint16[_frames*channels()];
	unsigned char * out = new unsigned char[bufSize];
	
	// scale to Sint16 instead of float
	for(int i=0; i < _frames; ++i)
	{
		for(int c=0; c < channels(); ++c)
		{
			in[i*channels()+c] = rescale( _ab[i][c] * _master_gain );
		}
	}

	int rc = m_lame.lame_encode_buffer_interleaved( m_lgf, in, _frames, 
		out, bufSize);

	switch(rc){
		case -1:
			qWarning() << "AudioFileMp3: encode error: buffer too small.";
			return;
		case -2: 
			qWarning() << "AudioFileMp3: encode error: out of memory";
			return;
		case -3: 
			qWarning() << "AudioFileMp3: encode error: lame_init_params not called";
			return;
		case -4: 
			qWarning() << "AudioFileMp3: encode error: psycho acoustic problems";
			return;
	}

	// write to file
	m_outfile->write( (const char *) out, rc );

	// clean up
	delete[] out;
	delete[] in;
}






/* vim: set tw=0 noexpandtab: */
