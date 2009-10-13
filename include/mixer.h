/*
 * mixer.h - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "lmmsconfig.h"

#ifndef LMMS_USE_3RDPARTY_LIBSRC
#include <samplerate.h>
#else
#ifndef OUT_OF_TREE_BUILD
#include "src/3rdparty/samplerate/samplerate.h"
#else
#include <samplerate.h>
#endif
#endif


#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QVector>
#include <QtCore/QWaitCondition>


#include "lmms_basics.h"
#include "note.h"
#include "fifo_buffer.h"


class AudioDevice;
class MidiClient;
class AudioPort;


const fpp_t DEFAULT_BUFFER_SIZE = 256;


const float BaseFreq = 440.0f;
const Keys BaseKey = Key_A;
const Octaves BaseOctave = DefaultOctave;



class MixerWorkerThread;

// TODO: move to ThreadableJob.h
class ThreadableJob
{
public:

	enum ProcessingState
	{
		Unstarted,
		Queued,
		InProgress,
		Done
	};

	ThreadableJob() :
		m_state( ThreadableJob::Unstarted )
	{
	}

	void reset()
	{
		m_state = ThreadableJob::Unstarted;
	}

	bool process( sampleFrame * _working_buffer )
	{
		if( m_state.testAndSetOrdered( Queued, InProgress ) )
		{
			doProcessing( _working_buffer );
			m_state = Done;
			return true;
		}
		return false;
	}

	virtual bool requiresProcessing() const = 0;

	QAtomicInt m_state;

private:
	virtual void doProcessing( sampleFrame * _working_buffer ) = 0;

} ;


#include "play_handle.h"


class EXPORT mixer : public QObject
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
					sampleExactControllers = false;
					aliasFreeOscillators = false;
					break;
				case Mode_HighQuality:
					interpolation =
						Interpolation_SincFastest;
					oversampling = Oversampling_2x;
					sampleExactControllers = true;
					aliasFreeOscillators = false;
					break;
				case Mode_FinalMix:
					interpolation = Interpolation_SincBest;
					oversampling = Oversampling_8x;
					sampleExactControllers = true;
					aliasFreeOscillators = true;
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

		int sampleRateMultiplier() const
		{
			switch( oversampling )
			{
				case Oversampling_None: return 1;
				case Oversampling_2x: return 2;
				case Oversampling_4x: return 4;
				case Oversampling_8x: return 8;
			}
			return 1;
		}

		int libsrcInterpolation() const
		{
			switch( interpolation )
			{
				case Interpolation_Linear:
					return SRC_ZERO_ORDER_HOLD;
				case Interpolation_SincFastest:
					return SRC_SINC_FASTEST;
				case Interpolation_SincMedium:
					return SRC_SINC_MEDIUM_QUALITY;
				case Interpolation_SincBest:
					return SRC_SINC_BEST_QUALITY;
			}
			return SRC_LINEAR;
		}
	} ;

	void initDevices();
	void clear();


	// audio-device-stuff
	inline const QString & audioDevName() const
	{
		return m_audioDevName;
	}

	void setAudioDevice( AudioDevice * _dev );
	void setAudioDevice( AudioDevice * _dev,
				const struct qualitySettings & _qs,
							bool _needs_fifo );
	void restoreAudioDevice();
	inline AudioDevice * audioDev()
	{
		return m_audioDev;
	}


	// audio-port-stuff
	inline void addAudioPort( AudioPort * _port )
	{
		lock();
		m_audioPorts.push_back( _port );
		unlock();
	}

	void removeAudioPort( AudioPort * _port );


	// MIDI-client-stuff
	inline const QString & midiClientName() const
	{
		return m_midiClientName;
	}

	inline MidiClient * midiClient()
	{
		return m_midiClient;
	}


	// play-handle stuff
	inline bool addPlayHandle( playHandle * _ph )
	{
		if( criticalXRuns() == false )
		{
			lock();
			m_playHandles.push_back( _ph );
			unlock();
			return true;
		}
		delete _ph;
		return false;
	}

	void removePlayHandle( playHandle * _ph );

	inline PlayHandleList & playHandles()
	{
		return m_playHandles;
	}

	void removePlayHandles( track * _track,
		playHandle::Type _type = playHandle::NumPlayHandleTypes );

	inline bool hasPlayHandles() const
	{
		return !m_playHandles.empty();
	}


	// methods providing information for other classes
	inline fpp_t framesPerPeriod() const
	{
		return m_framesPerPeriod;
	}

	inline const surroundSampleFrame * currentReadBuffer() const
	{
		return m_readBuf;
	}


	inline int cpuLoad() const
	{
		return m_cpuLoad;
	}

	const qualitySettings & currentQualitySettings() const
	{
		return m_qualitySettings;
	}


	sample_rate_t baseSampleRate() const;
	sample_rate_t outputSampleRate() const;
	sample_rate_t inputSampleRate() const;
	sample_rate_t processingSampleRate() const;


	inline float masterGain() const
	{
		return m_masterGain;
	}

	inline void setMasterGain( const float _mo )
	{
		m_masterGain = _mo;
	}


	static inline sample_t clip( const sample_t _s )
	{
		if( _s > 1.0f )
		{
			return 1.0f;
		}
		else if( _s < -1.0f )
		{
			return -1.0f;
		}
		return _s;
	}


	// methods needed by other threads to alter knob values, waveforms, etc
	void lock()
	{
		m_globalMutex.lock();
	}

	void unlock()
	{
		m_globalMutex.unlock();
	}

	void lockInputFrames()
	{
		m_inputFramesMutex.lock();
	}

	void unlockInputFrames()
	{
		m_inputFramesMutex.unlock();
	}

	// audio-buffer-mgm
	void bufferToPort( const sampleFrame * _buf,
					const fpp_t _frames,
					const f_cnt_t _offset,
					stereoVolumeVector _volume_vector,
					AudioPort * _port );

	static void clearAudioBuffer( sampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#ifndef LMMS_DISABLE_SURROUND
	static void clearAudioBuffer( surroundSampleFrame * _ab,
						const f_cnt_t _frames,
						const f_cnt_t _offset = 0 );
#endif

	static float peakValueLeft( sampleFrame * _ab, const f_cnt_t _frames );
	static float peakValueRight( sampleFrame * _ab, const f_cnt_t _frames );


	bool criticalXRuns() const;

	inline bool hasFifoWriter() const
	{
		return m_fifoWriter != NULL;
	}

	void pushInputFrames( sampleFrame * _ab, const f_cnt_t _frames );
	
	inline const sampleFrame * inputBuffer()
	{
		return m_inputBuffer[ m_inputBufferRead ];
	}

	inline f_cnt_t inputBufferFrames() const
	{
		return m_inputBufferFrames[ m_inputBufferRead ];
	}

	inline surroundSampleFrame * nextBuffer()
	{
		return hasFifoWriter() ? m_fifo->read() : renderNextBuffer();
	}

	void changeQuality( const struct qualitySettings & _qs );


signals:
	void qualitySettingsChanged();
	void sampleRateChanged();
	void nextAudioBuffer();


private:
	typedef fifoBuffer<surroundSampleFrame *> fifo;

	class fifoWriter : public QThread
	{
	public:
		fifoWriter( mixer * _mixer, fifo * _fifo );

		void finish();


	private:
		mixer * m_mixer;
		fifo * m_fifo;
		volatile bool m_writing;

		virtual void run();

	} ;


	mixer();
	virtual ~mixer();

	void startProcessing( bool _needs_fifo = true );
	void stopProcessing();


	AudioDevice * tryAudioDevices();
	MidiClient * tryMidiClients();


	surroundSampleFrame * renderNextBuffer();



	QVector<AudioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

	sampleFrame * m_workingBuf;

	sampleFrame * m_inputBuffer[2];
	f_cnt_t m_inputBufferFrames[2];
	f_cnt_t m_inputBufferSize[2];
	int m_inputBufferRead;
	int m_inputBufferWrite;
	
	surroundSampleFrame * m_readBuf;
	surroundSampleFrame * m_writeBuf;
	
	QVector<surroundSampleFrame *> m_bufferPool;
	int m_readBuffer;
	int m_writeBuffer;
	int m_poolDepth;

	surroundSampleFrame m_maxClip;
	surroundSampleFrame m_previousSample;
	fpp_t m_halfStart[SURROUND_CHANNELS];
	bool m_oldBuffer[SURROUND_CHANNELS];
	bool m_newBuffer[SURROUND_CHANNELS];
	
	int m_cpuLoad;
	QVector<MixerWorkerThread *> m_workers;
	int m_numWorkers;
	QWaitCondition m_queueReadyWaitCond;


	PlayHandleList m_playHandles;
	ConstPlayHandleList m_playHandlesToRemove;

	struct qualitySettings m_qualitySettings;
	float m_masterGain;


	AudioDevice * m_audioDev;
	AudioDevice * m_oldAudioDev;
	QString m_audioDevName;


	MidiClient * m_midiClient;
	QString m_midiClientName;


	QMutex m_globalMutex;
	QMutex m_inputFramesMutex;


	fifo * m_fifo;
	fifoWriter * m_fifoWriter;


	friend class engine;
	friend class MixerWorkerThread;

} ;


// TODO: move to MixerWorkerThread.h / MixerWorkerThread.cpp
#include "Cpu.h"
#include "engine.h"

class MixerWorkerThread : public QThread
{
public:
	struct JobQueue
	{
#define JOB_QUEUE_SIZE 1024
		JobQueue() :
			queueSize( 0 ),
			itemsDone( 0 )
		{
			for( int i = 0; i < JOB_QUEUE_SIZE; ++i )
			{
				items[i] = NULL;
			}
		}

		ThreadableJob * items[JOB_QUEUE_SIZE];
		QAtomicInt queueSize;
		QAtomicInt itemsDone;
	} ;

	static JobQueue s_jobQueue;

	MixerWorkerThread( int _worker_num, mixer * _mixer ) :
		QThread( _mixer ),
		m_workingBuf( CPU::allocFrames( _mixer->framesPerPeriod() ) ),
		m_workerNum( _worker_num ),
		m_quit( false ),
		m_mixer( _mixer ),
		m_queueReadyWaitCond( &m_mixer->m_queueReadyWaitCond )
	{
	}

	virtual ~MixerWorkerThread()
	{
		CPU::freeFrames( m_workingBuf );
	}

	virtual void quit()
	{
		m_quit = true;
	}

	void processJobQueue()
	{
		for( int i = 0; i < s_jobQueue.queueSize; ++i )
		{
			// returns true if ThreadableJob was not processed before
			if( s_jobQueue.items[i]->process( m_workingBuf ) )
			{
				s_jobQueue.itemsDone.fetchAndAddOrdered( 1 );
			}
		}
	}

	static void resetJobQueue()
	{
		s_jobQueue.queueSize = 0;
		s_jobQueue.itemsDone = 0;
	}

	template<typename T>
	static void fillJobQueue( const T & _vec )
	{
		resetJobQueue();
		for( typename T::ConstIterator it = _vec.begin(); it != _vec.end(); ++it )
		{
			addJob( *it );
		}
	}

	static void addJob( ThreadableJob * _job )
	{
		if( _job->requiresProcessing() )
		{
			_job->m_state = ThreadableJob::Queued;
			s_jobQueue.items[s_jobQueue.queueSize.fetchAndAddOrdered(1)] = _job;
		}
	}


// define a pause instruction for spinlock-loop - merely useful on
// HyperThreading systems with just one physical core (e.g. Intel Atom)
#ifdef LMMS_HOST_X86
#define SPINLOCK_PAUSE() 	asm( "pause" )
#else
#ifdef LMMS_HOST_X86_64
#define SPINLOCK_PAUSE() 	asm( "pause" )
#else
#define SPINLOCK_PAUSE()
#endif
#endif

	static void startJobs()
	{
		// TODO: this is dirty!
		engine::getMixer()->m_queueReadyWaitCond.wakeAll();
	}

	static void waitForJobs()
	{
		// TODO: this is dirty!
		mixer * m = engine::getMixer();
		m->m_workers[m->m_numWorkers]->processJobQueue();
		while( s_jobQueue.itemsDone < s_jobQueue.queueSize )
		{
			SPINLOCK_PAUSE();
		}
	}

	static void startAndWaitForJobs()
	{
		startJobs();
		waitForJobs();
	}


private:
	virtual void run()
	{
		QMutex m;
		while( m_quit == false )
		{
			m.lock();
			m_queueReadyWaitCond->wait( &m );
			processJobQueue();
			m.unlock();
		}
	}

	sampleFrame * m_workingBuf;
	int m_workerNum;
	volatile bool m_quit;
	mixer * m_mixer;
	QWaitCondition * m_queueReadyWaitCond;

} ;


#endif
