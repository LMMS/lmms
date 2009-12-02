/*
 * sample_buffer.cpp - container-class sampleBuffer
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "sample_buffer.h"
#include "Mixer.h"


#include <QtCore/QBuffer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>


#include <cstring>

#include <sndfile.h>

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
#include "config_mgr.h"
#include "debug.h"
#include "drumsynth.h"
#include "endian_handling.h"
#include "engine.h"
#include "interpolation.h"
#include "templates.h"

#include "lame_library.h"


const char * sampleBuffer::supportedExts[] = {
	"wav",
	"ogg",
	"ds",
	"spx",
	"au",
	"voc",
	"aif",
	"aiff",
	"flac",
	"raw",
	"mp3",
	NULL
};

sampleBuffer::sampleBuffer( const QString & _audio_file,
							bool _is_base64_data ) :
	m_audioFile( ( _is_base64_data == true ) ? "" : _audio_file ),
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
	m_sampleRate( engine::getMixer()->baseSampleRate() )
{
	if( _is_base64_data == true )
	{
		loadFromBase64( _audio_file );
	}
	update();
}




sampleBuffer::sampleBuffer( const sampleFrame * _data, const f_cnt_t _frames ) :
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
	m_sampleRate( engine::getMixer()->baseSampleRate() )
{
	if( _frames > 0 )
	{
		m_origData = new sampleFrame[_frames];
		memcpy( m_origData, _data, _frames * BYTES_PER_FRAME );
		m_origFrames = _frames;
	}
	update();
}




sampleBuffer::sampleBuffer( const f_cnt_t _frames ) :
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
	m_sampleRate( engine::getMixer()->baseSampleRate() )
{
	if( _frames > 0 )
	{
		m_origData = new sampleFrame[_frames];
		memset( m_origData, 0, _frames * BYTES_PER_FRAME );
		m_origFrames = _frames;
	}
	update();
}




sampleBuffer::~sampleBuffer()
{
	delete[] m_origData;
	delete[] m_data;
}






void sampleBuffer::update( bool _keep_settings )
{
	const bool lock = ( m_data != NULL );
	if( lock )
	{
		engine::getMixer()->lock();
		delete[] m_data;
	}

	if( m_audioFile.isEmpty() && m_origData != NULL && m_origFrames > 0 )
	{
		// TODO: reverse- and amplification-property is not covered
		// by following code...
		m_data = new sampleFrame[m_origFrames];
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
		QString file = tryToMakeAbsolute( m_audioFile );
		char * f = qstrdup( file.toUtf8().constData() );
		int_sample_t * buf = NULL;
		ch_cnt_t channels = DEFAULT_CHANNELS;
		sample_rate_t samplerate = engine::getMixer()->baseSampleRate();
		m_frames = 0;

		if( QFileInfo( file ).size() > 100*1024*1024 )
		{
			qWarning( "refusing to load sample files bigger "
								"than 100 MB" );
		}
		else
		{
			// PCM wave
			if( m_frames == 0 )
				m_frames = decodeSampleSF( f, buf, channels, samplerate );
#ifdef LMMS_HAVE_OGGVORBIS
			if( m_frames == 0 )
				m_frames = decodeSampleOGGVorbis( f, buf, channels, samplerate );
#endif
			if( m_frames == 0 )
				m_frames = decodeSampleDS( f, buf, channels, samplerate );

			// MP3
			if( m_frames == 0 )
				m_frames = decodeSampleMp3( file, buf, channels, samplerate );

			delete[] f;
		}

		if( m_frames > 0 && buf != NULL )
		{
			// following code transforms int-samples into
			// float-samples and does amplifying & reversing
			const float fac = m_amplification /
						OUTPUT_SAMPLE_MULTIPLIER;
			m_data = new sampleFrame[m_frames];
			const int ch = ( channels > 1 ) ? 1 : 0;

			// if reversing is on, we also reverse when
			// scaling
			if( m_reversed )
			{
				int idx = ( m_frames - 1 ) * channels;
				for( f_cnt_t frame = 0; frame < m_frames;
								++frame )
				{
					m_data[frame][0] = buf[idx+0] * fac;
					m_data[frame][1] = buf[idx+ch] * fac;
					idx -= channels;
				}
			}
			else
			{
				int idx = 0;
				for( f_cnt_t frame = 0; frame < m_frames;
								++frame )
				{
					m_data[frame][0] = buf[idx+0] * fac;
					m_data[frame][1] = buf[idx+ch] * fac;
					idx += channels;
				}
			}

			delete[] buf;

			normalizeSampleRate( samplerate, _keep_settings );
		}
		else
		{
			// sample couldn't be decoded, create buffer containing
			// one sample-frame
			m_data = new sampleFrame[1];
			memset( m_data, 0, sizeof( *m_data ) );
			m_frames = 1;
			m_loopStartFrame = m_startFrame = 0;
			m_loopEndFrame = m_endFrame = 1;

			if( engine::hasGUI() )
			{
				if( QFileInfo( file ).exists() )
				{
					// TODO: this list probably shouldn't be hardcoded
					QString decoders = niceListOfExts();
					QMessageBox::information( NULL,
						QObject::tr( "Unrecognized audio format" ),
						QObject::tr( "None of the available audio decoders "
							"recognized the format you are trying to load. Try "
							"converting the file to a format LMMS understands.\n\n"
							"Your file: %1\n"
							"Available decoders: %2\n").arg( file, decoders ),
						QMessageBox::Ok | QMessageBox::Default );
				}
				else
				{
					QMessageBox::information( NULL,
						QObject::tr( "File not found" ),
						QObject::tr( "The file %1 could not be found! "
							"You should set a valid file in order to allow "
							"proper playback." ).arg( file ),
						QMessageBox::Ok | QMessageBox::Default );
				}
			}
			else
			{
				// TODO: qWarning() & Co
			}
		}
	}
	else
	{
		// neither an audio-file nor a buffer to copy from, so create
		// buffer containing one sample-frame
		m_data = new sampleFrame[1];
		memset( m_data, 0, sizeof( *m_data ) );
		m_frames = 1;
		m_loopStartFrame = m_startFrame = 0;
		m_loopEndFrame = m_endFrame = 1;
	}

	if( lock )
	{
		engine::getMixer()->unlock();
	}

	emit sampleUpdated();
}




QString sampleBuffer::niceListOfExts() const
{
	QString ret;
	for( const char * * i = supportedExts; *i; ++i )
	{
		ret.append( *i );
		if( *(i+1) )
			ret.append( ", " );
	}

	return ret;
}




void sampleBuffer::normalizeSampleRate( const sample_rate_t _src_sr,
	bool _keep_settings )
{
	// do samplerate-conversion to our default-samplerate
	if( _src_sr != engine::getMixer()->baseSampleRate() )
	{
		sampleBuffer * resampled = resample( this, _src_sr,
					engine::getMixer()->baseSampleRate() );
		delete[] m_data;
		m_frames = resampled->frames();
		m_data = new sampleFrame[m_frames];
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
}




f_cnt_t sampleBuffer::decodeSampleSF( const char * _f,
					int_sample_t * & _buf,
					ch_cnt_t & _channels,
					sample_rate_t & _samplerate )
{
	SNDFILE * snd_file;
	SF_INFO sf_info;
	f_cnt_t frames = 0;
	if( ( snd_file = sf_open( _f, SFM_READ, &sf_info ) ) != NULL )
	{
		frames = sf_info.frames;
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

	return frames;
}
#endif


int lame_decode_fromfile(QFile &in, short pcm_l[], short pcm_r[],
	mp3data_struct * mp3data, LameLibrary &lame)
{
	int	 ret = 0;
	size_t  len = 0;
	unsigned char buf[1024];

	/* first see if we still have data buffered in the decoder: */
	ret = lame.lame_decode1_headers(buf, len, pcm_l, pcm_r, mp3data);
	if (ret != 0)
		return ret;


	/* read until we get a valid output frame */
	while (1) {
		len = in.read((char *)buf, 1024);
		if (len == 0) {
			/* we are done reading the file, but check for buffered data */
			ret = lame.lame_decode1_headers(buf, len, pcm_l, pcm_r, mp3data);
			if (ret <= 0) {
				//lame.lame_decode_exit(); /* release mp3decoder memory */
				//return -1; /* done with file */

				return 0;
			}
			break;
		}

		ret = lame.lame_decode1_headers(buf, len, pcm_l, pcm_r, mp3data);
		if (ret == -1) {
			lame.lame_decode_exit(); /* release mp3decoder memory */
			return -1;
		}
		if (ret > 0)
			break;
	}
	return ret;
}

f_cnt_t sampleBuffer::decodeSampleMp3( QString & file, int_sample_t * & _buf,
	ch_cnt_t & _channels, sample_rate_t & _samplerate )
{
	// create instance of LameLibrary to decode
	LameLibrary lame;

	// open the file
	QFile in(file);

	if( ! in.open(QIODevice::ReadOnly) ){
		printf("sampleBuffer::decodeSampleMp3: error opening %s for reading\n",
			file.toStdString().c_str());
		return 0;
	}

	// initialize lame decoder
	lame.lame_decode_init();

	short int pcm_l[1152];
	short int pcm_r[1152];
	mp3data_struct mp3data;

	// TODO: calc _buf size

	int bufPos = 0;
	int bufSize = 0;
	bool initBuf = false;

	while(1)
	{
		int ret = lame_decode_fromfile(in, pcm_l, pcm_r, &mp3data, lame);

		if( ret == -1 ){
			delete[] _buf;
			printf("error decoding mp3\n");
			return 0;
		} else if( ret == 0 ) {
			break;
		}

		if( ! initBuf )
		{
			if( mp3data.header_parsed == 0 )
			{
				qWarning("MP3 decoder: failed to parse header\n");
				return 0;
			}
			else
			{
				// calculate buffer size
				int safetyPad = 7200;
				int sampleSafetyPad = 1096;
				bufSize = (int)( (float)in.size() / 
					((float)mp3data.bitrate * 1000.0 / 8.0) *
					(float)mp3data.samplerate + sampleSafetyPad);
				bufSize = (bufSize + safetyPad) * _channels;

				// process header
				_samplerate = mp3data.samplerate;
				_channels = mp3data.stereo;
				_buf = new int_sample_t[bufSize];
				initBuf = true;
			}
		}

		// convert the decoded PCM into sample
		for(int i = 0; i<ret && bufPos<bufSize; ++i)
		{
			_buf[bufPos++] = pcm_l[i];
			if( _channels == 2 )
				_buf[bufPos++] = pcm_r[i];
		}

		if( bufPos >= bufSize )
		{
			qWarning("MP3 file exceeded the calculated buffer size");
		}

	}

	lame.lame_decode_exit();

	return bufPos / _channels;
}


f_cnt_t sampleBuffer::decodeSampleDS( const char * _f,
						int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _samplerate )
{
	DrumSynth ds;
	return ds.GetDSFileSamples( _f, _buf, _channels );
}




bool sampleBuffer::play( sampleFrame * _ab, handleState * _state,
					const fpp_t _frames,
					const float _freq,
					const bool _looped )
{
	QMutexLocker ml( &m_varLock );

	engine::getMixer()->clearAudioBuffer( _ab, _frames );

	if( m_endFrame == 0 || _frames == 0 )
	{
		return false;
	}

	const double freq_factor = (double) _freq / (double) m_frequency *
		m_sampleRate / engine::getMixer()->processingSampleRate();

	// calculate how many frames we have in requested pitch
	const f_cnt_t total_frames_for_current_pitch = static_cast<f_cnt_t>( (
						m_endFrame - m_startFrame ) /
								freq_factor );
	if( total_frames_for_current_pitch == 0 )
	{
		return false;
	}

	// this holds the number of the first frame to play
	f_cnt_t play_frame = _state->m_frameIndex;
	if( play_frame < m_startFrame )
	{
		play_frame = m_startFrame;
	}

	// this holds the number of remaining frames in current loop
	f_cnt_t frames_for_loop;
	if( _looped )
	{
		play_frame = getLoopedIndex( play_frame );
		frames_for_loop = static_cast<f_cnt_t>(
					( m_loopEndFrame - play_frame ) /
								freq_factor );
	}
	else
	{
		if( play_frame >= m_endFrame )
		{
			return false;
		}
		frames_for_loop = static_cast<f_cnt_t>(
					( m_endFrame - play_frame ) /
								freq_factor );
		if( frames_for_loop == 0 )
		{
			return false;
		}
	}

	sampleFrame * tmp = NULL;

	// check whether we have to change pitch...
	if( freq_factor != 1.0 || _state->m_varyingPitch )
	{
		SRC_DATA src_data;
		// Generate output
		const f_cnt_t margin = 64;
		f_cnt_t fragment_size = (f_cnt_t)( _frames * freq_factor )
								+ margin;
		src_data.data_in = getSampleFragment( play_frame,
					fragment_size, _looped, &tmp )[0];
		src_data.data_out = _ab[0];
		src_data.input_frames = fragment_size;
		src_data.output_frames = _frames;
		src_data.src_ratio = 1.0 / freq_factor;
		src_data.end_of_input = 0;
		int error = src_process( _state->m_resamplingData,
								&src_data );
		if( error )
		{
			printf( "sampleBuffer: error while resampling: %s\n",
							src_strerror( error ) );
		}
		if( src_data.output_frames_gen > _frames )
		{
			printf( "sampleBuffer: not enough frames: %ld / %d\n",
					src_data.output_frames_gen, _frames );
		}
		// Advance
		play_frame += src_data.input_frames_used;
		if( _looped )
		{
			play_frame = getLoopedIndex( play_frame );
		}
	}
	else
	{
		// we don't have to pitch, so we just copy the sample-data
		// as is into pitched-copy-buffer

		// Generate output
		memcpy( _ab,
			getSampleFragment( play_frame, _frames, _looped, &tmp ),
						_frames * BYTES_PER_FRAME );
		// Advance
		play_frame += _frames;
		if( _looped )
		{
			play_frame = getLoopedIndex( play_frame );
		}
	}

	delete[] tmp;

	_state->m_frameIndex = play_frame;

	return true;

}




sampleFrame * sampleBuffer::getSampleFragment( f_cnt_t _start,
		f_cnt_t _frames, bool _looped, sampleFrame * * _tmp ) const
{
	if( _looped )
	{
		if( _start + _frames <= m_loopEndFrame )
		{
			return m_data + _start;
		}
	}
	else
	{
		if( _start + _frames <= m_endFrame )
		{
			return m_data + _start;
		}
	}

	*_tmp = new sampleFrame[_frames];

	if( _looped )
	{
		f_cnt_t copied = m_loopEndFrame - _start;
		memcpy( *_tmp, m_data + _start, copied * BYTES_PER_FRAME );
		f_cnt_t loop_frames = m_loopEndFrame - m_loopStartFrame;
		while( _frames - copied > 0 )
		{
			f_cnt_t todo = qMin( _frames - copied, loop_frames );
			memcpy( *_tmp + copied, m_data + m_loopStartFrame,
						todo * BYTES_PER_FRAME );
			copied += todo;
		}
	}
	else
	{
		f_cnt_t available = m_endFrame - _start;
		memcpy( *_tmp, m_data + _start, available * BYTES_PER_FRAME );
		memset( *_tmp + available, 0, ( _frames - available ) *
							BYTES_PER_FRAME );
	}

	return *_tmp;
}




f_cnt_t sampleBuffer::getLoopedIndex( f_cnt_t _index ) const
{
	if( _index < m_loopEndFrame )
	{
		return _index;
	}
	return m_loopStartFrame + ( _index - m_loopStartFrame )
				% ( m_loopEndFrame - m_loopStartFrame );
}




void sampleBuffer::visualize( QPainter & _p, const QRect & _dr,
							const QRect & _clip )
{
//	_p.setClipRect( _clip );
//	_p.setPen( QColor( 0x22, 0xFF, 0x44 ) );
	//_p.setPen( QColor( 64, 224, 160 ) );
	const int w = _dr.width();
	const int h = _dr.height();

	const int yb = h / 2 + _dr.y();
	const float y_space = h*0.25f;

	if( m_frames < 60000 )
	{
		_p.setRenderHint( QPainter::Antialiasing );
		QColor c = _p.pen().color();
		_p.setPen( QPen( c, 0.7 ) );
	}
	const int fpp = tLimit<int>( m_frames / w, 1, 20 );
	QPoint * l = new QPoint[m_frames / fpp + 1];
	int n = 0;
	const int xb = _dr.x();
	for( int frame = 0; frame < m_frames; frame += fpp )
	{
		l[n] = QPoint( xb + ( frame * w / m_frames ),
			(int)( yb - ( ( m_data[frame][0]+m_data[frame][1] ) *
								y_space ) ) );
		++n;
	}
	_p.drawPolyline( l, m_frames / fpp );
	delete[] l;
}




QString sampleBuffer::openAudioFile() const
{
	QFileDialog ofd( NULL, tr( "Open audio file" ) );

	QString dir;
	if( !m_audioFile.isEmpty() )
	{
		QString f = m_audioFile;
		if( QFileInfo( f ).isRelative() )
		{
			f = configManager::inst()->userSamplesDir() + f;
			if( QFileInfo( f ).exists() == false )
			{
				f = configManager::inst()->factorySamplesDir() +
								m_audioFile;
			}
		}
		dir = QFileInfo( f ).absolutePath();
	}
	else
	{
		dir = configManager::inst()->userSamplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
	QStringList types;
	types << tr( "All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
					"*.aif *.aiff *.au *.raw *.mp3)" )
		<< tr( "Wave-Files (*.wav)" )
		<< tr( "OGG-Files (*.ogg)" )
		<< tr( "DrumSynth-Files (*.ds)" )
		<< tr( "FLAC-Files (*.flac)" )
		<< tr( "SPEEX-Files (*.spx)" )
		<< tr( "MP3-Files (*.mp3)" )
		//<< tr( "MIDI-Files (*.mid)" )
		<< tr( "VOC-Files (*.voc)" )
		<< tr( "AIFF-Files (*.aif *.aiff)" )
		<< tr( "AU-Files (*.au)" )
		<< tr( "RAW-Files (*.raw)" )
		//<< tr( "MOD-Files (*.mod)" )
		;
	ofd.setFilters( types );
	if( !m_audioFile.isEmpty() )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_audioFile ).fileName() );
	}

	if( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return QString::null;
		}
		return tryToMakeRelative( ofd.selectedFiles()[0] );
	}

	return QString::null;
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



QString & sampleBuffer::toBase64( QString & _dst ) const
{
#ifdef LMMS_HAVE_FLAC_STREAM_ENCODER_H
	const f_cnt_t FRAMES_PER_BUF = 1152;

	FLAC__StreamEncoder * flac_enc = FLAC__stream_encoder_new();
	FLAC__stream_encoder_set_channels( flac_enc, DEFAULT_CHANNELS );
	FLAC__stream_encoder_set_blocksize( flac_enc, FRAMES_PER_BUF );
/*	FLAC__stream_encoder_set_do_exhaustive_model_search( flac_enc, true );
	FLAC__stream_encoder_set_do_mid_side_stereo( flac_enc, true );*/
	FLAC__stream_encoder_set_sample_rate( flac_enc,
					engine::getMixer()->sampleRate() );
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


#else	/* LMMS_HAVE_FLAC_STREAM_ENCODER_H */

	base64::encode( (const char *) m_data,
					m_frames * sizeof( sampleFrame ), _dst );

#endif	/* LMMS_HAVE_FLAC_STREAM_ENCODER_H */

	return _dst;
}




sampleBuffer * sampleBuffer::resample( sampleFrame * _data,
						const f_cnt_t _frames,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
{
	const f_cnt_t dst_frames = static_cast<f_cnt_t>( _frames /
					(float) _src_sr * (float) _dst_sr );
	sampleBuffer * dst_sb = new sampleBuffer( dst_frames );
	sampleFrame * dst_buf = dst_sb->m_origData;

	// yeah, libsamplerate, let's rock with sinc-interpolation!
	int error;
	SRC_STATE * state;
	if( ( state = src_new( SRC_SINC_MEDIUM_QUALITY,
					DEFAULT_CHANNELS, &error ) ) != NULL )
	{
		SRC_DATA src_data;
		src_data.end_of_input = 1;
		src_data.data_in = _data[0];
		src_data.data_out = dst_buf[0];
		src_data.input_frames = _frames;
		src_data.output_frames = dst_frames;
		src_data.src_ratio = (double) _dst_sr / _src_sr;
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
	dst_sb->update();
	return dst_sb;
}




void sampleBuffer::setAudioFile( const QString & _audio_file )
{
	m_audioFile = tryToMakeRelative( _audio_file );
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


void sampleBuffer::loadFromBase64( const QString & _data )
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
	delete[] m_origData;
	m_origData = new sampleFrame[m_origFrames];
	memcpy( m_origData, orig_data.data(), orig_data.size() );

#else /* LMMS_HAVE_FLAC_STREAM_DECODER_H */

	m_origFrames = dsize / sizeof( sampleFrame );
	delete[] m_origData;
	m_origData = new sampleFrame[m_origFrames];
	memcpy( m_origData, dst, dsize );

#endif

	delete[] dst;

	m_audioFile = QString();
	update();
}




void sampleBuffer::setStartFrame( const f_cnt_t _s )
{
	m_varLock.lock();
	m_loopStartFrame = m_startFrame = _s;
	m_varLock.unlock();
}




void sampleBuffer::setEndFrame( const f_cnt_t _e )
{
	m_varLock.lock();
	m_loopEndFrame = m_endFrame = _e;
	m_varLock.unlock();
}




void sampleBuffer::setAmplification( float _a )
{
	m_amplification = _a;
	update( true );
}




void sampleBuffer::setReversed( bool _on )
{
	m_reversed = _on;
	update( true );
}




QString sampleBuffer::tryToMakeRelative( const QString & _file )
{
	if( QFileInfo( _file ).isRelative() == false )
	{
		QString f = QString( _file ).replace( QDir::separator(), '/' );
		QString fsd = configManager::inst()->factorySamplesDir();
		QString usd = configManager::inst()->userSamplesDir();
		fsd.replace( QDir::separator(), '/' );
		usd.replace( QDir::separator(), '/' );
		if( f.startsWith( fsd ) )
		{
			return QString( f ).mid( fsd.length() );
		}
		else if( f.startsWith( usd ) )
		{
			return QString( f ).mid( usd.length() );
		}
	}
	return _file;
}




QString sampleBuffer::tryToMakeAbsolute( const QString & _file )
{
	if( QFileInfo( _file ).isAbsolute() )
	{
		return _file;
	}

	QString f = configManager::inst()->userSamplesDir() + _file;
	if( QFileInfo( f ).exists() )
	{
		return f;
	}

	return configManager::inst()->factorySamplesDir() + _file;
}








sampleBuffer::handleState::handleState( bool _varying_pitch ) :
	m_frameIndex( 0 ),
	m_varyingPitch( _varying_pitch )
{
	int error;
	if( ( m_resamplingData = src_new(/*
		( engine::getMixer()->highQuality() == true ) ?
					SRC_SINC_FASTEST :*/
					SRC_LINEAR,
					DEFAULT_CHANNELS, &error ) ) == NULL )
	{
		printf( "Error: src_new() failed in sample_buffer.cpp!\n" );
	}
}




sampleBuffer::handleState::~handleState()
{
	src_delete( m_resamplingData );
}




#include "moc_sample_buffer.cxx"


/* vim: set tw=0 noexpandtab: */
