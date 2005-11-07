/*
 * sample_buffer.h - container-class sampleBuffer
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


#ifndef _SAMPLE_BUFFER_H
#define _SAMPLE_BUFFER_H

#include "qt3support.h"

#ifdef QT4

#include <QObject>
#include <QMutex>

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
#include "types.h"


class QPainter;


class sampleBuffer : public QObject
{
	Q_OBJECT
public:
	enum drawMethods
	{
		LINE_CONNECT,
		DOTS
	} ;

	sampleBuffer( const QString & _audio_file = "" );
	sampleBuffer( const sampleFrame * _data, Uint32 _frames );
	
	~sampleBuffer();

	bool FASTCALL play( sampleFrame * _ab, Uint32 _start_frame,
				Uint32 _frames =
					mixer::inst()->framesPerAudioBuffer(),
				float _freq = BASE_FREQ, bool _looped = FALSE,
				void * * _resampling_data = NULL );
	void FASTCALL drawWaves( QPainter & _p, QRect _dr,
					drawMethods _dm = LINE_CONNECT );
	inline const QString & audioFile( void ) const
	{
		return( m_audioFile );
	}
	inline Uint32 startFrame( void ) const
	{
		return( m_startFrame );
	}
	inline Uint32 endFrame( void ) const
	{
		return( m_endFrame );
	}
	inline Uint32 frames( void ) const
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
	inline const sampleFrame * data( void ) const
	{
		return( m_data );
	}

	void FASTCALL deleteResamplingData( void * * _ptr );

	QString openAudioFile( void ) const;



public slots:
	void setAudioFile( const QString & _audio_file );
	void setStartFrame( Uint32 _s );
	void setEndFrame( Uint32 _e );
	void setAmplification( float _a );
	void setReversed( bool _on );


private:
	void FASTCALL update( bool _keep_settings = FALSE );

#ifdef SDL_SDL_SOUND_H
	Uint32 FASTCALL decodeSampleSDL( const char * _f, Sint16 * & _buf,
							Uint8 & _channels );
#endif
#ifdef HAVE_SNDFILE_H
	Uint32 FASTCALL decodeSampleSF( const char * _f, Sint16 * & _buf,
							Uint8 & _channels );
#endif
#ifdef HAVE_VORBIS_VORBISFILE_H
	Uint32 FASTCALL decodeSampleOGG( const char * _f, Sint16 * & _buf,
							Uint8 & _channels );
#endif

	QString m_audioFile;
	sampleFrame * m_origData;
	Uint32 m_origFrames;
	sampleFrame * m_data;
	Uint32 m_frames;
	Uint32 m_startFrame;
	Uint32 m_endFrame;
	float m_amplification;
	bool m_reversed;
	QMutex m_dataMutex;

#ifdef HAVE_SAMPLERATE_H
	void initResampling( void );
	void quitResampling( void );
	SRC_STATE * createResamplingContext( void );
	void FASTCALL destroyResamplingContext( SRC_STATE * _context );

	SRC_DATA m_srcData;
	SRC_STATE * m_srcState;
#endif


signals:
	void sampleUpdated( void );

} ;


#endif
