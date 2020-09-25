/*
 * SampleBuffer.cpp - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include "SampleBuffer.h"

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPainter>


#include <sndfile.h>

#define OV_EXCLUDE_STATIC_CALLBACKS
#ifdef LMMS_HAVE_OGGVORBIS
#include <vorbis/vorbisfile.h>
#endif

#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
#include <FLAC/stream_encoder.h>
#endif

#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H
#include <FLAC/stream_decoder.h>
#endif


#include "base64.h"
#include "ConfigManager.h"
#include "DrumSynth.h"
#include "endian_handling.h"
#include "Engine.h"
#include "GuiApplication.h"
#include "Mixer.h"
#include "PathUtil.h"

#include "FileDialog.h"


SampleBuffer::SampleBuffer() :
	m_audioFile( "" ),
	m_origData( NULL ),
	m_origFrames( 0 ),
	m_data( NULL ),
	m_frames( 0 ),
	m_startFrame( 0 ),
	m_endFrame( 0 ),
	m_loopStartFrame( 0 ),
	m_loopEndFrame( 0 ),
	m_amplification( 1.0f ),
	m_reversed( false ),
	m_frequency( BaseFreq ),
	m_sampleRate( mixerSampleRate () )
{

	connect( Engine::mixer(), SIGNAL( sampleRateChanged() ), this, SLOT( sampleRateChanged() ) );
	update();
}



SampleBuffer::SampleBuffer( const QString & _audio_file,
							bool _is_base64_data )
	: SampleBuffer()
{
	if( _is_base64_data )
	{
		loadFromBase64( _audio_file );
	}
	else
	{
		m_audioFile = _audio_file;
		update();
	}
}




SampleBuffer::SampleBuffer( const sampleFrame * _data, const f_cnt_t _frames )
	: SampleBuffer()
{
	if( _frames > 0 )
	{
		m_origData = MM_ALLOC( sampleFrame, _frames );
		memcpy( m_origData, _data, _frames * BYTES_PER_FRAME );
		m_origFrames = _frames;
		update();
	}
}




SampleBuffer::SampleBuffer( const f_cnt_t _frames )
	: SampleBuffer()
{
	if( _frames > 0 )
	{
		m_origData = MM_ALLOC( sampleFrame, _frames );
		memset( m_origData, 0, _frames * BYTES_PER_FRAME );
		m_origFrames = _frames;
		update();
	}
}




SampleBuffer::~SampleBuffer()
{
	MM_FREE( m_origData );
	MM_FREE( m_data );
}



void SampleBuffer::sampleRateChanged()
{
	update( true );
}

sample_rate_t SampleBuffer::mixerSampleRate()
{
	return Engine::mixer()->processingSampleRate();
}


void SampleBuffer::update( bool _keep_settings )
{
	const bool lock = ( m_data != NULL );
	if( lock )
	{
		Engine::mixer()->requestChangeInModel();
		m_varLock.lockForWrite();
		MM_FREE( m_data );
	}

	// File size and sample length limits
	const int fileSizeMax = 300; // MB
	const int sampleLengthMax = 90; // Minutes

	bool fileLoadError = false;
	if( m_audioFile.isEmpty() && m_origData != NULL && m_origFrames > 0 )
	{
		// TODO: reverse- and amplification-property is not covered
		// by following code...
		m_data = MM_ALLOC( sampleFrame, m_origFrames );
		memcpy( m_data, m_origData, m_origFrames * BYTES_PER_FRAME );
		if( _keep_settings == false )
		{
			m_frames = m_origFrames;
			m_loopStartFrame = m_startFrame = 0;
			m_loopEndFrame = m_endFrame = m_frames;
		}
	}
	else if( !m_audioFile.isEmpty() )
	{
		QString file = PathUtil::toAbsolute( m_audioFile );
		int_sample_t * buf = NULL;
		sample_t * fbuf = NULL;
		ch_cnt_t channels = DEFAULT_CHANNELS;
		sample_rate_t samplerate = mixerSampleRate();
		m_frames = 0;

		const QFileInfo fileInfo( file );
		if( fileInfo.size() > fileSizeMax * 1024 * 1024 )
		{
			fileLoadError = true;
		}
		else
		{
			// Use QFile to handle unicode file names on Windows
			QFile f(file);
			f.open(QIODevice::ReadOnly);
			SNDFILE * snd_file;
			SF_INFO sf_info;
			sf_info.format = 0;
			if( ( snd_file = sf_open_fd( f.handle(), SFM_READ, &sf_info, false ) ) != NULL )
			{
				f_cnt_t frames = sf_info.frames;
				int rate = sf_info.samplerate;
				if( frames / rate > sampleLengthMax * 60 )
				{
					fileLoadError = true;
				}
				sf_close( snd_file );
			}
			f.close();
		}

		if( !fileLoadError )
		{
#ifdef LMMS_HAVE_OGGVORBIS
			// workaround for a bug in libsndfile or our libsndfile decoder
			// causing some OGG files to be distorted -> try with OGG Vorbis
			// decoder first if filename extension matches "ogg"
			if( m_frames == 0 && fileInfo.suffix() == "ogg" )
			{
				m_frames = decodeSampleOGGVorbis( file, buf, channels, samplerate );
			}
#endif
			if( m_frames == 0 )
			{
				m_frames = decodeSampleSF( file, fbuf, channels,
									samplerate );
			}
#ifdef LMMS_HAVE_OGGVORBIS
			if( m_frames == 0 )
			{
				m_frames = decodeSampleOGGVorbis( file, buf, channels,
									samplerate );
			}
#endif
			if( m_frames == 0 )
			{
				m_frames = decodeSampleDS( file, buf, channels,
									samplerate );
			}
		}

		if ( m_frames == 0 || fileLoadError )  // if still no frames, bail
		{
			// sample couldn't be decoded, create buffer containing
			// one sample-frame
			m_data = MM_ALLOC( sampleFrame, 1 );
			memset( m_data, 0, sizeof( *m_data ) );
			m_frames = 1;
			m_loopStartFrame = m_startFrame = 0;
			m_loopEndFrame = m_endFrame = 1;
		}
		else // otherwise normalize sample rate
		{
			normalizeSampleRate( samplerate, _keep_settings );
		}
	}
	else
	{
		// neither an audio-file nor a buffer to copy from, so create
		// buffer containing one sample-frame
		m_data = MM_ALLOC( sampleFrame, 1 );
		memset( m_data, 0, sizeof( *m_data ) );
		m_frames = 1;
		m_loopStartFrame = m_startFrame = 0;
		m_loopEndFrame = m_endFrame = 1;
	}

	if( lock )
	{
		m_varLock.unlock();
		Engine::mixer()->doneChangeInModel();
	}

	emit sampleUpdated();

	if( fileLoadError )
	{
		QString title = tr( "Fail to open file" );
		QString message = tr( "Audio files are limited to %1 MB "
				"in size and %2 minutes of playing time"
				).arg( fileSizeMax ).arg( sampleLengthMax );
		if( gui )
		{
			QMessageBox::information( NULL,
				title, message,	QMessageBox::Ok );
		}
		else
		{
			fprintf( stderr, "%s\n", message.toUtf8().constData() );
		}
	}
}


void SampleBuffer::convertIntToFloat ( int_sample_t * & _ibuf, f_cnt_t _frames, int _channels)
{
	// following code transforms int-samples into
	// float-samples and does amplifying & reversing
	const float fac = 1 / OUTPUT_SAMPLE_MULTIPLIER;
	m_data = MM_ALLOC( sampleFrame, _frames );
	const int ch = ( _channels > 1 ) ? 1 : 0;

	// if reversing is on, we also reverse when
	// scaling
	if( m_reversed )
	{
		int idx = ( _frames - 1 ) * _channels;
		for( f_cnt_t frame = 0; frame < _frames;
						++frame )
		{
			m_data[frame][0] = _ibuf[idx+0] * fac;
			m_data[frame][1] = _ibuf[idx+ch] * fac;
			idx -= _channels;
		}
	}
	else
	{
		int idx = 0;
		for( f_cnt_t frame = 0; frame < _frames;
						++frame )
		{
			m_data[frame][0] = _ibuf[idx+0] * fac;
			m_data[frame][1] = _ibuf[idx+ch] * fac;
			idx += _channels;
		}
	}

	delete[] _ibuf;
}

void SampleBuffer::directFloatWrite ( sample_t * & _fbuf, f_cnt_t _frames, int _channels)

{

	m_data = MM_ALLOC( sampleFrame, _frames );
	const int ch = ( _channels > 1 ) ? 1 : 0;

	// if reversing is on, we also reverse when
	// scaling
	if( m_reversed )
	{
		int idx = ( _frames - 1 ) * _channels;
		for( f_cnt_t frame = 0; frame < _frames;
						++frame )
		{
			m_data[frame][0] = _fbuf[idx+0];
			m_data[frame][1] = _fbuf[idx+ch];
			idx -= _channels;
		}
	}
	else
	{
		int idx = 0;
		for( f_cnt_t frame = 0; frame < _frames;
						++frame )
		{
			m_data[frame][0] = _fbuf[idx+0];
			m_data[frame][1] = _fbuf[idx+ch];
			idx += _channels;
		}
	}

	delete[] _fbuf;
}


void SampleBuffer::normalizeSampleRate( const sample_rate_t _src_sr,
							bool _keep_settings )
{
	const sample_rate_t old_rate = m_sampleRate;
	// do samplerate-conversion to our default-samplerate
	if( _src_sr != mixerSampleRate() )
	{
		SampleBuffer * resampled = resample( _src_sr,
					mixerSampleRate() );

		m_sampleRate = mixerSampleRate();
		MM_FREE( m_data );
		m_frames = resampled->frames();
		m_data = MM_ALLOC( sampleFrame, m_frames );
		memcpy( m_data, resampled->data(), m_frames *
							sizeof( sampleFrame ) );
		delete resampled;
	}

	if( _keep_settings == false )
	{
		// update frame-variables
		m_loopStartFrame = m_startFrame = 0;
		m_loopEndFrame = m_endFrame = m_frames;
	}
	else if( old_rate != mixerSampleRate() )
	{
		auto old_rate_to_new_rate_ratio = static_cast<float>(mixerSampleRate()) / old_rate;

		m_startFrame = qBound(0, f_cnt_t(m_startFrame*old_rate_to_new_rate_ratio), m_frames);
		m_endFrame = qBound(m_startFrame, f_cnt_t(m_endFrame*old_rate_to_new_rate_ratio), m_frames);
		m_loopStartFrame = qBound(0, f_cnt_t(m_loopStartFrame*old_rate_to_new_rate_ratio), m_frames);
		m_loopEndFrame = qBound(m_loopStartFrame, f_cnt_t(m_loopEndFrame*old_rate_to_new_rate_ratio), m_frames);
		m_sampleRate = mixerSampleRate();
	}
}




f_cnt_t SampleBuffer::decodeSampleSF(QString _f,
					sample_t * & _buf,
					ch_cnt_t & _channels,
					sample_rate_t & _samplerate )
{
	SNDFILE * snd_file;
	SF_INFO sf_info;
	sf_info.format = 0;
	f_cnt_t frames = 0;
	bool sf_rr = false;


	// Use QFile to handle unicode file names on Windows
	QFile f(_f);
	f.open(QIODevice::ReadOnly);
	if( ( snd_file = sf_open_fd( f.handle(), SFM_READ, &sf_info, false ) ) != NULL )
	{
		frames = sf_info.frames;

		_buf = new sample_t[sf_info.channels * frames];
		sf_rr = sf_read_float( snd_file, _buf, sf_info.channels * frames );

		if( sf_rr < sf_info.channels * frames )
		{
#ifdef DEBUG_LMMS
			qDebug( "SampleBuffer::decodeSampleSF(): could not read"
				" sample %s: %s", _f, sf_strerror( NULL ) );
#endif
		}
		_channels = sf_info.channels;
		_samplerate = sf_info.samplerate;

		sf_close( snd_file );
	}
	else
	{
#ifdef DEBUG_LMMS
		qDebug( "SampleBuffer::decodeSampleSF(): could not load "
				"sample %s: %s", _f, sf_strerror( NULL ) );
#endif
	}
	f.close();

	//write down either directly or convert i->f depending on file type

	if ( frames > 0 && _buf != NULL )
	{
		directFloatWrite ( _buf, frames, _channels);
	}

	return frames;
}




#ifdef LMMS_HAVE_OGGVORBIS

// callback-functions for reading ogg-file

size_t qfileReadCallback( void * _ptr, size_t _size, size_t _n, void * _udata )
{
	return static_cast<QFile *>( _udata )->read( (char*) _ptr,
								_size * _n );
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
	return 0;
}




int qfileCloseCallback( void * _udata )
{
	delete static_cast<QFile *>( _udata );
	return 0;
}




long qfileTellCallback( void * _udata )
{
	return static_cast<QFile *>( _udata )->pos();
}




f_cnt_t SampleBuffer::decodeSampleOGGVorbis( QString _f,
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
	if( f->open( QFile::ReadOnly ) == false )
	{
		delete f;
		return 0;
	}

	int err = ov_open_callbacks( f, &vf, NULL, 0, callbacks );

	if( err < 0 )
	{
		switch( err )
		{
			case OV_EREAD:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
						" media read error\n" );
				break;
			case OV_ENOTVORBIS:
/*				printf( "SampleBuffer::decodeSampleOGGVorbis():"
					" not an Ogg Vorbis file\n" );*/
				break;
			case OV_EVERSION:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
						" vorbis version mismatch\n" );
				break;
			case OV_EBADHEADER:
				printf( "SampleBuffer::decodeSampleOGGVorbis():"
					" invalid Vorbis bitstream header\n" );
				break;
			case OV_EFAULT:
				printf( "SampleBuffer::decodeSampleOgg(): "
					"internal logic fault\n" );
				break;
		}
		delete f;
		return 0;
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
	// if buffer isn't empty, convert it to float and write it down

	if ( frames > 0 && _buf != NULL )
	{
		convertIntToFloat ( _buf, frames, _channels);
	}

	return frames;
}
#endif




f_cnt_t SampleBuffer::decodeSampleDS( QString _f,
						int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _samplerate )
{
	DrumSynth ds;
	f_cnt_t frames = ds.GetDSFileSamples( _f, _buf, _channels, _samplerate );

	if ( frames > 0 && _buf != NULL )
	{
		convertIntToFloat ( _buf, frames, _channels);
	}

	return frames;

}




bool SampleBuffer::play( sampleFrame * _ab, handleState * _state,
					const fpp_t _frames,
					const float _freq,
					const LoopMode _loopmode )
{
	f_cnt_t startFrame = m_startFrame;
	f_cnt_t endFrame = m_endFrame;
	f_cnt_t loopStartFrame = m_loopStartFrame;
	f_cnt_t loopEndFrame = m_loopEndFrame;

	if( endFrame == 0 || _frames == 0 )
	{
		return false;
	}

	// variable for determining if we should currently be playing backwards in a ping-pong loop
	bool is_backwards = _state->isBackwards();

	const double freq_factor = (double) _freq / (double) m_frequency *
		m_sampleRate / Engine::mixer()->processingSampleRate();

	// calculate how many frames we have in requested pitch
	const f_cnt_t total_frames_for_current_pitch = static_cast<f_cnt_t>( (
						endFrame - startFrame ) /
								freq_factor );

	if( total_frames_for_current_pitch == 0 )
	{
		return false;
	}


	// this holds the index of the first frame to play
	f_cnt_t play_frame = qMax(_state->m_frameIndex, startFrame);

	if( _loopmode == LoopOff )
	{
		if( play_frame >= endFrame || ( endFrame - play_frame ) / freq_factor == 0 )
		{
			// the sample is done being played
			return false;
		}
	}
	else if( _loopmode == LoopOn )
	{
		play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame );
	}
	else
	{
		play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame );
	}

	f_cnt_t fragment_size = (f_cnt_t)( _frames * freq_factor ) + MARGIN[ _state->interpolationMode() ];

	sampleFrame * tmp = NULL;

	// check whether we have to change pitch...
	if( freq_factor != 1.0 || _state->m_varyingPitch )
	{
		SRC_DATA src_data;
		// Generate output
		src_data.data_in =
			getSampleFragment( play_frame, fragment_size, _loopmode, &tmp, &is_backwards,
			loopStartFrame, loopEndFrame, endFrame )->data ();
		src_data.data_out = _ab->data ();
		src_data.input_frames = fragment_size;
		src_data.output_frames = _frames;
		src_data.src_ratio = 1.0 / freq_factor;
		src_data.end_of_input = 0;
		int error = src_process( _state->m_resamplingData,
								&src_data );
		if( error )
		{
			printf( "SampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		if( src_data.output_frames_gen > _frames )
		{
			printf( "SampleBuffer: not enough frames: %ld / %d\n",
					src_data.output_frames_gen, _frames );
		}
		// Advance
		switch( _loopmode )
		{
			case LoopOff:
				play_frame += src_data.input_frames_used;
				break;
			case LoopOn:
				play_frame += src_data.input_frames_used;
				play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame );
				break;
			case LoopPingPong:
			{
				f_cnt_t left = src_data.input_frames_used;
				if( _state->isBackwards() )
				{
					play_frame -= src_data.input_frames_used;
					if( play_frame < loopStartFrame )
					{
						left -= ( loopStartFrame - play_frame );
						play_frame = loopStartFrame;
					}
					else left = 0;
				}
				play_frame += left;
				play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			}
		}
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		memcpy( _ab,
			getSampleFragment( play_frame, _frames, _loopmode, &tmp, &is_backwards,
						loopStartFrame, loopEndFrame, endFrame ),
						_frames * BYTES_PER_FRAME );
		// Advance
		switch( _loopmode )
		{
			case LoopOff:
				play_frame += _frames;
				break;
			case LoopOn:
				play_frame += _frames;
				play_frame = getLoopedIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			case LoopPingPong:
			{
				f_cnt_t left = _frames;
				if( _state->isBackwards() )
				{
					play_frame -= _frames;
					if( play_frame < loopStartFrame )
					{
						left -= ( loopStartFrame - play_frame );
						play_frame = loopStartFrame;
					}
					else left = 0;
				}
				play_frame += left;
				play_frame = getPingPongIndex( play_frame, loopStartFrame, loopEndFrame  );
				break;
			}
		}
	}

	if( tmp != NULL )
	{
		MM_FREE( tmp );
	}

	_state->setBackwards( is_backwards );
	_state->setFrameIndex( play_frame );

	for( fpp_t i = 0; i < _frames; ++i )
	{
		_ab[i][0] *= m_amplification;
		_ab[i][1] *= m_amplification;
	}

	return true;
}




sampleFrame * SampleBuffer::getSampleFragment( f_cnt_t _index,
		f_cnt_t _frames, LoopMode _loopmode, sampleFrame * * _tmp, bool * _backwards,
		f_cnt_t _loopstart, f_cnt_t _loopend, f_cnt_t _end ) const
{
	if( _loopmode == LoopOff )
	{
		if( _index + _frames <= _end )
		{
			return m_data + _index;
		}
	}
	else if( _loopmode == LoopOn )
	{
		if( _index + _frames <= _loopend )
		{
			return m_data + _index;
		}
	}
	else
	{
		if( ! *_backwards && _index + _frames < _loopend )
		{
			return m_data + _index;
		}
	}

	*_tmp = MM_ALLOC( sampleFrame, _frames );

	if( _loopmode == LoopOff )
	{
		f_cnt_t available = _end - _index;
		memcpy( *_tmp, m_data + _index, available * BYTES_PER_FRAME );
		memset( *_tmp + available, 0, ( _frames - available ) *
							BYTES_PER_FRAME );
	}
	else if( _loopmode == LoopOn )
	{
		f_cnt_t copied = qMin( _frames, _loopend - _index );
		memcpy( *_tmp, m_data + _index, copied * BYTES_PER_FRAME );
		f_cnt_t loop_frames = _loopend - _loopstart;
		while( copied < _frames )
		{
			f_cnt_t todo = qMin( _frames - copied, loop_frames );
			memcpy( *_tmp + copied, m_data + _loopstart, todo * BYTES_PER_FRAME );
			copied += todo;
		}
	}
	else
	{
		f_cnt_t pos = _index;
		bool backwards = pos < _loopstart
			? false
			: *_backwards;
		f_cnt_t copied = 0;


		if( backwards )
		{
			copied = qMin( _frames, pos - _loopstart );
			for( int i=0; i < copied; i++ )
			{
				(*_tmp)[i][0] = m_data[ pos - i ][0];
				(*_tmp)[i][1] = m_data[ pos - i ][1];
			}
			pos -= copied;
			if( pos == _loopstart ) backwards = false;
		}
		else
		{
			copied = qMin( _frames, _loopend - pos );
			memcpy( *_tmp, m_data + pos, copied * BYTES_PER_FRAME );
			pos += copied;
			if( pos == _loopend ) backwards = true;
		}

		while( copied < _frames )
		{
			if( backwards )
			{
				f_cnt_t todo = qMin( _frames - copied, pos - _loopstart );
				for ( int i=0; i < todo; i++ )
				{
					(*_tmp)[ copied + i ][0] = m_data[ pos - i ][0];
					(*_tmp)[ copied + i ][1] = m_data[ pos - i ][1];
				}
				pos -= todo;
				copied += todo;
				if( pos <= _loopstart ) backwards = false;
			}
			else
			{
				f_cnt_t todo = qMin( _frames - copied, _loopend - pos );
				memcpy( *_tmp + copied, m_data + pos, todo * BYTES_PER_FRAME );
				pos += todo;
				copied += todo;
				if( pos >= _loopend ) backwards = true;
			}
		}
		*_backwards = backwards;
	}

	return *_tmp;
}




f_cnt_t SampleBuffer::getLoopedIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf ) const
{
	if( _index < _endf )
	{
		return _index;
	}
	return _startf + ( _index - _startf )
				% ( _endf - _startf );
}


f_cnt_t SampleBuffer::getPingPongIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf ) const
{
	if( _index < _endf )
	{
		return _index;
	}
	const f_cnt_t looplen = _endf - _startf;
	const f_cnt_t looppos = ( _index - _endf ) % ( looplen*2 );

	return ( looppos < looplen )
		? _endf - looppos
		: _startf + ( looppos - looplen );
}


void SampleBuffer::visualize( QPainter & _p, const QRect & _dr,
							const QRect & _clip, f_cnt_t _from_frame, f_cnt_t _to_frame )
{
	if( m_frames == 0 ) return;

	const bool focus_on_range = _to_frame <= m_frames
					&& 0 <= _from_frame && _from_frame < _to_frame;
	//_p.setClipRect( _clip );
	const int w = _dr.width();
	const int h = _dr.height();

	const int yb = h / 2 + _dr.y();
	const float y_space = h*0.5f;
	const int nb_frames = focus_on_range ? _to_frame - _from_frame : m_frames;

	const int fpp = qBound<int>( 1, nb_frames / w, 20 );
	QPointF * l = new QPointF[nb_frames / fpp + 1];
	QPointF * r = new QPointF[nb_frames / fpp + 1];
	int n = 0;
	const int xb = _dr.x();
	const int first = focus_on_range ? _from_frame : 0;
	const int last = focus_on_range ? _to_frame : m_frames;
	for( int frame = first; frame < last; frame += fpp )
	{
		l[n] = QPointF( xb + ( (frame - first) * double( w ) / nb_frames ),
			( yb - ( m_data[frame][0] * y_space * m_amplification ) ) );
		r[n] = QPointF( xb + ( (frame - first) * double( w ) / nb_frames ),
			( yb - ( m_data[frame][1] * y_space * m_amplification ) ) );
		++n;
	}
	_p.setRenderHint( QPainter::Antialiasing );
	_p.drawPolyline( l, nb_frames / fpp );
	_p.drawPolyline( r, nb_frames / fpp );
	delete[] l;
	delete[] r;
}




QString SampleBuffer::openAudioFile() const
{
	FileDialog ofd( NULL, tr( "Open audio file" ) );

	QString dir;
	if( !m_audioFile.isEmpty() )
	{
		QString f = m_audioFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = ConfigManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == false )
			{
				f = ConfigManager::inst()->factorySamplesDir() +
								m_audioFile;
			}
		}
		dir = QFileInfo( f ).absolutePath();
	}
	else
	{
		dir = ConfigManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( FileDialog::ExistingFiles );

	// set filters
	QStringList types;
	types << tr( "All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
					"*.aif *.aiff *.au *.raw)" )
		<< tr( "Wave-Files (*.wav)" )
		<< tr( "OGG-Files (*.ogg)" )
		<< tr( "DrumSynth-Files (*.ds)" )
		<< tr( "FLAC-Files (*.flac)" )
		<< tr( "SPEEX-Files (*.spx)" )
		//<< tr( "MP3-Files (*.mp3)" )
		//<< tr( "MIDI-Files (*.mid)" )
		<< tr( "VOC-Files (*.voc)" )
		<< tr( "AIFF-Files (*.aif *.aiff)" )
		<< tr( "AU-Files (*.au)" )
		<< tr( "RAW-Files (*.raw)" )
		//<< tr( "MOD-Files (*.mod)" )
		;
	ofd.setNameFilters( types );
	if( !m_audioFile.isEmpty() )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_audioFile ).fileName() );
	}

	if( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return QString();
		}
		return PathUtil::toShortestRelative( ofd.selectedFiles()[0] );
	}

	return QString();
}




QString SampleBuffer::openAndSetAudioFile()
{
	QString fileName = this->openAudioFile();

	if(!fileName.isEmpty())
	{
		this->setAudioFile( fileName );
	}

	return fileName;
}


QString SampleBuffer::openAndSetWaveformFile()
{
	if( m_audioFile.isEmpty() )
	{
		m_audioFile = ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac";
	}

	QString fileName = this->openAudioFile();

	if(!fileName.isEmpty())
	{
		this->setAudioFile( fileName );
	}
	else
	{
		m_audioFile = "";
	}

	return fileName;
}



#undef LMMS_HAVE_FLAC_STREAM_ENCODER_H	/* not yet... */
#undef LMMS_HAVE_FLAC_STREAM_DECODER_H

#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
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
		return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	}*/
	return ( static_cast<QBuffer *>( _client_data )->write(
				(const char *) _buffer, _bytes ) ==
								(int) _bytes ) ?
				FLAC__STREAM_ENCODER_WRITE_STATUS_OK :
				FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
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



QString & SampleBuffer::toBase64( QString & _dst ) const
{
#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
	const f_cnt_t FRAMES_PER_BUF = 1152;

	FLAC__StreamEncoder * flac_enc = FLAC__stream_encoder_new();
	FLAC__stream_encoder_set_channels( flac_enc, DEFAULT_CHANNELS );
	FLAC__stream_encoder_set_blocksize( flac_enc, FRAMES_PER_BUF );
/*	FLAC__stream_encoder_set_do_exhaustive_model_search( flac_enc, true );
	FLAC__stream_encoder_set_do_mid_side_stereo( flac_enc, true );*/
	FLAC__stream_encoder_set_sample_rate( flac_enc,
					Engine::mixer()->sampleRate() );
	QBuffer ba_writer;
	ba_writer.open( QBuffer::WriteOnly );

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
		f_cnt_t remaining = qMin<f_cnt_t>( FRAMES_PER_BUF,
							m_frames - frame_cnt );
		FLAC__int32 buf[FRAMES_PER_BUF * DEFAULT_CHANNELS];
		for( f_cnt_t f = 0; f < remaining; ++f )
		{
			for( ch_cnt_t ch = 0; ch < DEFAULT_CHANNELS; ++ch )
			{
				buf[f*DEFAULT_CHANNELS+ch] = (FLAC__int32)(
					Mixer::clip( m_data[f+frame_cnt][ch] ) *
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


#else	/* LMMS_HAVE_FLAC_STREAM_ENCODER_H */

	base64::encode( (const char *) m_data,
					m_frames * sizeof( sampleFrame ), _dst );

#endif	/* LMMS_HAVE_FLAC_STREAM_ENCODER_H */

	return _dst;
}




SampleBuffer * SampleBuffer::resample( const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
{
	sampleFrame * data = m_data;
	const f_cnt_t frames = m_frames;
	const f_cnt_t dst_frames = static_cast<f_cnt_t>( frames /
					(float) _src_sr * (float) _dst_sr );
	SampleBuffer * dst_sb = new SampleBuffer( dst_frames );
	sampleFrame * dst_buf = dst_sb->m_origData;

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE * state;
	if( ( state = src_new( SRC_SINC_MEDIUM_QUALITY,
					DEFAULT_CHANNELS, &error ) ) != NULL )
	{
		SRC_DATA src_data;
		src_data.end_of_input = 1;
		src_data.data_in = data->data ();
		src_data.data_out = dst_buf->data ();
		src_data.input_frames = frames;
		src_data.output_frames = dst_frames;
		src_data.src_ratio = (double) _dst_sr / _src_sr;
		if( ( error = src_process( state, &src_data ) ) )
		{
			printf( "SampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		src_delete( state );
	}
	else
	{
		printf( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
	dst_sb->update();
	return dst_sb;
}




void SampleBuffer::setAudioFile( const QString & _audio_file )
{
	m_audioFile = PathUtil::toShortestRelative( _audio_file );
	update();
}



#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H

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
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

	}
	*_bytes = 0;
	return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
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
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if( _frame->header.bits_per_sample != 16 )
	{
		printf( "bits_per_sample != 16 in "
					"flacStreamDecoderWriteCallback()\n" );
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
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
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
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


void SampleBuffer::loadFromBase64( const QString & _data )
{
	char * dst = NULL;
	int dsize = 0;
	base64::decode( _data, &dst, &dsize );

#ifdef LMMS_HAVE_FLAC_STREAM_DECODER_H

	QByteArray orig_data = QByteArray::fromRawData( dst, dsize );
	QBuffer ba_reader( &orig_data );
	ba_reader.open( QBuffer::ReadOnly );

	QBuffer ba_writer;
	ba_writer.open( QBuffer::WriteOnly );

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
	MM_FREE( m_origData );
	m_origData = MM_ALLOC( sampleFrame, m_origFrames );
	memcpy( m_origData, orig_data.data(), orig_data.size() );

#else /* LMMS_HAVE_FLAC_STREAM_DECODER_H */

	m_origFrames = dsize / sizeof( sampleFrame );
	MM_FREE( m_origData );
	m_origData = MM_ALLOC( sampleFrame, m_origFrames );
	memcpy( m_origData, dst, dsize );

#endif

	delete[] dst;

	m_audioFile = QString();
	update();
}




void SampleBuffer::setStartFrame( const f_cnt_t _s )
{
	m_startFrame = _s;
}




void SampleBuffer::setEndFrame( const f_cnt_t _e )
{
	m_endFrame = _e;
}




void SampleBuffer::setAmplification( float _a )
{
	m_amplification = _a;
	emit sampleUpdated();
}




void SampleBuffer::setReversed( bool _on )
{
	m_reversed = _on;
	update( true );
}












SampleBuffer::handleState::handleState( bool _varying_pitch, int interpolation_mode ) :
	m_frameIndex( 0 ),
	m_varyingPitch( _varying_pitch ),
	m_isBackwards( false )
{
	int error;
	m_interpolationMode = interpolation_mode;

	if( ( m_resamplingData = src_new( interpolation_mode, DEFAULT_CHANNELS, &error ) ) == NULL )
	{
		qDebug( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
}




SampleBuffer::handleState::~handleState()
{
	src_delete( m_resamplingData );
}
