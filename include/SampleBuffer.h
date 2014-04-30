/*
 * SampleBuffer.h - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QRect>

#include <samplerate.h>

#include "export.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "shared_object.h"


class QPainter;


class EXPORT SampleBuffer : public QObject, public sharedObject
{
	Q_OBJECT
public:
	class EXPORT handleState
	{
	public:
		handleState( bool _varying_pitch = false );
		virtual ~handleState();

		inline const f_cnt_t frameIndex() const
		{
			return m_frameIndex;
		}

		inline void setFrameIndex( f_cnt_t _index )
		{
			m_frameIndex = _index;
		}



	private:
		f_cnt_t m_frameIndex;
		const bool m_varyingPitch;
		SRC_STATE * m_resamplingData;

		friend class SampleBuffer;

	} ;


	// constructor which either loads sample _audio_file or decodes
	// base64-data out of string
	SampleBuffer( const QString & _audio_file = QString(),
						bool _is_base64_data = false );
	SampleBuffer( const sampleFrame * _data, const f_cnt_t _frames );
	SampleBuffer( const f_cnt_t _frames );

	virtual ~SampleBuffer();

	bool play( sampleFrame * _ab, handleState * _state,
				const fpp_t _frames,
				const float _freq,
				const bool _looped = false );

	void visualize( QPainter & _p, const QRect & _dr, const QRect & _clip, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0 );
	inline void visualize( QPainter & _p, const QRect & _dr, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0 )
	{
		visualize( _p, _dr, _dr, _from_frame, _to_frame );
	}

	inline const QString & audioFile() const
	{
		return m_audioFile;
	}

	f_cnt_t startFrame() const
	{
		return m_startFrame;
	}

	f_cnt_t endFrame() const
	{
		return m_endFrame;
	}

	void setLoopStartFrame( f_cnt_t start );
	void setLoopEndFrame( f_cnt_t end );

	inline f_cnt_t frames() const
	{
		return m_frames;
	}

	inline float amplification() const
	{
		return m_amplification;
	}

	inline bool reversed() const
	{
		return m_reversed;
	}

	inline float frequency() const
	{
		return m_frequency;
	}

	sample_rate_t sampleRate() const
	{
		return m_sampleRate;
	}

	int sampleLength() const
	{
		return double( m_endFrame - m_startFrame ) / m_sampleRate * 1000;
	}

	inline void setFrequency( float _freq )
	{
		m_varLock.lock();
		m_frequency = _freq;
		m_varLock.unlock();
	}

	inline void setSampleRate( sample_rate_t _rate )
	{
		m_varLock.lock();
		m_sampleRate = _rate;
		m_varLock.unlock();
	}

	inline const sampleFrame * data() const
	{
		return m_data;
	}

    QString openAudioFile() const;
    QString openAndSetAudioFile();
	QString openAndSetWaveformFile();
	
	QString & toBase64( QString & _dst ) const;


	static SampleBuffer * resample( sampleFrame * _data,
						const f_cnt_t _frames,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr );

	static inline SampleBuffer * resample( SampleBuffer * _buf,
						const sample_rate_t _src_sr,
						const sample_rate_t _dst_sr )
	{
		return resample( _buf->m_data, _buf->m_frames, _src_sr,
								_dst_sr );
	}

	void normalizeSampleRate( const sample_rate_t _src_sr,
						bool _keep_settings = false );

	inline sample_t userWaveSample( const float _sample ) const
	{
		// Precise implementation
//		const float frame = fraction( _sample ) * m_frames;
//		const f_cnt_t f1 = static_cast<f_cnt_t>( frame );
//		const f_cnt_t f2 = ( f1 + 1 ) % m_frames;
//		sample_t waveSample = linearInterpolate( m_data[f1][0],
//						m_data[f2][0],
//						fraction( frame ) );
//		return waveSample;

		// Fast implementation
		const float frame = _sample * m_frames;
		f_cnt_t f1 = static_cast<f_cnt_t>( frame ) % m_frames;
		if( f1 < 0 )
		{
			f1 += m_frames;
		}
		return m_data[f1][0];
	}

	static QString tryToMakeRelative( const QString & _file );
	static QString tryToMakeAbsolute( const QString & _file );


public slots:
	void setAudioFile( const QString & _audio_file );
	void loadFromBase64( const QString & _data );
	void setStartFrame( f_cnt_t start );
	void setEndFrame( f_cnt_t end );
	void setAmplification( float _a );
	void setReversed( bool _on );


private:
	/*! Ensures that all settings are sane like end frame > start frame etc. */
	void sanitizeSettings();

	void update( bool _keep_settings = false );

    void convertIntToFloat ( int_sample_t * & _ibuf, f_cnt_t _frames, int _channels);
    void directFloatWrite ( sample_t * & _fbuf, f_cnt_t _frames, int _channels);

	f_cnt_t decodeSampleSF( const char * _f, int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate );
#ifdef LMMS_HAVE_OGGVORBIS
	f_cnt_t decodeSampleOGGVorbis( const char * _f, int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate );
#endif
	f_cnt_t decodeSampleDS( const char * _f, int_sample_t * & _buf,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate );

	QString m_audioFile;
	sampleFrame * m_origData;
	f_cnt_t m_origFrames;
	sampleFrame * m_data;
	QMutex m_varLock;
	f_cnt_t m_frames;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loopStartFrame;
	f_cnt_t m_loopEndFrame;
	float m_amplification;
	bool m_reversed;
	float m_frequency;
	sample_rate_t m_sampleRate;

	sampleFrame * getSampleFragment( f_cnt_t _start, f_cnt_t _frames,
						bool _looped,
						sampleFrame * * _tmp ) const;
	f_cnt_t getLoopedIndex( f_cnt_t _index ) const;


signals:
	void sampleUpdated();

} ;

#endif
