/*
 * mixer.h - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIXER_H
#define _MIXER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QMutex>
#include <QVector>

#else

#include <qobject.h>
#include <qmutex.h>
#include <qvaluevector.h>

#endif


#include "types.h"
#include "volume.h"
#include "panning.h"
#include "note.h"
#include "play_handle.h"
#include "effect_board.h"
#include "engine.h"


class audioDevice;
class midiClient;
class audioPort;


const fpab_t DEFAULT_BUFFER_SIZE = 512;

const ch_cnt_t DEFAULT_CHANNELS = 2;

const ch_cnt_t SURROUND_CHANNELS =
#ifndef DISABLE_SURROUND
				4;
#else
				2;
#endif


enum qualityLevels
{
	DEFAULT_QUALITY_LEVEL,
	HIGH_QUALITY_LEVEL,
	QUALITY_LEVELS
} ;

extern sample_rate_t SAMPLE_RATES[QUALITY_LEVELS];
const sample_rate_t DEFAULT_SAMPLE_RATE = 44100;


typedef sample_t sampleFrame[DEFAULT_CHANNELS];
typedef sample_t surroundSampleFrame[SURROUND_CHANNELS];

typedef struct
{
	float vol[SURROUND_CHANNELS];
} volumeVector;


const Uint8 BYTES_PER_SAMPLE = sizeof( sample_t );
const Uint8 BYTES_PER_INT_SAMPLE = sizeof( int_sample_t );
const Uint8 BYTES_PER_FRAME = sizeof( sampleFrame );
const Uint8 BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;


const float BASE_FREQ = 440.0f;
const tones BASE_TONE = A;
const octaves BASE_OCTAVE = OCTAVE_4;



class mixer : public QObject, public engineObject
{
	Q_OBJECT
public:
	void initDevices( void );
	void FASTCALL clear( const bool _everything = FALSE );


	// audio-device-stuff
	inline const QString & audioDevName( void ) const
	{
		return( m_audioDevName );
	}

	void FASTCALL setAudioDevice( audioDevice * _dev, bool _hq );
	void restoreAudioDevice( void );
	inline audioDevice * audioDev( void )
	{
		return( m_audioDev );
	}


	// audio-port-stuff
	inline void addAudioPort( audioPort * _port )
	{
		pause();
		m_audioPorts.push_back( _port );
		play();
	}

	inline void removeAudioPort( audioPort * _port )
	{
		vvector<audioPort *>::iterator it = qFind( m_audioPorts.begin(),
							m_audioPorts.end(),
							_port );
		if( it != m_audioPorts.end() )
		{
			m_audioPorts.erase( it );
		}
	}


	// MIDI-client-stuff
	inline const QString & midiClientName( void ) const
	{
		return( m_midiClientName );
	}

	inline midiClient * getMIDIClient( void )
	{
		return( m_midiClient );
	}


	// play-handle stuff
	inline bool addPlayHandle( playHandle * _ph )
	{
		if( criticalXRuns() == FALSE )
		{
			m_playHandles.push_back( _ph );
			return( TRUE );
		}
		else
		{
			delete _ph;
		}
		return( FALSE );
	}

	inline void removePlayHandle( playHandle * _ph )
	{
		m_playHandlesToRemove.push_back( _ph );
	}

	inline const playHandleVector & playHandles( void ) const
	{
		return( m_playHandles );
	}

	void checkValidityOfPlayHandles( void );

	inline bool haveNoRunningNotes( void ) const
	{
		return( m_playHandles.size() == 0 );
	}


	// methods providing information for other classes
	inline fpab_t framesPerAudioBuffer( void ) const
	{
		return( m_framesPerAudioBuffer );
	}

	inline const surroundSampleFrame * currentAudioBuffer( void ) const
	{
		return( m_curBuf );
	}


	inline Uint8 cpuLoad( void ) const
	{
		return( m_cpuLoad );
	}

	inline bool highQuality( void ) const
	{
		return( m_qualityLevel > DEFAULT_QUALITY_LEVEL );
	}


	inline sample_rate_t sampleRate( void ) const
	{
		return( SAMPLE_RATES[m_qualityLevel] );
	}


	inline float masterGain( void ) const
	{
		return( m_masterGain );
	}

	inline void setMasterGain( const float _mo )
	{
		m_masterGain = _mo;
	}


	static inline sample_t clip( const sample_t _s )
	{
		if( _s > 1.0f )
		{
			return( 1.0f );
		}
		else if( _s < -1.0f )
		{
			return( -1.0f );
		}
		return( _s );
	}


	// methods for controlling mixer-state
	void pause( void )
	{
		if( m_mixMutexLockLevel == 0 )
		{
			m_mixMutex.lock();
		}
		++m_mixMutexLockLevel;
	}

	void play( void )
	{
		if( m_mixMutexLockLevel == 1 )
		{
			m_mixMutex.unlock();
		}
		--m_mixMutexLockLevel;
	}


	// audio-buffer-mgm
	void FASTCALL bufferToPort( const sampleFrame * _buf,
					const fpab_t _frames,
					const fpab_t _framesAhead,
					const volumeVector & _volumeVector,
							audioPort * _port );

	void FASTCALL clearAudioBuffer( sampleFrame * _ab,
							const f_cnt_t _frames );
#ifndef DISABLE_SURROUND
	void FASTCALL clearAudioBuffer( surroundSampleFrame * _ab,
							const f_cnt_t _frames );
#endif

	bool criticalXRuns( void ) const;

	const surroundSampleFrame * renderNextBuffer( void );


public slots:
	void setHighQuality( bool _hq_on = FALSE );


signals:
	void sampleRateChanged( void );
	void nextAudioBuffer( const surroundSampleFrame *,
						const fpab_t _frames );


private:
	mixer( engine * _engine );
	~mixer();

	void startProcessing( void );
	void stopProcessing( void );


	// we don't allow to create mixer by using copy-ctor
	mixer( const mixer & ) :
		engineObject( NULL )
	{
	}



	audioDevice * tryAudioDevices( void );
	midiClient * tryMIDIClients( void );

	void processBuffer( const surroundSampleFrame * _buf,
						const fx_ch_t _fx_chnl );



	vvector<audioPort *> m_audioPorts;

	fpab_t m_framesPerAudioBuffer;

	surroundSampleFrame * m_curBuf;
	surroundSampleFrame * m_nextBuf;

	Uint8 m_cpuLoad;

	playHandleVector m_playHandles;
	playHandleVector m_playHandlesToRemove;

	qualityLevels m_qualityLevel;
	float m_masterGain;


	audioDevice * m_audioDev;
	audioDevice * m_oldAudioDev;
	QString m_audioDevName;


	midiClient * m_midiClient;
	QString m_midiClientName;


	QMutex m_mixMutex;
	Uint8 m_mixMutexLockLevel;


	friend class engine;

} ;


#endif
