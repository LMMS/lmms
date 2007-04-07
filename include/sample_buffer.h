/*
 * sample_buffer.h - container-class sampleBuffer
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _SAMPLE_BUFFER_H
#define _SAMPLE_BUFFER_H

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QObject>
#include <QtCore/QMutex>

#else

#include <qobject.h>
#include <qmutex.h>

#endif


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SAMPLERATE_H
#include <samplerate.h>
#endif


#include "mixer.h"
#include "interpolation.h"
#include "types.h"
#include "lmms_math.h"
#include "shared_object.h"


class QPainter;


class sampleBuffer : public QObject, public engineObject, public sharedObject
{
	Q_OBJECT
public:
	enum drawMethods
	{
		LINE_CONNECT,
		DOTS
	} ;


	class handleState
	{
	public:
		handleState( bool _varying_pitch = FALSE );
		virtual ~handleState();


	private:
		f_cnt_t m_frame_index;
		const bool m_varying_pitch;
#ifdef HAVE_SAMPLERATE_H
		SRC_STATE * m_resampling_data;
#endif

		friend class sampleBuffer;

	} ;


	// constructor which either loads sample _audio_file or decodes
	// base64-data out of string
	sampleBuffer( engine * _engine, const QString & _audio_file = "",
						bool _is_base64_data = FALSE );
	sampleBuffer( const sampleFrame * _data, const f_cnt_t _frames,
							engine * _engine );
	sampleBuffer( const f_cnt_t _frames, engine * _engine );
	
	virtual ~sampleBuffer();

	bool FASTCALL play( sampleFrame * _ab, handleState * _state,
				const fpab_t _frames,
				const float _freq = BASE_FREQ,
				const bool _looped = FALSE );

	void FASTCALL visualize( QPainter & _p, const QRect & _dr,
					const QRect & _clip,
					drawMethods _dm = LINE_CONNECT );
	inline void visualize( QPainter & _p, const QRect & _dr,
					drawMethods _dm = LINE_CONNECT )
	{
		visualize( _p, _dr, _dr, _dm );
	}

	inline const QString & audioFile( void ) const
	{
		return( m_audioFile );
	}

	inline f_cnt_t startFrame( void ) const
	{
		return( m_startFrame );
	}

	inline f_cnt_t endFrame( void ) const
	{
		return( m_endFrame );
	}

	void setLoopStartFrame( f_cnt_t _start )
	{
		m_loop_startFrame = _start;
	}

	void setLoopEndFrame( f_cnt_t _end )
	{
		m_loop_endFrame = _end;
	}

	inline f_cnt_t frames( void ) const
	{
		return( m_frames );
	}

	inline float amplification( void ) const
	{
		return( m_amplification );
	}

	inline bool reversed( void ) const
	{
		return( m_reversed );
	}

	inline float frequency( void ) const
	{
		return( m_frequency );
	}

	inline void setFrequency( float _freq )
	{
		m_frequency = _freq;
	}

	inline const sampleFrame * data( void ) const
	{
		return( m_data );
	}

	QString openAudioFile( void ) const;

	QString & toBase64( QString & _dst ) const;


	static sampleBuffer * FASTCALL resample( sampleFrame * _data,
						const f_cnt_t _frames,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr,
						engine * _engine );

	static inline sampleBuffer * FASTCALL resample(
						sampleBuffer * _buf,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
	{
		return( resample( _buf->m_data, _buf->m_frames, _src_sr,
						_dst_sr, _buf->eng() ) );
	}

	void normalize_sample_rate( const sample_rate_t _src_sr,
						bool _keep_settings = FALSE );

	inline sample_t userWaveSample( const float _sample )
	{
		// Precise implementation
//		const float frame = fraction( _sample ) * m_frames;
//		const f_cnt_t f1 = static_cast<f_cnt_t>( frame );
//		const f_cnt_t f2 = ( f1 + 1 ) % m_frames;
//		sample_t waveSample = linearInterpolate( m_data[f1][0],
//						m_data[f2][0],
//						fraction( frame ) );
//		return( waveSample );

		// Fast implementation
		const float frame = _sample * m_frames;
		f_cnt_t f1 = static_cast<f_cnt_t>( frame ) % m_frames;
		if( f1 < 0 )
		{
			f1 += m_frames;
		}
		return( m_data[f1][0] );
	}

	void lock( void )
	{
		m_dataMutex.lock();
	}
	void unlock( void )
	{
		m_dataMutex.unlock();
	}

	static QString tryToMakeRelative( const QString & _file );
	static QString tryToMakeAbsolute( const QString & _file );


public slots:
	void setAudioFile( const QString & _audio_file );
	void loadFromBase64( const QString & _data );
	void setStartFrame( const f_cnt_t _s );
	void setEndFrame( const f_cnt_t _e );
	void setAmplification( float _a );
	void setReversed( bool _on );


private:
	void FASTCALL update( bool _keep_settings = FALSE );


#ifdef SDL_SDL_SOUND_H
	f_cnt_t FASTCALL decodeSampleSDL( const char * _f,
						int_sample_t * & _buf,
						ch_cnt_t _channels,
						sample_rate_t _sample_rate );
#endif
#ifdef HAVE_SNDFILE_H
	f_cnt_t FASTCALL decodeSampleSF( const char * _f,
						int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate );
#endif
#ifdef HAVE_VORBIS_VORBISFILE_H
	f_cnt_t FASTCALL decodeSampleOGGVorbis( const char * _f,
						int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate );
#endif

	QString m_audioFile;
	sampleFrame * m_origData;
	f_cnt_t m_origFrames;
	sampleFrame * m_data;
	f_cnt_t m_frames;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loop_startFrame;
	f_cnt_t m_loop_endFrame;
	float m_amplification;
	bool m_reversed;
	float m_frequency;
	QMutex m_dataMutex;

#ifdef HAVE_SAMPLERATE_H
	void initResampling( void );

	SRC_DATA m_srcData;
#endif

	sampleFrame * m_sample_fragment;
	sampleFrame * getSampleFragment( f_cnt_t _start, f_cnt_t _frames,
								bool _looped );
	f_cnt_t getLoopedIndex( f_cnt_t _index );


signals:
	void sampleUpdated( void );

} ;


#endif
