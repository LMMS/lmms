/*
 * mixer.h - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MIXER_H
#define _MIXER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QVector>


#include "types.h"
#include "note.h"
#include "play_handle.h"
#include "fifo_buffer.h"


class audioDevice;
class midiClient;
class audioPort;


const fpp_t DEFAULT_BUFFER_SIZE = 512;

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



class mixer : public QObject
{
	Q_OBJECT
public:
	void initDevices( void );
	void FASTCALL clear( void );


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
		lock();
		m_audioPorts.push_back( _port );
		unlock();
	}

	inline void removeAudioPort( audioPort * _port )
	{
		lock();
		QVector<audioPort *>::iterator it = qFind( m_audioPorts.begin(),
							m_audioPorts.end(),
							_port );
		if( it != m_audioPorts.end() )
		{
			m_audioPorts.erase( it );
		}
		unlock();
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
			lockPlayHandles();
			m_playHandles.push_back( _ph );
			unlockPlayHandles();
			return( TRUE );
		}
		delete _ph;
		return( FALSE );
	}

	inline void removePlayHandle( const playHandle * _ph )
	{
		lockPlayHandlesToRemove();
		m_playHandlesToRemove.push_back( _ph );
		unlockPlayHandlesToRemove();
	}

	inline playHandleVector & playHandles( void )
	{
		return( m_playHandles );
	}

	void removePlayHandles( track * _track );

	inline bool hasPlayHandles( void ) const
	{
		return( !m_playHandles.empty() );
	}


	// methods providing information for other classes
	inline fpp_t framesPerPeriod( void ) const
	{
		return( m_framesPerPeriod );
	}

	inline const surroundSampleFrame * currentAudioBuffer( void ) const
	{
		return( m_writeBuf );
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


	// methods needed by other threads to alter knob values, waveforms, etc
	void lock( void )
	{
		m_globalMutex.lock();
	}

	void unlock( void )
	{
		m_globalMutex.unlock();
	}

	void lockPlayHandles( void )
	{
		m_playHandlesMutex.lock();
	}

	void unlockPlayHandles( void )
	{
		m_playHandlesMutex.unlock();
	}

	void lockPlayHandlesToRemove( void )
	{
		m_playHandlesToRemoveMutex.lock();
	}

	void unlockPlayHandlesToRemove( void )
	{
		m_playHandlesToRemoveMutex.unlock();
	}

	// audio-buffer-mgm
	void FASTCALL bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					const volumeVector & _volumeVector,
							audioPort * _port );

	void FASTCALL clearAudioBuffer( sampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#ifndef DISABLE_SURROUND
	void FASTCALL clearAudioBuffer( surroundSampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#endif

	bool criticalXRuns( void ) const;

	const surroundSampleFrame * nextBuffer( void )
	{
		return( m_fifo->read() );
	}


public slots:
	void setHighQuality( bool _hq_on = FALSE );
	void setClipScaling( bool _state );


signals:
	void sampleRateChanged( void );
	void nextAudioBuffer( const surroundSampleFrame *, int _frames );


private:
	typedef fifoBuffer<surroundSampleFrame *> fifo;

	class fifoWriter : public QThread
	{
	public:
		fifoWriter( mixer * _mixer, fifo * _fifo );

		void finish( void );


	private:
		mixer * m_mixer;
		fifo * m_fifo;
		bool m_writing;

		virtual void run( void );

	} ;


	mixer( void );
	virtual ~mixer();

	void startProcessing( void );
	void stopProcessing( void );


	audioDevice * tryAudioDevices( void );
	midiClient * tryMIDIClients( void );

	void processBuffer( const surroundSampleFrame * _buf,
						const fx_ch_t _fx_chnl );

	void FASTCALL scaleClip( fpp_t _frame, ch_cnt_t _chnl );

	const surroundSampleFrame * renderNextBuffer( void );

	QVector<audioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

	surroundSampleFrame * m_readBuf;
	surroundSampleFrame * m_writeBuf;
	
	QVector<surroundSampleFrame *> m_bufferPool;
	Uint8 m_readBuffer;
	Uint8 m_writeBuffer;
	Uint8 m_analBuffer;
	Uint8 m_poolDepth;

	bool m_scaleClip;
	surroundSampleFrame m_maxClip;
	surroundSampleFrame m_previousSample;
	fpp_t m_halfStart[SURROUND_CHANNELS];
	bool m_clipped[SURROUND_CHANNELS];
	bool m_oldBuffer[SURROUND_CHANNELS];
	bool m_newBuffer[SURROUND_CHANNELS];
	
	Uint8 m_cpuLoad;
	int m_parallelizingLevel;

	playHandleVector m_playHandles;
	constPlayHandleVector m_playHandlesToRemove;

	qualityLevels m_qualityLevel;
	float m_masterGain;


	audioDevice * m_audioDev;
	audioDevice * m_oldAudioDev;
	QString m_audioDevName;


	midiClient * m_midiClient;
	QString m_midiClientName;


	QMutex m_globalMutex;
	QMutex m_playHandlesMutex;
	QMutex m_playHandlesToRemoveMutex;


	fifo * m_fifo;
	fifoWriter * m_fifo_writer;


	friend class engine;

} ;


#endif
