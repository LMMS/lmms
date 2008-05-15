/*
 * mixer.h - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef USE_3RDPARTY_LIBSRC
#include <samplerate.h>
#else
#include "src/3rdparty/samplerate/samplerate.h"
#endif


#include <QtCore/QMutex>
#include <QtCore/QSemaphore>
#include <QtCore/QThread>
#include <QtCore/QVector>


#include "types.h"
#include "note.h"
#include "fifo_buffer.h"


class audioDevice;
class midiClient;
class audioPort;


const fpp_t DEFAULT_BUFFER_SIZE = 256;

const ch_cnt_t DEFAULT_CHANNELS = 2;

const ch_cnt_t SURROUND_CHANNELS =
#ifndef DISABLE_SURROUND
				4;
#else
				2;
#endif



typedef sample_t sampleFrame[DEFAULT_CHANNELS];
typedef sample_t surroundSampleFrame[SURROUND_CHANNELS];


const Uint8 BYTES_PER_SAMPLE = sizeof( sample_t );
const Uint8 BYTES_PER_INT_SAMPLE = sizeof( int_sample_t );
const Uint8 BYTES_PER_FRAME = sizeof( sampleFrame );
const Uint8 BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;


const float BaseFreq = 440.0f;
const Keys BaseKey = Key_A;
const Octaves BaseOctave = DefaultOctave;


#include "play_handle.h"


class mixerWorkerThread;


class mixer : public QObject
{
	Q_OBJECT
public:
	struct qualitySettings
	{
		enum Mode
		{
			Mode_Draft,
			Mode_HighQuality,
			Mode_FinalMix
		} ;

		enum Interpolation
		{
			Interpolation_Linear,
			Interpolation_SincFastest,
			Interpolation_SincMedium,
			Interpolation_SincBest
		} ; 

		enum Oversampling
		{
			Oversampling_None,
			Oversampling_2x,
			Oversampling_4x,
			Oversampling_8x
		} ;

		Interpolation interpolation;
		Oversampling oversampling;
		bool sampleExactControllers;
		bool aliasFreeOscillators;

		qualitySettings( Mode _m )
		{
			switch( _m )
			{
				case Mode_Draft:
					interpolation = Interpolation_Linear;
					oversampling = Oversampling_None;
					sampleExactControllers = FALSE;
					aliasFreeOscillators = FALSE;
					break;
				case Mode_HighQuality:
					interpolation =
						Interpolation_SincFastest;
					oversampling = Oversampling_2x;
					sampleExactControllers = TRUE;
					aliasFreeOscillators = FALSE;
					break;
				case Mode_FinalMix:
					interpolation = Interpolation_SincBest;
					oversampling = Oversampling_8x;
					sampleExactControllers = TRUE;
					aliasFreeOscillators = TRUE;
					break;
			}
		}

		qualitySettings( Interpolation _i, Oversampling _o, bool _sec,
								bool _afo ) :
			interpolation( _i ),
			oversampling( _o ),
			sampleExactControllers( _sec ),
			aliasFreeOscillators( _afo )
		{
		}

		int sampleRateMultiplier( void ) const
		{
			switch( oversampling )
			{
				case Oversampling_None: return 1;
				case Oversampling_2x: return 2;
				case Oversampling_4x: return 4;
				case Oversampling_8x: return 8;
			}
			return( 1 );
		}

		int libsrcInterpolation( void ) const
		{
			switch( interpolation )
			{
				case Interpolation_Linear:
					return( SRC_ZERO_ORDER_HOLD );
				case Interpolation_SincFastest:
					return( SRC_SINC_FASTEST );
				case Interpolation_SincMedium:
					return( SRC_SINC_MEDIUM_QUALITY );
				case Interpolation_SincBest:
					return( SRC_SINC_BEST_QUALITY );
			}
			return( SRC_LINEAR );
		}
	} ;

	void initDevices( void );
	void clear( void );


	// audio-device-stuff
	inline const QString & audioDevName( void ) const
	{
		return( m_audioDevName );
	}

	void setAudioDevice( audioDevice * _dev );
	void setAudioDevice( audioDevice * _dev,
					const struct qualitySettings & _qs );
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

	inline const surroundSampleFrame * currentReadBuffer( void ) const
	{
		return( m_readBuf );
	}


	inline Uint8 cpuLoad( void ) const
	{
		return( m_cpuLoad );
	}

	const qualitySettings & currentQualitySettings( void ) const
	{
		return( m_qualitySettings );
	}


	sample_rate_t baseSampleRate( void ) const;
	sample_rate_t outputSampleRate( void ) const;
	sample_rate_t processingSampleRate( void ) const;


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
	void bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					stereoVolumeVector _volume_vector,
					audioPort * _port );

	static void clearAudioBuffer( sampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#ifndef DISABLE_SURROUND
	static void clearAudioBuffer( surroundSampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#endif

	static float peakValueLeft( sampleFrame * _ab, const f_cnt_t _frames );
	static float peakValueRight( sampleFrame * _ab, const f_cnt_t _frames );


	bool criticalXRuns( void ) const;

	const surroundSampleFrame * nextBuffer( void )
	{
		return( m_fifo->read() );
	}


	void changeQuality( const struct qualitySettings & _qs );


signals:
	void qualitySettingsChanged( void );
	void sampleRateChanged( void );
	void nextAudioBuffer( void );


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
		volatile bool m_writing;

		virtual void run( void );

	} ;


	mixer( void );
	virtual ~mixer();

	void startProcessing( void );
	void stopProcessing( void );


	audioDevice * tryAudioDevices( void );
	midiClient * tryMIDIClients( void );


	const surroundSampleFrame * renderNextBuffer( void );



	QVector<audioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

	sampleFrame * m_workingBuf;

	surroundSampleFrame * m_readBuf;
	surroundSampleFrame * m_writeBuf;
	
	QVector<surroundSampleFrame *> m_bufferPool;
	Uint8 m_readBuffer;
	Uint8 m_writeBuffer;
	Uint8 m_analBuffer;
	Uint8 m_poolDepth;

	surroundSampleFrame m_maxClip;
	surroundSampleFrame m_previousSample;
	fpp_t m_halfStart[SURROUND_CHANNELS];
	bool m_oldBuffer[SURROUND_CHANNELS];
	bool m_newBuffer[SURROUND_CHANNELS];
	
	Uint8 m_cpuLoad;
	bool m_multiThreaded;
	QVector<mixerWorkerThread *> m_workers;
	int m_numWorkers;
	QSemaphore m_workerSem;


	playHandleVector m_playHandles;
	constPlayHandleVector m_playHandlesToRemove;

	struct qualitySettings m_qualitySettings;
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
	friend class mixerWorkerThread;

} ;


#endif
