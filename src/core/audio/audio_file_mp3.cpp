/*
 * audo_file_mp3.cpp - audio-device which encodes mp3-stream and writes it
 *                       into an mp3-file. This is used for song-export.
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *               2009 Andrew Kelley <superjoe30@gmail.com>
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

#include "lame.h"
#include "audio_file_mp3.h"

#include <cstring>
#include <limits>
using namespace std;



AudioFileMp3::AudioFileMp3( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, bool & _success_ful,
				const QString & _file,
				const bool _use_vbr,
				const bitrate_t _nom_bitrate,
				const bitrate_t _min_bitrate,
				const bitrate_t _max_bitrate,
				const int _depth,
				mixer * _mixer ) :
	audioFileDevice( _sample_rate, _channels, _file, _use_vbr,
			_nom_bitrate, _min_bitrate, _max_bitrate,
								_depth, _mixer ),
    m_lgf( NULL ),
    m_lame( NULL ),
    m_outfile( NULL ),
    m_hq_mode( false )
{
    // connect to lame
    m_ok = initLame("/usr/lib/libmp3lame.so.0");
	m_ok = _success_ful = m_ok && startEncoding();
}


bool AudioFileMp3::initLame(QString libpath)
{
    // dynamically load the .so file
    m_lame = new QLibrary(libpath);

    if( ! m_lame->load() ) return false;

    // grab the functions and stuff we need
    lame_init = (lame_init_t *)
      m_lame->resolve("lame_init");
    get_lame_version = (get_lame_version_t *)
      m_lame->resolve("get_lame_version");
    lame_init_params = (lame_init_params_t *)
      m_lame->resolve("lame_init_params");
    lame_encode_buffer = (lame_encode_buffer_t *)
      m_lame->resolve("lame_encode_buffer");
    lame_encode_buffer_interleaved = (lame_encode_buffer_interleaved_t *)
      m_lame->resolve("lame_encode_buffer_interleaved");
    lame_encode_flush = (lame_encode_flush_t *)
      m_lame->resolve("lame_encode_flush");
    lame_close = (lame_close_t *)
      m_lame->resolve("lame_close");

    lame_set_in_samplerate = (lame_set_in_samplerate_t *)
       m_lame->resolve("lame_set_in_samplerate");
    lame_set_out_samplerate = (lame_set_out_samplerate_t *)
       m_lame->resolve("lame_set_out_samplerate");
    lame_set_num_channels = (lame_set_num_channels_t *)
       m_lame->resolve("lame_set_num_channels");
    lame_set_quality = (lame_set_quality_t *)
       m_lame->resolve("lame_set_quality");
    lame_set_brate = (lame_set_brate_t *)
       m_lame->resolve("lame_set_brate");
    lame_set_VBR = (lame_set_VBR_t *)
       m_lame->resolve("lame_set_VBR");
    lame_set_VBR_q = (lame_set_VBR_q_t *)
       m_lame->resolve("lame_set_VBR_q");
    lame_set_VBR_min_bitrate_kbps = (lame_set_VBR_min_bitrate_kbps_t *)
       m_lame->resolve("lame_set_VBR_min_bitrate_kbps");
    lame_set_mode = (lame_set_mode_t *) 
       m_lame->resolve("lame_set_mode");
    lame_set_preset = (lame_set_preset_t *)
       m_lame->resolve("lame_set_preset");
    lame_set_error_protection = (lame_set_error_protection_t *)
       m_lame->resolve("lame_set_error_protection");
    lame_set_disable_reservoir = (lame_set_disable_reservoir_t *)
       m_lame->resolve("lame_set_disable_reservoir");
    lame_set_padding_type = (lame_set_padding_type_t *)
       m_lame->resolve("lame_set_padding_type");
    lame_set_bWriteVbrTag = (lame_set_bWriteVbrTag_t *)
       m_lame->resolve("lame_set_bWriteVbrTag");

    lame_set_findReplayGain = (lame_set_findReplayGain_t *) 
        m_lame->resolve("lame_set_findReplayGain");
    lame_set_VBR_quality = (lame_set_VBR_quality_t *)
        m_lame->resolve("lame_set_VBR_quality");
    lame_set_VBR_mean_bitrate_kbps = (lame_set_VBR_mean_bitrate_kbps_t *)
        m_lame->resolve("lame_set_VBR_mean_bitrate_kbps");
    lame_set_VBR_max_bitrate_kbps = (lame_set_VBR_max_bitrate_kbps_t *)
        m_lame->resolve("lame_set_VBR_max_bitrate_kbps");

    // These are optional
    lame_get_lametag_frame = (lame_get_lametag_frame_t *)
       m_lame->resolve("lame_get_lametag_frame");
    lame_mp3_tags_fid = (lame_mp3_tags_fid_t *)
       m_lame->resolve("lame_mp3_tags_fid");

    if (!lame_init ||
        !get_lame_version ||
        !lame_init_params ||
        !lame_encode_buffer ||
        !lame_encode_buffer_interleaved ||
        !lame_encode_flush ||
        !lame_close ||
        !lame_set_in_samplerate ||
        !lame_set_out_samplerate ||
        !lame_set_num_channels ||
        !lame_set_quality ||
        !lame_set_brate ||
        !lame_set_VBR ||
        !lame_set_VBR_q ||
        !lame_set_mode ||
        !lame_set_preset ||
        !lame_set_error_protection ||
        !lame_set_disable_reservoir ||
        !lame_set_padding_type ||
        !lame_set_bWriteVbrTag)
    {
        // some symbols are missing
        printf("AudioFileMp3: some symbols are missing from the lame library\n");
        m_lame->unload();
        return false;
    }

    return true;
}



AudioFileMp3::~AudioFileMp3()
{
	finishEncoding();

    if( m_lame )
    {
        if( m_lame->isLoaded() )
            m_lame->unload();   

        delete m_lame;
        m_lame = NULL;
    }

    if( m_outfile )
    {
        if( m_outfile->isOpen() )
            m_outfile->close();

        delete m_outfile;
    }
}


void AudioFileMp3::finishEncoding( void )
{
    if( m_lgf )
    {
        // flush
        int bufSize = 7200;
        unsigned char * out = new unsigned char[bufSize];
        int rc = lame_encode_flush(m_lgf, out, bufSize);

        if( m_outfile && m_outfile->isOpen() ){
            m_outfile->write( (const char *) out, rc );

            m_outfile->close();
            delete m_outfile;
            m_outfile = NULL;
        }

        // cleanup
        delete[] out;

        // close any open handles we may have
        lame_close(m_lgf);
        m_lgf = NULL;
    }
}


bool AudioFileMp3::startEncoding( void )
{
    // open any handles, files, etc
    m_lgf = lame_init();
    if( m_lgf == NULL ){
        printf("AudioFileMp3: Unable to initialize lame\n");
        return false;
    }

    if( channels() > 2 )
        printf("I don't think lame can do more than 2 channels\n");

    lame_set_in_samplerate(m_lgf, sampleRate() );
    lame_set_num_channels(m_lgf, channels() );

    if( m_hq_mode )
        lame_set_quality(m_lgf, 0); // best, very slow
    else
        lame_set_quality(m_lgf, 2); // near-best, not too slow
    
    lame_set_mode(m_lgf, STEREO); 
    lame_set_findReplayGain(m_lgf, 1); // perform ReplayGain analysis

	if( useVBR() == 0 )
    {
        lame_set_VBR(m_lgf, vbr_off);
        lame_set_brate(m_lgf, nominalBitrate() );
    }
    else
    {
        lame_set_VBR(m_lgf, vbr_abr);
        lame_set_VBR_quality(m_lgf, 2);
        lame_set_VBR_mean_bitrate_kbps(m_lgf, nominalBitrate() );
        lame_set_VBR_min_bitrate_kbps(m_lgf, minBitrate() );
        lame_set_VBR_max_bitrate_kbps(m_lgf, maxBitrate() );
    }

    if( lame_init_params( m_lgf ) < 0 )
        return false;

    // open the file
    m_outfile = new QFile( outputFile() );
    if( ! m_outfile->open( QIODevice::WriteOnly ) )
    {
        printf("AudioFileMp3: unable to open file for output\n");
        return false;
    }

    // write the headers and such

    return true;

}


short int AudioFileMp3::rescale(float sample) {
    return (sample / 1) * std::numeric_limits<short int>::max();
}

// encode data and write to file
void AudioFileMp3::writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames, const float _master_gain )
{
    // encode with lame
    int bufSize = 1.25 * _frames + 7200;
    short int * in = new short int[_frames*2];
    unsigned char * out = new unsigned char[bufSize];
    
    // scale to short int instead of float
    for(int i=0; i < _frames; ++i)
    {
        in[i*2] = rescale( _ab[i][0] );
        in[i*2+1] = rescale( _ab[i][1] );
    }

    int rc = lame_encode_buffer_interleaved( m_lgf, in, _frames, out, bufSize);

    switch(rc){
        case -1:
            printf("AudioFileMp3: encode error: buffer too small.\n");
            return;
        case -2: 
            printf("AudioFileMp3: encode error: out of memory\n");
            return;
        case -3: 
            printf("AudioFileMp3: encode error: lame_init_params not called\n");
            return;
        case -4: 
            printf("AudioFileMp3: encode error: psycho acoustic problems\n");
            return;
    }

    // write to file
    m_outfile->write( (const char *) out, rc );

    // clean up
    delete[] out;
    delete[] in;
}






