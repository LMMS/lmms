/*
 * sample_buffer.cpp - container-class sampleBuffer
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QPainter>
#include <QMutex>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>

#else

#include <qpainter.h>
#include <qmutex.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qfileinfo.h>
#include <qfile.h>

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


#include "sample_buffer.h"
#include "interpolation.h"
#include "paths.h"
#include "templates.h"
#include "config_mgr.h"
#include "endian_handling.h"
#include "debug.h"



sampleBuffer::sampleBuffer( const QString & _audio_file ) :
	QObject(),
	m_audioFile( _audio_file ),
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
	update();
}




sampleBuffer::sampleBuffer( const sampleFrame * _data, Uint32 _frames ) :
	QObject(),
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
	memcpy( m_origData, _data, _frames*BYTES_PER_FRAME );
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
	m_dataMutex.unlock();

#ifdef HAVE_SAMPLERATE_H
	quitResampling();
#endif
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
			file = configManager::inst()->samplesDir() + file;
		}
		const char * f =
#ifdef QT4
				file.toAscii().constData();
#else
				file.ascii();
#endif
		Sint16 * buf = NULL;
		Uint8 channels;

#ifdef SDL_SDL_SOUND_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleSDL( f, buf, channels );
		}
#endif
#ifdef HAVE_SNDFILE_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleSF( f, buf, channels );
		}
#endif
#ifdef HAVE_VORBIS_VORBISFILE_H
		if( m_frames == 0 )
		{
			m_frames = decodeSampleOGG( f, buf, channels );
		}
#endif
		if( m_frames > 0 && buf != NULL )
		{
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

			// following code transforms int-samples into
			// float-samples and does amplifying & reversing
			float fac = m_amplification / 32767.0f;
			m_data = new sampleFrame[m_frames];

			// if reversing is on, we also reverse when
			// scaling
			if( m_reversed )
			{
				for( Uint32 frame = 0; frame < m_frames;
								++frame )
				{
					for( Uint8 chnl = 0;
							chnl < DEFAULT_CHANNELS;
									++chnl )
					{
const Uint32 idx = ( m_frames - frame ) * channels + ( chnl % channels );
m_data[frame][chnl] = buf[idx] * fac;
					}
				}
			}
			else
			{
				for( Uint32 frame = 0; frame < m_frames;
								++frame )
				{
					for( Uint8 chnl = 0;
							chnl < DEFAULT_CHANNELS;
									++chnl )
					{
		const Uint32 idx = frame * channels + ( chnl % channels );
		m_data[frame][chnl] = buf[idx] * fac;
					}
				}
			}

			delete[] buf;

		}
		else
		{
			m_data = new sampleFrame[1];
			memset( m_data, 0, sizeof( *m_data ) );
			m_frames = 1;
			m_startFrame = 0;
			m_endFrame = 1;
		}
	}
	else
	{
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
Uint32 sampleBuffer::decodeSampleSDL( const char * _f, Sint16 * & _buf,
							Uint8 & _channels )
{
	Sound_AudioInfo STD_AUDIO_INFO =
	{
		AUDIO_S16SYS,
		DEFAULT_CHANNELS,
		SAMPLE_RATES[DEFAULT_QUALITY_LEVEL]
	} ;
	Uint32 frames = 0;

	Sound_Sample * snd_sample = Sound_NewSampleFromFile( _f,
						&STD_AUDIO_INFO, 16384 );
	// file not found?
	if( snd_sample != NULL )
	{
		// let SDL_sound decode our file to requested format
		( void )Sound_DecodeAll( snd_sample );
		_channels = STD_AUDIO_INFO.channels;
		frames = snd_sample->buffer_size / ( BYTES_PER_OUTPUT_SAMPLE *
								_channels );
		_buf = new Sint16[frames * _channels];
		memcpy( _buf, snd_sample->buffer, snd_sample->buffer_size );

		Sound_FreeSample( snd_sample );
	}
	return( frames );
}
#endif




#ifdef HAVE_SNDFILE_H
Uint32 sampleBuffer::decodeSampleSF( const char * _f, Sint16 * & _buf,
							Uint8 & _channels )
{
	SNDFILE * snd_file;
	SF_INFO sf_info;
	Uint32 frames = 0;
#ifdef OLD_SNDFILE
	if( ( snd_file = sf_open_read( _f, &sf_info ) ) != NULL )
	{
		frames = sf_info.samples;
#else
	if( ( snd_file = sf_open( _f, SFM_READ, &sf_info ) ) != NULL )
	{
		frames = sf_info.frames;
#endif
		_buf = new Sint16[sf_info.channels * frames];
		frames = sf_read_short( snd_file, _buf, frames );
		_channels = sf_info.channels;

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

#ifndef QT4
#define read readBlock
#define seek at 
#define pos at
#endif

size_t qfileReadCallback( void * _ptr, size_t _size, size_t _n, void * _udata )
{
	return( static_cast<QFile *>( _udata )->read( (char*)_ptr,
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

#undef read
#undef seek
#undef pos



Uint32 sampleBuffer::decodeSampleOGG( const char * _f, Sint16 * & _buf,
							Uint8 & _channels )
{
	static ov_callbacks callbacks =
	{
		qfileReadCallback,
		qfileSeekCallback,
		qfileCloseCallback,
		qfileTellCallback
	} ;

	OggVorbis_File vf;

	Uint32 frames = 0;

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
				printf( "sampleBuffer::decodeSampleOgg(): "
						"media read error\n" );
				break;
			case OV_ENOTVORBIS:
				printf( "sampleBuffer::decodeSampleOgg(): "
						"not an Ogg Vorbis file\n" );
				break;
			case OV_EVERSION:
				printf( "sampleBuffer::decodeSampleOgg(): "
						"vorbis version mismatch\n" );
				break;
			case OV_EBADHEADER:
				printf( "sampleBuffer::decodeSampleOgg(): "
					"invalid Vorbis bitstream header\n" );
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
	ogg_int64_t total = ov_pcm_total( &vf, -1 );

	_buf = new Sint16[total * _channels];
	int bitstream = 0;
	long bytes_read = 0;

	do
	{
		bytes_read = ov_read( &vf, (char *)&_buf[frames * _channels],
					( total - frames ) * _channels *
							sizeof( Sint16 ),
					isLittleEndian()? 0 : 1,
					sizeof( Sint16 ), 1, &bitstream );
		if( bytes_read < 0 )
		{
			break;
		}
		frames += bytes_read / ( _channels * sizeof( Sint16 ) );
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
	if( ( state = src_new(
#ifdef HQ_SINC
					SRC_SINC_MEDIUM_QUALITY,
#else
					SRC_ZERO_ORDER_HOLD,
#endif
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




bool FASTCALL sampleBuffer::play( sampleFrame * _ab, Uint32 _start_frame,
						Uint32 _frames, float _freq,
						bool _looped,
						void * * _resampling_data )
{
	mixer::inst()->clearAudioBuffer( _ab, _frames );

	if( m_data == NULL || m_frames == 0 || m_endFrame == 0 || _frames == 0 )
	{
		return( FALSE );
	}

	const float freq_factor = 1.0f / (BASE_FREQ / _freq);
	const Sint16 freq_diff = static_cast<Sint16>( BASE_FREQ - _freq );

	Uint32 frames_to_process = _frames;

	// calculate how many frames we have in requested pitch
	const Uint32 total_frames_for_current_pitch = static_cast<Uint32>( (
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
	const Uint32 play_frame = m_startFrame + ( _start_frame %
					total_frames_for_current_pitch );

	// this holds the number of remaining frames in current loop
	Uint32 frames_for_loop = total_frames_for_current_pitch -
						( play_frame - m_startFrame );

	// make sure, data isn't accessed in any other way (e.g. deleting
	// of this buffer...)
	m_dataMutex.lock();

	if( _looped == FALSE && frames_for_loop < frames_to_process )
	{
		frames_to_process = frames_for_loop;
	}

	// calc pointer of first frame
	sampleFrame * start_frame = (sampleFrame *) m_data +
					static_cast<Uint32>( play_frame *
								freq_factor );
	sampleFrame * loop_start = start_frame;

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
		m_srcData.data_in = start_frame[0];
		m_srcData.data_out = _ab[0];
		m_srcData.input_frames = static_cast<Uint32>( frames_for_loop *
								freq_factor );
		m_srcData.output_frames = frames_to_process;
		m_srcData.src_ratio = 1.0 / freq_factor;
		int error;
		if( ( error = src_process( state, &m_srcData ) ) )
		{
			printf( "sampleBuffer: error while resampling: %s\n",
						src_strerror( error ) );
		}
#else
		Uint32 src_frame_base = 0;
		// check whether we're in high-quality-mode
		if( mixer::inst()->highQuality() )
		{
			// we are, so let's use cubic interpolation...
			for( Uint32 frame = 0; frame < frames_to_process;
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

				const float src_frame_idx = frame*freq_factor;
				Uint32 frame_num = static_cast<Uint32>(
						src_frame_idx) - src_frame_base;
				const float frac_pos = src_frame_idx -
					static_cast<Uint32>( src_frame_idx );

				// because of cubic interpolation we have to
				// access start_frame[frame_num-1], so make
				// sure we don't access data out of
				// buffer-array-boundaries
				if( frame_num == 0 && play_frame == 0 )
				{
					frame_num = 1;
				}
				for ( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS;
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
			for( Uint32 frame = 0; frame < frames_to_process;
								++frame )
			{
				if( _looped && ( frame-src_frame_base ) >
							frames_for_loop )
				{
					start_frame = loop_start;
					src_frame_base = frame;
					frames_for_loop = frames_to_process %
						total_frames_for_current_pitch;
				}
				const float src_frame_idx = frame * freq_factor;
				const Uint32 frame_num = (Uint32)src_frame_idx -
							src_frame_base + 0;
				const float frac_pos = src_frame_idx -
							(Uint32) src_frame_idx;
				for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS;
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
		if( _looped && frames_for_loop < frames_to_process )
		{
			Uint32 total_frames_copied = 0;
			while( total_frames_copied < frames_to_process )
			{
				memcpy( _ab[total_frames_copied], start_frame,
					frames_for_loop * BYTES_PER_FRAME );
				total_frames_copied += frames_for_loop;

				// reset start_frame to start
				start_frame = loop_start;
				// and calculate frames for next loop
				frames_for_loop = frames_to_process %
						total_frames_for_current_pitch;
				if( frames_for_loop >
					frames_to_process-total_frames_copied )
				{
					frames_for_loop = frames_to_process -
							total_frames_copied;
				}
			}
		}
		else
		{
			memcpy( _ab, start_frame,
					frames_to_process * BYTES_PER_FRAME );
		}
	}

	m_dataMutex.unlock();

	return( TRUE );

}




void sampleBuffer::drawWaves( QPainter & _p, QRect _dr, drawMethods _dm )
{
	_p.setClipRect( _dr );
	_p.setPen (QColor(0x22, 0xFF, 0x44));
#ifdef QT4
	// TODO: save and restore aa-settings
	_p.setRenderHint( QPainter::Antialiasing );
#endif
	const int w = _dr.width();
	const int h = _dr.height();

	const Uint16 y_base = h/2+_dr.y();
	const float y_space = h/2;

	if( m_data == NULL || m_frames == 0 )
	{
		_p.drawLine( _dr.x(), y_base, _dr.x()+w, y_base );
		return;
	}
	else if( _dm == LINE_CONNECT )
	{
#ifdef QT4
		float old_x = _dr.x();
		float old_y[DEFAULT_CHANNELS] = { y_base, y_base };
	
		float fpp = tMax<float>( tMin<float>( m_frames / (float)w,
								20.0f ), 1.0f );
		
		for( float frame = 0; frame < m_frames; frame += fpp )
		{
			const float x = frame*w / m_frames + _dr.x();
			for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
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
		int old_x = _dr.x();
		int old_y[DEFAULT_CHANNELS] = { y_base, y_base };
	
		Uint32 fpp = tMax<Uint32>( tMin<Uint32>( m_frames / w, 20 ),
									1 );

		for( Uint32 frame = 0; frame < m_frames; frame += fpp )
		{
			const int x = static_cast<int>( frame /
							(float) m_frames * w ) +
					_dr.x();
			for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				const Uint16 y = y_base +
			static_cast<Uint16>( m_data[frame][chnl]*y_space );
				_p.drawLine( old_x, old_y[chnl], x, y );
				old_y[chnl] = y;
			}
			old_x = x;
		}
#endif

	}
	else if( _dm == DOTS )
	{
		for( Uint32 frame = 0; frame < m_frames; ++frame )
		{
			const int x = frame*w / m_frames + _dr.x();
			for( Uint8 chnl = 0; chnl < DEFAULT_CHANNELS; ++chnl )
			{
				_p.drawPoint( x, y_base +
					static_cast<Uint16>(
						m_data[frame][chnl]*y_space ) );
			}
		}
	}
	_p.setClipping( FALSE );
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
			f = configManager::inst()->samplesDir() + f;
		}
#ifdef QT4
		dir = QFileInfo( f ).absolutePath();
#else
		dir = QFileInfo( f ).dirPath( TRUE );
#endif
	}
	else
	{
		dir = configManager::inst()->samplesDir();
	}
	// change dir to position of previously opened file
	ofd.setDirectory( dir );
	ofd.setFileMode( QFileDialog::ExistingFiles );

	// set filters
#ifdef QT4
	QStringList types;
	types << tr( "All Audio-Files (*.wav *.ogg *.voc *.aif *.aiff *.au "
								"*.raw)" )
		<< tr( "Wave-Files (*.wav)" )
		<< tr( "OGG-Files (*.ogg)" )
		//<< tr( "MP3-Files (*.mp3)" )
		//<< tr( "MIDI-Files (*.mid)" )
		<< tr( "VOC-Files (*.voc)" )
		<< tr( "AIFF-Files (*.aif *.aiff)" )
		<< tr( "AU-Files (*.au)" )
		<< tr( "RAW-Files (*.raw)" )
		//<< tr( "FLAC-Files (*.flac)" )
		//<< tr( "MOD-Files (*.mod)" )
		;
	ofd.setFilters( types );
#else
	ofd.addFilter( tr( "All Audio-Files (*.wav *.ogg *.voc *.aif *.aiff "
							"*.au *.raw)" ) );
	ofd.addFilter( tr( "Wave-Files (*.wav)" ) );
	ofd.addFilter( tr( "OGG-Files (*.ogg)" ) );
	//ofd.addFilter (tr("MP3-Files (*.mp3)"));
	//ofd.addFilter (tr("MIDI-Files (*.mid)"));
	ofd.addFilter( tr( "VOC-Files (*.voc)" ) );
	ofd.addFilter( tr( "AIFF-Files (*.aif *.aiff)" ) );
	ofd.addFilter( tr( "AU-Files (*.au)" ) );
	ofd.addFilter( tr( "RAW-Files (*.raw)" ) );
	//ofd.addFilter (tr("FLAC-Files (*.flac)"));
	//ofd.addFilter (tr("MOD-Files (*.mod)"));
	ofd.setSelectedFilter( tr( "All Audio-Files (*.wav *.ogg *.voc *.aif "
						"*.aiff *.au *.raw)" ) );
#endif
	if( m_audioFile != "" )
	{
		// select previously opened file
		ofd.selectFile( QFileInfo( m_audioFile ).fileName() );
	}

	if ( ofd.exec () == QDialog::Accepted )
	{
		if( ofd.selectedFiles().isEmpty() )
		{
			return( "" );
		}
		QString sf = ofd.selectedFiles()[0];
		if( !QFileInfo( sf ).isRelative() )
		{
#if QT_VERSION >= 0x030100
			sf = sf.replace( configManager::inst()->samplesDir(),
									"" );
#else
			sf = sf.replace( QRegExp(
					configManager::inst()->samplesDir() ),
									"" );
#endif
		}
		return( sf );
	}

	return( "" );
}




void sampleBuffer::setAudioFile( const QString & _audio_file )
{
	m_audioFile = _audio_file;
	// try to make path of audio-file relative if it's posated
	// within LMMS-working-dir
	if( !QFileInfo( m_audioFile ).isRelative() )
	{
#if QT_VERSION >= 0x030100
		m_audioFile = m_audioFile.replace(
				configManager::inst()->samplesDir(), "" );
#else
		m_audioFile = m_audioFile.replace(
				QRegExp( configManager::inst()->samplesDir() ),
									"" );
#endif
	}
	update();
}




void sampleBuffer::setStartFrame( Uint32 _s )
{
	// don't set this parameter while playing
	m_dataMutex.lock();
	m_startFrame = _s;
	m_dataMutex.unlock();
}




void sampleBuffer::setEndFrame( Uint32 _e )
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



#include "sample_buffer.moc"

