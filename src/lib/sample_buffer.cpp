#ifndef SINGLE_SOURCE_COMPILE

/*
 * sample_buffer.cpp - container-class sampleBuffer
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMutex>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>

#else

#include <qpainter.h>
#include <qmutex.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qbuffer.h>

#if QT_VERSION < 0x030100
#include <qregexp.h>
#endif

#endif


#include <cstring>

#ifdef SDL_SDL_SOUND_H
#include SDL_SDL_SOUND_H
#endif

#ifdef HAVE_SNDFILE_H
#include <sndfile.h>
#endif

#ifdef HAVE_VORBIS_VORBISFILE_H
#include <vorbis/vorbisfile.h>
#endif

#ifdef HAVE_FLAC_STREAM_ENCODER_H
#include <FLAC/stream_encoder.h>
#endif

#ifdef HAVE_FLAC_STREAM_DECODER_H
#include <FLAC/stream_decoder.h>
#endif


#include "sample_buffer.h"
#include "interpolation.h"
#include "templates.h"
#include "config_mgr.h"
#include "endian_handling.h"
#include "base64.h"
#include "debug.h"


#ifndef QT4

#define write writeBlock
#define read readBlock
#define pos at

#endif



sampleBuffer::sampleBuffer( engine * _engine, const QString & _audio_file,
							bool _is_base64_data ) :
	QObject(),
	engineObject( _engine ),
	m_audioFile( ( _is_base64_data == TRUE ) ? "" : _audio_file ),
	m_origData( NULL ),
	m_origFrames( 0 ),
	m_data( NULL ),
	m_frames( 0 ),
	m_startFrame( 0 ),
	m_endFrame( 0 ),
	m_amplification( 1.0f ),
	m_reversed( FALSE ),
	m_dataMutex()
{
#ifdef SDL_SDL_SOUND_H
	// init sound-file-system of SDL
	Sound_Init();
#endif
#ifdef HAVE_SAMPLERATE_H
	initResampling();
#endif
	if( _is_base64_data == TRUE )
	{
		loadFromBase64( _audio_file );
	}
	update();
}




sampleBuffer::sampleBuffer( const sampleFrame * _data, const f_cnt_t _frames,
							engine * _engine ) :
	QObject(),
	engineObject( _engine ),
	m_audioFile( "" ),
	m_origData( NULL ),
	m_origFrames( 0 ),
	m_data( NULL ),
	m_frames( 0 ),
	m_startFrame( 0 ),
	m_endFrame( 0 ),
	m_amplification( 1.0f ),
	m_reversed( FALSE ),
	m_dataMutex()
{
	m_origData = new sampleFrame[_frames];
	memcpy( m_origData, _data, _frames * BYTES_PER_FRAME );
	m_origFrames = _frames;
#ifdef SDL_SDL_SOUND_H
	// init sound-file-system of SDL
	Sound_Init();
#endif
#ifdef HAVE_SAMPLERATE_H
	initResampling();
#endif
	update();
}




sampleBuffer::sampleBuffer( const f_cnt_t _frames, engine * _engine ) :
	QObject(),
	engineObject( _engine ),
	m_audioFile( "" ),
	m_origData( NULL ),
	m_origFrames( 0 ),
	m_data( NULL ),
	m_frames( 0 ),
	m_startFrame( 0 ),
	m_endFrame( 0 ),
	m_amplification( 1.0f ),
	m_reversed( FALSE ),
	m_dataMutex()
{
	m_origData = new sampleFrame[_frames];
	memset( m_origData, 0, _frames * BYTES_PER_FRAME );
	m_origFrames = _frames;
#ifdef SDL_SDL_SOUND_H
	// init sound-file-system of SDL
	Sound_Init();
#endif
#ifdef HAVE_SAMPLERATE_H
	initResampling();
#endif
	update();
}




sampleBuffer::~sampleBuffer()
{
	m_dataMutex.lock();
	delete[] m_origData;
	m_origData = NULL;
	delete[] m_data;
	m_data = NULL;

#ifdef HAVE_SAMPLERATE_H
	quitResampling();
#endif

	m_dataMutex.unlock();
}






void sampleBuffer::update( bool _keep_settings )
{
	m_dataMutex.lock();

	delete[] m_data;
	m_data = NULL;
	m_frames = 0;

	if( m_audioFile == "" && m_origData != NULL && m_origFrames > 0 )
	{
		// TODO: reverse- and amplification-property is not covered
		// by following code...
		m_data = new sampleFrame[m_origFrames];
		memcpy( m_data, m_origData, m_origFrames * BYTES_PER_FRAME );
		if( _keep_settings == FALSE )
		{
			m_frames = m_origFrames;
			m_startFrame = 0;
			if( m_frames > 0 )
			{
				m_endFrame = m_frames - 1;
			}
			else
			{
				m_endFrame = 0;
			}
		}
	}
	else if( m_audioFile != "" )
	{
		QString file = m_audioFile;
		// if there's not an absolute filename, we assume that we made
		// it relative before and so we have to add sample-dir to file-
		// name
		if( file[0] != '/' )
		{
			file = configManager::inst()->userSamplesDir() + file;
			if( QFileInfo( file ).exists() == FALSE )
			{
				file =
		configManager::inst()->factorySamplesDir() + m_audioFile;
			}
		}
		const char * f =
#ifdef QT4
				file.toAscii().constData();
#else
				file.ascii();
#endif
		int_sample_t * buf = NULL;
		ch_cnt_t channels = DEFAULT_CHANNELS;
		sample_rate_t samplerate = SAMPLE_RATES[DEFAULT_QUALITY_LEVEL];

#ifdef HAVE_SNDFILE_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleSF( f, buf, channels,
								samplerate );
		}
#endif
#ifdef HAVE_VORBIS_VORBISFILE_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleOGGVorbis( f, buf, channels,
								samplerate );
		}
#endif
#ifdef SDL_SDL_SOUND_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleSDL( f, buf, channels,
								samplerate );
		}
#endif
		if( m_frames > 0 && buf != NULL )
		{
			// following code transforms int-samples into
			// float-samples and does amplifying & reversing
			const float fac = m_amplification /
						OUTPUT_SAMPLE_MULTIPLIER;
			m_data = new sampleFrame[m_frames];

			// if reversing is on, we also reverse when
			// scaling
			if( m_reversed )
			{
				for( f_cnt_t frame = 0; frame < m_frames;
								++frame )
				{
					for( ch_cnt_t chnl = 0;
							chnl < DEFAULT_CHANNELS;
									++chnl )
					{
const f_cnt_t idx = ( m_frames - frame ) * channels + ( chnl % channels );
m_data[frame][chnl] = buf[idx] * fac;
					}
				}
			}
			else
			{
				for( f_cnt_t frame = 0; frame < m_frames;
								++frame )
				{
					for( ch_cnt_t chnl = 0;
							chnl < DEFAULT_CHANNELS;
									++chnl )
					{
		const f_cnt_t idx = frame * channels + ( chnl % channels );
		m_data[frame][chnl] = buf[idx] * fac;
					}
				}
			}

			delete[] buf;

			// do samplerate-conversion if sample-decoder didn't
			// convert sample-rate to our default-samplerate
			if( samplerate != SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] )
			{
				sampleBuffer * resampled = resample( this,
								samplerate,
					SAMPLE_RATES[DEFAULT_QUALITY_LEVEL] );
				delete[] m_data;
				m_frames = resampled->frames();
				m_data = new sampleFrame[m_frames];
				memcpy( m_data, resampled->data(), m_frames *
							sizeof( sampleFrame ) );
				delete resampled;
			}

			if( _keep_settings == FALSE )
			{
				// update frame-variables
				m_startFrame = 0;
				if( m_frames > 0 )
				{
					m_endFrame = m_frames - 1;
				}
				else
				{
					m_endFrame = 0;
				}
			}
		}
		else
		{
			// sample couldn't be decoded, create buffer containing
			// one sample-frame
			m_data = new sampleFrame[1];
			memset( m_data, 0, sizeof( *m_data ) );
			m_frames = 1;
			m_startFrame = 0;
			m_endFrame = 1;
		}
	}
	else
	{
		// neither an audio-file nor a buffer to copy from, so create
		// buffer containing one sample-frame
		m_data = new sampleFrame[1];
		memset( m_data, 0, sizeof( *m_data ) * 1 );
		m_frames = 1;
		m_startFrame = 0;
		m_endFrame = 1;
	}

	m_dataMutex.unlock();

	emit sampleUpdated();
}




#ifdef SDL_SDL_SOUND_H
f_cnt_t sampleBuffer::decodeSampleSDL( const char * _f,
					int_sample_t * & _buf,
					ch_cnt_t & _channels,
					sample_rate_t & _samplerate )
{
	Sound_AudioInfo STD_AUDIO_INFO =
	{
		AUDIO_S16SYS,
		_channels,
		_samplerate,
	} ;
	f_cnt_t frames = 0;

	Sound_Sample * snd_sample = Sound_NewSampleFromFile( _f,
						&STD_AUDIO_INFO, 16384 );
	// file found?
	if( snd_sample != NULL )
	{
		// let SDL_sound decode our file to requested format
		( void )Sound_DecodeAll( snd_sample );
		_channels = snd_sample->actual.channels;
		_samplerate = snd_sample->actual.rate;
		frames = snd_sample->buffer_size / ( BYTES_PER_INT_SAMPLE *
								_channels );
		_buf = new int_sample_t[frames * _channels];
		memcpy( _buf, snd_sample->buffer, snd_sample->buffer_size );

		Sound_FreeSample( snd_sample );
	}
	return( frames );
}
#endif




#ifdef HAVE_SNDFILE_H
f_cnt_t sampleBuffer::decodeSampleSF( const char * _f,
					int_sample_t * & _buf,
					ch_cnt_t & _channels,
					sample_rate_t & _samplerate )
{
	SNDFILE * snd_file;
	SF_INFO sf_info;
	f_cnt_t frames = 0;
#ifdef OLD_SNDFILE
	if( ( snd_file = sf_open_read( _f, &sf_info ) ) != NULL )
	{
		frames = sf_info.samples;
#else
	if( ( snd_file = sf_open( _f, SFM_READ, &sf_info ) ) != NULL )
	{
		frames = sf_info.frames;
#endif
		_buf = new int_sample_t[sf_info.channels * frames];
		if( sf_read_short( snd_file, _buf, sf_info.channels * frames )
						< sf_info.channels * frames )
		{
#ifdef DEBUG_LMMS
			printf( "sampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s\n", _f, sf_strerror( NULL ) );
#endif
		}
		_channels = sf_info.channels;
		_samplerate = sf_info.samplerate;

		sf_close( snd_file );
	}
	else
	{
#ifdef DEBUG_LMMS
		printf( "sampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s\n", _f, sf_strerror( NULL ) );
#endif
	}
	return( frames );
}
#endif




#ifdef HAVE_VORBIS_VORBISFILE_H

// callback-functions for reading ogg-file

size_t qfileReadCallback( void * _ptr, size_t _size, size_t _n, void * _udata )
{
	return( static_cast<QFile *>( _udata )->read( (char*) _ptr,
								_size * _n ) );
}




int qfileSeekCallback( void * _udata, ogg_int64_t _offset, int _whence )
{
	QFile * f = static_cast<QFile *>( _udata );

	if( _whence == SEEK_CUR )
	{
		f->seek( f->pos() + _offset );
	}
	else if( _whence == SEEK_END )
	{
		f->seek( f->size() + _offset );
	}
	else
	{
		f->seek( _offset );
	}
	return( 0 );
}




int qfileCloseCallback( void * _udata )
{
	delete static_cast<QFile *>( _udata );
	return( 0 );
}




long qfileTellCallback( void * _udata )
{
	return( static_cast<QFile *>( _udata )->pos() );
}




f_cnt_t sampleBuffer::decodeSampleOGGVorbis( const char * _f,
						int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _samplerate )
{
	static ov_callbacks callbacks =
	{
		qfileReadCallback,
		qfileSeekCallback,
		qfileCloseCallback,
		qfileTellCallback
	} ;

	OggVorbis_File vf;

	f_cnt_t frames = 0;

	QFile * f = new QFile( _f );
#ifdef QT4
	if( f->open( QFile::ReadOnly | QFile::Truncate ) == FALSE )
#else
	if( f->open( IO_ReadOnly | IO_Truncate ) == FALSE )
#endif
	{
		delete f;
		return( 0 );
	}

	int err = ov_open_callbacks( f, &vf, NULL, 0, callbacks );

	if( err < 0 )
	{
		switch( err )
		{
			case OV_EREAD:
				printf( "sampleBuffer::decodeSampleOGGVorbis():"
						" media read error\n" );
				break;
			case OV_ENOTVORBIS:
/*				printf( "sampleBuffer::decodeSampleOGGVorbis():"
					" not an Ogg Vorbis file\n" );*/
				break;
			case OV_EVERSION:
				printf( "sampleBuffer::decodeSampleOGGVorbis():"
						" vorbis version mismatch\n" );
				break;
			case OV_EBADHEADER:
				printf( "sampleBuffer::decodeSampleOGGVorbis():"
					" invalid Vorbis bitstream header\n" );
				break;
			case OV_EFAULT:
				printf( "sampleBuffer::decodeSampleOgg(): "
					"internal logic fault\n" );
				break;
		}
		delete f;
		return( 0 );
	}

	ov_pcm_seek( &vf, 0 );

   	_channels = ov_info( &vf, -1 )->channels;
   	_samplerate = ov_info( &vf, -1 )->rate;

	ogg_int64_t total = ov_pcm_total( &vf, -1 );

	_buf = new int_sample_t[total * _channels];
	int bitstream = 0;
	long bytes_read = 0;

	do
	{
		bytes_read = ov_read( &vf, (char *) &_buf[frames * _channels],
					( total - frames ) * _channels *
							BYTES_PER_INT_SAMPLE,
					isLittleEndian() ? 0 : 1,
					BYTES_PER_INT_SAMPLE, 1, &bitstream );
		if( bytes_read < 0 )
		{
			break;
		}
		frames += bytes_read / ( _channels * BYTES_PER_INT_SAMPLE );
	}
	while( bytes_read != 0 && bitstream == 0 );

	ov_clear( &vf );

	return( frames );
}
#endif




#ifdef HAVE_SAMPLERATE_H
void sampleBuffer::initResampling( void )
{
	m_srcState = createResamplingContext();
	m_srcData.end_of_input = 0;
}




void sampleBuffer::quitResampling( void )
{
	destroyResamplingContext( m_srcState );
}




SRC_STATE * sampleBuffer::createResamplingContext( void )
{
	int error;
	SRC_STATE * state;
	if( ( state = src_new(/*
		( eng()->getMixer()->highQuality() == TRUE ) ?
					SRC_SINC_FASTEST :*/
					SRC_LINEAR,
					DEFAULT_CHANNELS, &error ) ) == NULL )
	{
		printf( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
	return( state );
}




void sampleBuffer::destroyResamplingContext( SRC_STATE * _context )
{
	src_delete( _context );
}
#endif




bool FASTCALL sampleBuffer::play( sampleFrame * _ab,
					const f_cnt_t _start_frame,
					const fpab_t _frames,
					const float _freq,
					const bool _looped,
					void * * _resampling_data )
{
	eng()->getMixer()->clearAudioBuffer( _ab, _frames );

	if( m_data == NULL || m_frames == 0 || m_endFrame == 0 || _frames == 0 )
	{
		return( FALSE );
	}

	const double freq_factor = (double) _freq / (double) BASE_FREQ;
	const Sint16 freq_diff = static_cast<Sint16>( BASE_FREQ - _freq );

	fpab_t frames_to_process = _frames;

	// calculate how many frames we have in requested pitch
	const f_cnt_t total_frames_for_current_pitch = static_cast<f_cnt_t>( (
						m_endFrame - m_startFrame ) /
								freq_factor );
	if( total_frames_for_current_pitch == 0 )
	{
		return( FALSE );
	}

	// do we have frames left?? this is only important when not in
	// looping-mode because in looping-mode we loop to start-frame...
	if( _start_frame >= total_frames_for_current_pitch && _looped == FALSE )
	{
		return( FALSE );
	}

	// this holds the number of the first frame to play
	const f_cnt_t play_frame = m_startFrame + ( _start_frame %
					total_frames_for_current_pitch );

	// this holds the number of remaining frames in current loop
	f_cnt_t frames_for_loop = total_frames_for_current_pitch -
						( play_frame - m_startFrame );

	// make sure, data isn't accessed in any other way (e.g. deleting
	// of this buffer...)
	m_dataMutex.lock();

	if( _looped == FALSE && frames_for_loop < frames_to_process )
	{
		frames_to_process = frames_for_loop;
	}
	const f_cnt_t f1 = static_cast<f_cnt_t>( m_startFrame +
				( play_frame - m_startFrame ) * freq_factor );
/*	Uint32 f2 = 0;
	while( f2 < f1 )
	{
		f2 += frames_to_process * freq_factor;
	}
	if( f2 > f1 && f2 >= frames_to_process )
	{
		f2 -= frames_to_process * freq_factor;
	}*/
//	static int foo = 0;
	// calc pointer of first frame
	sampleFrame * start_frame = (sampleFrame *) m_data + f1;
	//printf("diff:%d %f  %d f2: %d  input: %d\n", f2 -foo, play_frame * freq_factor, static_cast<Uint32>( play_frame * freq_factor ), f2, (Uint32)( frames_for_loop * freq_factor ) );
//	foo = f2;
	sampleFrame * loop_start = (sampleFrame *) m_data + m_startFrame;

	// check whether we have to change pitch...
	if( freq_diff != 0 )
	{
#ifdef HAVE_SAMPLERATE_H
		SRC_STATE * state = m_srcState;
		if( _resampling_data != NULL )
		{
			if( _start_frame == 0 )
			{
				*_resampling_data = createResamplingContext();
			}
			state = static_cast<SRC_STATE *>( *_resampling_data );
		}

		// Check loop
		if( _looped && frames_for_loop < frames_to_process )
		{
			f_cnt_t total_frames_copied = 0;
			while( total_frames_copied < frames_to_process )
			{
				// Generate output
				m_srcData.data_in = start_frame[0];
				m_srcData.data_out = _ab[total_frames_copied];
				m_srcData.input_frames = static_cast<f_cnt_t>(
						frames_for_loop * freq_factor );
				m_srcData.output_frames = frames_for_loop;
				m_srcData.src_ratio = 1.0 / freq_factor;
				int error = src_process( state, &m_srcData );
				if( error )
				{
					printf( "sampleBuffer: error while "
							"resampling: %s\n",
							src_strerror( error ) );
				}
				// Advance
				total_frames_copied += frames_for_loop;

				// reset start_frame to start
				start_frame = loop_start;
				// and calculate frames for next loop
				frames_for_loop = frames_to_process
							- total_frames_copied;
				if( frames_for_loop
					> total_frames_for_current_pitch )
				{
					frames_for_loop =
						total_frames_for_current_pitch;
				}
			}
		}
		else
		{
			// Generate output
			m_srcData.data_in = start_frame[0];
			m_srcData.data_out = _ab[0];
			m_srcData.input_frames = static_cast<f_cnt_t>(
						frames_for_loop * freq_factor );
			m_srcData.output_frames = frames_to_process;
			m_srcData.src_ratio = 1.0 / freq_factor;
			int error = src_process( state, &m_srcData );
			if( error )
			{
				printf( "sampleBuffer: error while resampling: "
						"%s\n", src_strerror( error ) );
			}
		}
#else
		f_cnt_t src_frame_base = 0;
		// check whether we're in high-quality-mode
		if( eng()->getMixer()->highQuality() == TRUE )
		{
			// we are, so let's use cubic interpolation...
			for( f_cnt_t frame = 0; frame < frames_to_process;
								++frame )
			{
				// current loop done?
				if( _looped && ( frame-src_frame_base ) >
							frames_for_loop )
				{
					start_frame = loop_start;
					src_frame_base = frame;
					frames_for_loop = frames_to_process %
						total_frames_for_current_pitch;
				}

				const float src_frame_idx = frame * freq_factor;
				f_cnt_t frame_num = static_cast<f_cnt_t>(
						src_frame_idx) - src_frame_base;
				const float frac_pos = src_frame_idx -
					static_cast<f_cnt_t>( src_frame_idx );

				// because of cubic interpolation we have to
				// access start_frame[frame_num-1], so make
				// sure we don't access data out of
				// buffer-array-boundaries
				if( frame_num == 0 && play_frame == 0 )
				{
					frame_num = 1;
				}
				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
				{
					_ab[frame][chnl] = cubicInterpolate(
						start_frame[frame_num-1][chnl],
						start_frame[frame_num+0][chnl],
						start_frame[frame_num+1][chnl],
						start_frame[frame_num+2][chnl],
								frac_pos );
				}
			}
		}
		else
		{
			// just normal mode, so we can use linear
			// interpolation...
			for( f_cnt_t frame = 0; frame < frames_to_process;
								++frame )
			{
				if( _looped && ( frame - src_frame_base ) >
							frames_for_loop )
				{
					start_frame = loop_start;
					src_frame_base = frame;
					frames_for_loop = frames_to_process %
						total_frames_for_current_pitch;
				}
				const float src_frame_idx = frame * freq_factor;
				const f_cnt_t frame_num =
					(f_cnt_t)src_frame_idx-src_frame_base;
				const float frac_pos = src_frame_idx -
							(f_cnt_t) src_frame_idx;
				for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
				{
					_ab[frame][chnl] = linearInterpolate(
						start_frame[frame_num][chnl],
						start_frame[frame_num+1][chnl],
								frac_pos );
				}
			}
		}
#endif
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Check loop
		if( _looped && frames_for_loop < frames_to_process )
		{
			f_cnt_t total_frames_copied = 0;
			while( total_frames_copied < frames_to_process )
			{
				// Generate output
				memcpy( _ab[total_frames_copied], start_frame,
					frames_for_loop * BYTES_PER_FRAME );
				// Advance
				total_frames_copied += frames_for_loop;

				// reset start_frame to start
				start_frame = loop_start;
				// and calculate frames for next loop
				frames_for_loop = frames_to_process
							- total_frames_copied;
				if( frames_for_loop
					> total_frames_for_current_pitch )
				{
					frames_for_loop =
						total_frames_for_current_pitch;
				}
			}
		}
		else
		{
			// Generate output
			memcpy( _ab, start_frame,
					frames_to_process * BYTES_PER_FRAME );
		}
	}

	m_dataMutex.unlock();

	return( TRUE );

}




void sampleBuffer::visualize( QPainter & _p, const QRect & _dr,
					const QRect & _clip, drawMethods _dm )
{
//	_p.setClipRect( _clip );
//	_p.setPen( QColor( 0x22, 0xFF, 0x44 ) );
	//_p.setPen( QColor( 64, 224, 160 ) );
#ifdef QT4
	// TODO: save and restore aa-settings
	_p.setRenderHint( QPainter::Antialiasing );
#endif
	const int w = _dr.width();
	const int h = _dr.height();

	const Uint16 y_base = h / 2 + _dr.y();
	const float y_space = h / 2;

	const QRect isect = _dr.intersect( _clip );

	if( m_data == NULL || m_frames == 0 )
	{
		_p.drawLine( isect.x(), y_base, isect.right(), y_base );
		return;
	}
	else if( _dm == LINE_CONNECT )
	{
#ifdef QT4
		float old_x = _dr.x();
		float old_y[DEFAULT_CHANNELS] = { y_base, y_base };
	
		const float fpp = tMax<float>( tMin<float>( m_frames / (float)w,
								20.0f ), 1.0f );
		const float fmax = tMin<float>( m_frames, isect.right() * fpp );
		for( float frame = fpp * _clip.x(); frame < fmax; frame += fpp )
		{
			const float x = frame*w / m_frames + _dr.x();
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				const float y = y_base +
			m_data[static_cast<int>( frame )][chnl] * y_space;
				_p.drawLine(
					QLineF( old_x, old_y[chnl], x, y ) );
				old_y[chnl] = y;
			}
			old_x = x;
		}
#else
		int old_y[DEFAULT_CHANNELS] = { y_base, y_base };
	
		const f_cnt_t fpp = tMax<f_cnt_t>( tMin<f_cnt_t>( m_frames / w,
								20 ), 1 );
		const f_cnt_t fbase = m_frames * _clip.x() / _clip.width();
		const f_cnt_t fmax = tMin<f_cnt_t>( m_frames,
							_clip.width() * fpp );
		int old_x = _clip.x();
		//printf("%d\n", fmax );
		for( f_cnt_t frame = 0; frame < m_frames; frame += fpp )
		{
			const int x = _dr.x() + static_cast<int>( frame /
						(float) m_frames * _dr.width() );
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				const Uint16 y = y_base +
			static_cast<Uint16>( m_data[frame][chnl] *
								y_space );
				_p.drawLine( old_x, old_y[chnl], x, y );
				old_y[chnl] = y;
			}
			old_x = x;
		}
#endif

	}
	else if( _dm == DOTS )
	{
		for( f_cnt_t frame = 0; frame < m_frames; ++frame )
		{
			const int x = frame * w / m_frames + _dr.x();
			for( ch_cnt_t chnl = 0; chnl < DEFAULT_CHANNELS;
									++chnl )
			{
				_p.drawPoint( x, y_base +
			static_cast<Uint16>( m_data[frame][chnl] * y_space ) );
			}
		}
	}
//	_p.setClipping( FALSE );
}




QString sampleBuffer::openAudioFile( void ) const
{
#ifdef QT4
	QFileDialog ofd( NULL, tr( "Open audio file" ) );
#else
	QFileDialog ofd( QString::null, QString::null, NULL, "", TRUE );
	ofd.setWindowTitle( tr( "Open audio file" ) );
#endif

	QString dir;
	if( m_audioFile != "" )
	{
		QString f = m_audioFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = configManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == FALSE )
			{
				f = configManager::inst()->factorySamplesDir() +
								m_audioFile;
			}
		}
#ifdef QT4
		dir = QFileInfo( f ).absolutePath();
#else
		dir = QFileInfo( f ).dirPath( TRUE );
#endif
	}
	else
	{
		dir = configManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "All Audio-Files (*.wav *.ogg *.flac *.voc *.aif *.aiff "
								"*.au *.raw)" )
		<< tr( "Wave-Files (*.wav)" )
		<< tr( "OGG-Files (*.ogg)" )
		<< tr( "FLAC-Files (*.flac)" )
		//<< tr( "MP3-Files (*.mp3)" )
		//<< tr( "MIDI-Files (*.mid)" )
		<< tr( "VOC-Files (*.voc)" )
		<< tr( "AIFF-Files (*.aif *.aiff)" )
		<< tr( "AU-Files (*.au)" )
		<< tr( "RAW-Files (*.raw)" )
		//<< tr( "MOD-Files (*.mod)" )
		;
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "All Audio-Files (*.wav *.ogg *.flac *.voc *.aif "
						"*.aiff *.au *.raw)" ) );
	ofd.addFilter( tr( "Wave-Files (*.wav)" ) );
	ofd.addFilter( tr( "OGG-Files (*.ogg)" ) );
	ofd.addFilter( tr( "FLAC-Files (*.flac)" ) );
	//ofd.addFilter (tr("MP3-Files (*.mp3)"));
	//ofd.addFilter (tr("MIDI-Files (*.mid)"));^
	ofd.addFilter( tr( "VOC-Files (*.voc)" ) );
	ofd.addFilter( tr( "AIFF-Files (*.aif *.aiff)" ) );
	ofd.addFilter( tr( "AU-Files (*.au)" ) );
	ofd.addFilter( tr( "RAW-Files (*.raw)" ) );
	//ofd.addFilter (tr("MOD-Files (*.mod)"));
	ofd.setSelectedFilter( tr( "All Audio-Files (*.wav *.ogg *.flac *.voc "
						"*.aif *.aiff *.au *.raw)" ) );
#endif
	if( m_audioFile != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_audioFile ).fileName() );
	}

	if( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return( "" );
		}
		return( tryToMakeRelative( ofd.selectedFiles()[0] ) );
	}

	return( "" );
}


#undef HAVE_FLAC_STREAM_ENCODER_H	/* not yet... */
#undef HAVE_FLAC_STREAM_DECODER_H

#ifdef HAVE_FLAC_STREAM_ENCODER_H
FLAC__StreamEncoderWriteStatus flacStreamEncoderWriteCallback(
					const FLAC__StreamEncoder *
								/*_encoder*/,
					const FLAC__byte _buffer[], 
					unsigned int/* _samples*/,
					unsigned int _bytes,
					unsigned int/* _current_frame*/,
					void * _client_data )
{
/*	if( _bytes == 0 )
	{
		return( FLAC__STREAM_ENCODER_WRITE_STATUS_OK );
	}*/
	return( ( static_cast<QBuffer *>( _client_data )->write(
				(const char *) _buffer, _bytes ) ==
								(int) _bytes ) ?
				FLAC__STREAM_ENCODER_WRITE_STATUS_OK :
				FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR );
}


void flacStreamEncoderMetadataCallback( const FLAC__StreamEncoder *,
					const FLAC__StreamMetadata * _metadata,
					void * _client_data )
{
	QBuffer * b = static_cast<QBuffer *>( _client_data );
	b->seek( 0 );
	b->write( (const char *) _metadata, sizeof( *_metadata ) );
}

#endif



QString & sampleBuffer::toBase64( QString & _dst ) const
{
	if( m_data == NULL || m_frames == 0 )
	{
		return( _dst = "" );
	}

#ifdef HAVE_FLAC_STREAM_ENCODER_H
	const f_cnt_t FRAMES_PER_BUF = 1152;

	FLAC__StreamEncoder * flac_enc = FLAC__stream_encoder_new();
	FLAC__stream_encoder_set_channels( flac_enc, DEFAULT_CHANNELS );
	FLAC__stream_encoder_set_blocksize( flac_enc, FRAMES_PER_BUF );
/*	FLAC__stream_encoder_set_do_exhaustive_model_search( flac_enc, TRUE );
	FLAC__stream_encoder_set_do_mid_side_stereo( flac_enc, TRUE );*/
	FLAC__stream_encoder_set_sample_rate( flac_enc,
					eng()->getMixer()->sampleRate() );
	QBuffer ba_writer;
#ifdef QT4
	ba_writer.open( QBuffer::WriteOnly );
#else
	ba_writer.open( IO_WriteOnly );
#endif

	FLAC__stream_encoder_set_write_callback( flac_enc,
					flacStreamEncoderWriteCallback );
	FLAC__stream_encoder_set_metadata_callback( flac_enc,
					flacStreamEncoderMetadataCallback );
	FLAC__stream_encoder_set_client_data( flac_enc, &ba_writer );
	if( FLAC__stream_encoder_init( flac_enc ) != FLAC__STREAM_ENCODER_OK )
	{
		printf( "error within FLAC__stream_encoder_init()!\n" );
	}
	f_cnt_t frame_cnt = 0;
	while( frame_cnt < m_frames )
	{
		f_cnt_t remaining = tMin<f_cnt_t>( FRAMES_PER_BUF,
							m_frames - frame_cnt );
		FLAC__int32 buf[FRAMES_PER_BUF * DEFAULT_CHANNELS];
		for( f_cnt_t f = 0; f < remaining; ++f )
		{
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				buf[f*DEFAULT_CHANNELS+ch] = (FLAC__int32)(
					mixer::clip( m_data[f+frame_cnt][ch] ) *
						OUTPUT_SAMPLE_MULTIPLIER );
			}
		}
		FLAC__stream_encoder_process_interleaved( flac_enc, buf,
								remaining );
		frame_cnt += remaining;
	}
	FLAC__stream_encoder_finish( flac_enc );
	FLAC__stream_encoder_delete( flac_enc );
	printf("%d %d\n", frame_cnt, (int)ba_writer.size() );
	ba_writer.close();

	base64::encode( ba_writer.buffer().data(), ba_writer.buffer().size(),
									_dst );


#else	/* HAVE_FLAC_STREAM_ENCODER_H */

	base64::encode( (const char *) m_data,
					m_frames * sizeof( sampleFrame ), _dst );

#endif	/* HAVE_FLAC_STREAM_ENCODER_H */

	return( _dst );
}




sampleBuffer * sampleBuffer::resample( sampleFrame * _data,
						const f_cnt_t _frames,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr,
						engine * _engine )
{
	const f_cnt_t dst_frames = static_cast<f_cnt_t>( _frames /
					(float) _src_sr * (float) _dst_sr );
	sampleBuffer * dst_sb = new sampleBuffer( dst_frames, _engine );
	sampleFrame * dst_buf = dst_sb->m_origData;
#ifdef HAVE_SAMPLERATE_H
	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE * state;
	if( ( state = src_new( SRC_SINC_MEDIUM_QUALITY,
					DEFAULT_CHANNELS, &error ) ) != NULL )
	{
		SRC_DATA src_data;
		src_data.end_of_input = 0;
		src_data.data_in = _data[0];
		src_data.data_out = dst_buf[0];
		src_data.input_frames = _frames;
		src_data.output_frames = dst_frames;
		src_data.src_ratio = (double) _dst_sr / _src_sr;
		int error;
		if( ( error = src_process( state, &src_data ) ) )
		{
			printf( "sampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		src_delete( state );
	}
	else
	{
		printf( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
#else
	// no libsamplerate, so do simple cubic interpolation
	for( f_cnt_t frame = 0; frame < dst_frames; ++frame )
	{
		const float src_frame_float = frame * (float) _src_sr / _dst_sr;
		const float frac_pos = src_frame_float -
					static_cast<f_cnt_t>( src_frame_float );
		const f_cnt_t src_frame = tLimit<f_cnt_t>(
					static_cast<f_cnt_t>( src_frame_float ),
							1, _frames - 3 );
		for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
		{
			dst_buf[frame][ch] = cubicInterpolate(
						_data[src_frame - 1][ch],
						_data[src_frame + 0][ch],
						_data[src_frame + 1][ch],
						_data[src_frame + 2][ch],
								frac_pos );
		}
	}
#endif
	dst_sb->update();
	return( dst_sb );
}




void sampleBuffer::setAudioFile( const QString & _audio_file )
{
	m_audioFile = tryToMakeRelative( _audio_file );
	update();
}



#ifdef HAVE_FLAC_STREAM_DECODER_H

struct flacStreamDecoderClientData
{
	QBuffer * read_buffer;
	QBuffer * write_buffer;
} ;



FLAC__StreamDecoderReadStatus flacStreamDecoderReadCallback(
					const FLAC__StreamDecoder *
								/*_decoder*/,
					FLAC__byte * _buffer,
					unsigned int * _bytes,
					void * _client_data )
{
	int res = static_cast<flacStreamDecoderClientData *>(
					_client_data )->read_buffer->read(
						(char *) _buffer, *_bytes );

	if( res > 0 )
	{
		*_bytes = res;
		return( FLAC__STREAM_DECODER_READ_STATUS_CONTINUE );

	}
	*_bytes = 0;
	return( FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM );
}




FLAC__StreamDecoderWriteStatus flacStreamDecoderWriteCallback(
					const FLAC__StreamDecoder *
								/*_decoder*/,
					const FLAC__Frame * _frame,
					const FLAC__int32 * const _buffer[],
					void * _client_data )
{
	if( _frame->header.channels != 2 )
	{
		printf( "channels != 2 in "
					"flacStreamDecoderWriteCallback()\n" );
		return( FLAC__STREAM_DECODER_WRITE_STATUS_ABORT );
	}

	if( _frame->header.bits_per_sample != 16 )
	{
		printf( "bits_per_sample != 16 in "
					"flacStreamDecoderWriteCallback()\n" );
		return( FLAC__STREAM_DECODER_WRITE_STATUS_ABORT );
	}

	const f_cnt_t frames = _frame->header.blocksize;
	for( f_cnt_t frame = 0; frame < frames; ++frame )
	{
		sampleFrame sframe = { _buffer[0][frame] /
						OUTPUT_SAMPLE_MULTIPLIER,
					_buffer[1][frame] /
						OUTPUT_SAMPLE_MULTIPLIER
					} ;
		static_cast<flacStreamDecoderClientData *>(
					_client_data )->write_buffer->write(
				(const char *) sframe, sizeof( sframe ) );
	} 
	return( FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE );
}


void flacStreamDecoderMetadataCallback( const FLAC__StreamDecoder *,
					const FLAC__StreamMetadata *,
					void * /*_client_data*/ )
{
	printf("stream decoder metadata callback\n");
/*	QBuffer * b = static_cast<QBuffer *>( _client_data );
	b->seek( 0 );
	b->write( (const char *) _metadata, sizeof( *_metadata ) );*/
}


void flacStreamDecoderErrorCallback( const FLAC__StreamDecoder *,
					FLAC__StreamDecoderErrorStatus _status,
					void * /*_client_data*/ )
{
	printf("error callback! %d\n", _status);
	// what to do now??
}

#endif


void sampleBuffer::loadFromBase64( const QString & _data )
{
	char * dst = NULL;
	int dsize = 0;
	base64::decode( _data, &dst, &dsize );

#ifdef HAVE_FLAC_STREAM_DECODER_H

#ifndef QT3
	QByteArray orig_data = QByteArray::fromRawData( dst, dsize );
	QBuffer ba_reader( &orig_data );
	ba_reader.open( QBuffer::ReadOnly );
#else
	QByteArray orig_data;
	orig_data.setRawData( dst, dsize );
	QBuffer ba_reader( orig_data );
	ba_reader.open( IO_ReadOnly );
#endif

	QBuffer ba_writer;
#ifdef QT4
	ba_writer.open( QBuffer::WriteOnly );
#else
	ba_writer.open( IO_WriteOnly );
#endif

	flacStreamDecoderClientData cdata = { &ba_reader, &ba_writer } ;

	FLAC__StreamDecoder * flac_dec = FLAC__stream_decoder_new();

	FLAC__stream_decoder_set_read_callback( flac_dec,
					flacStreamDecoderReadCallback );
	FLAC__stream_decoder_set_write_callback( flac_dec,
					flacStreamDecoderWriteCallback );
	FLAC__stream_decoder_set_error_callback( flac_dec,
					flacStreamDecoderErrorCallback );
	FLAC__stream_decoder_set_metadata_callback( flac_dec,
					flacStreamDecoderMetadataCallback );
	FLAC__stream_decoder_set_client_data( flac_dec, &cdata );

	FLAC__stream_decoder_init( flac_dec );

	FLAC__stream_decoder_process_until_end_of_stream( flac_dec );

	FLAC__stream_decoder_finish( flac_dec );
	FLAC__stream_decoder_delete( flac_dec );

	ba_reader.close();

	orig_data = ba_writer.buffer();
	printf("%d\n", (int) orig_data.size() );

	m_origFrames = orig_data.size() / sizeof( sampleFrame ); 
	delete[] m_origData;
	m_origData = new sampleFrame[m_origFrames];
	memcpy( m_origData, orig_data.data(), orig_data.size() );

#else /* HAVE_FLAC_STREAM_DECODER_H */

	m_origFrames = dsize / sizeof( sampleFrame ); 
	delete[] m_origData;
	m_origData = new sampleFrame[m_origFrames];
	memcpy( m_origData, dst, dsize );

#endif

#ifndef QT3
	delete[] dst;
#endif

	m_audioFile = "";
	update();
}




void sampleBuffer::setStartFrame( const f_cnt_t _s )
{
	// don't set this parameter while playing
	m_dataMutex.lock();
	m_startFrame = _s;
	m_dataMutex.unlock();
}




void sampleBuffer::setEndFrame( const f_cnt_t _e )
{
	// don't set this parameter while playing
	m_dataMutex.lock();
	m_endFrame = _e;
	m_dataMutex.unlock();
}




void sampleBuffer::setAmplification( float _a )
{
	m_amplification = _a;
	update( TRUE );
}




void sampleBuffer::setReversed( bool _on )
{
	m_reversed = _on;
	update( TRUE );
}




void sampleBuffer::deleteResamplingData( void * * _ptr )
{
#ifdef HAVE_SAMPLERATE_H
#ifdef LMMS_DEBUG
	assert( _ptr != NULL );
	assert( *_ptr != NULL );
#endif
	destroyResamplingContext( static_cast<SRC_STATE *>( *_ptr ) );
#endif
	*_ptr = NULL;
}




QString sampleBuffer::tryToMakeRelative( const QString & _file )
{
	if( QFileInfo( _file ).isRelative() == FALSE )
	{
		QString fsd = configManager::inst()->factorySamplesDir();
		QString usd = configManager::inst()->userSamplesDir();
		if( _file.contains( fsd ) )
		{
			return( QString( _file ).replace( fsd, "" ) );
		}
		else if( _file.contains( usd ) )
		{
			return( QString( _file ).replace( usd, "" ) );
		}
	}
	return( _file );
}



#undef write
#undef read
#undef pos


#include "sample_buffer.moc"


#endif
