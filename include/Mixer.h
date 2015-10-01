/*
 * Mixer.h - audio-device-independent mixer for LMMS
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef MIXER_H
#define MIXER_H

#include "denormals.h"

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
#include "Note.h"
#include "fifo_buffer.h"
#include "MixerProfiler.h"


class AudioDevice;
class MidiClient;
class AudioPort;


const fpp_t MINIMUM_BUFFER_SIZE = 32;
const fpp_t DEFAULT_BUFFER_SIZE = 256;

const int BYTES_PER_SAMPLE = sizeof( sample_t );
const int BYTES_PER_INT_SAMPLE = sizeof( int_sample_t );
const int BYTES_PER_FRAME = sizeof( sampleFrame );
const int BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;


const float BaseFreq = 440.0f;
const Keys BaseKey = Key_A;
const Octaves BaseOctave = DefaultOctave;


#include "PlayHandle.h"


class MixerWorkerThread;


class EXPORT Mixer : public QObject
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

		qualitySettings( Mode _m )
		{
			switch( _m )
			{
				case Mode_Draft:
					interpolation = Interpolation_Linear;
					oversampling = Oversampling_None;
					break;
				case Mode_HighQuality:
					interpolation =
						Interpolation_SincFastest;
					oversampling = Oversampling_2x;
					break;
				case Mode_FinalMix:
					interpolation = Interpolation_SincBest;
					oversampling = Oversampling_8x;
					break;
			}
		}

		qualitySettings( Interpolation _i, Oversampling _o ) :
			interpolation( _i ),
			oversampling( _o )
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
	bool addPlayHandle( PlayHandle* handle );

	void removePlayHandle( PlayHandle* handle );

	inline PlayHandleList& playHandles()
	{
		return m_playHandles;
	}

	void removePlayHandles( Track * _track, bool removeIPHs = true );

	bool hasNotePlayHandles();


	// methods providing information for other classes
	inline fpp_t framesPerPeriod() const
	{
		return m_framesPerPeriod;
	}


	MixerProfiler& profiler()
	{
		return m_profiler;
	}

	int cpuLoad() const
	{
		return m_profiler.cpuLoad();
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

	void lockPlayHandleRemoval()
	{
		m_playHandleRemovalMutex.lock();
	}

	void unlockPlayHandleRemoval()
	{
		m_playHandleRemovalMutex.unlock();
	}

	// audio-buffer-mgm
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

	inline const surroundSampleFrame * nextBuffer()
	{
		return hasFifoWriter() ? m_fifo->read() : renderNextBuffer();
	}

	void changeQuality( const struct qualitySettings & _qs );

	inline bool isMetronomeActive() const { return m_metronomeActive; }
	inline void setMetronomeActive(bool value = true) { m_metronomeActive = value; }


signals:
	void qualitySettingsChanged();
	void sampleRateChanged();
	void nextAudioBuffer( const surroundSampleFrame * buffer );


private:
	typedef fifoBuffer<surroundSampleFrame *> fifo;

	class fifoWriter : public QThread
	{
	public:
		fifoWriter( Mixer * _mixer, fifo * _fifo );

		void finish();


	private:
		Mixer * m_mixer;
		fifo * m_fifo;
		volatile bool m_writing;

		virtual void run();

	} ;


	Mixer( bool renderOnly );
	virtual ~Mixer();

	void startProcessing( bool _needs_fifo = true );
	void stopProcessing();


	AudioDevice * tryAudioDevices();
	MidiClient * tryMidiClients();


	const surroundSampleFrame * renderNextBuffer();



	QVector<AudioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

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

	// worker thread stuff
	QVector<MixerWorkerThread *> m_workers;
	int m_numWorkers;

	// playhandle stuff
	PlayHandleList m_playHandles;
	PlayHandleList m_newPlayHandles;	// place where new playhandles are added temporarily
	ConstPlayHandleList m_playHandlesToRemove;


	struct qualitySettings m_qualitySettings;
	float m_masterGain;

	// audio device stuff
	AudioDevice * m_audioDev;
	AudioDevice * m_oldAudioDev;
	QString m_audioDevName;

	// MIDI device stuff
	MidiClient * m_midiClient;
	QString m_midiClientName;

	// mutexes
	QMutex m_globalMutex;
	QMutex m_inputFramesMutex;
	QMutex m_playHandleMutex;			// mutex used only for adding playhandles
	QMutex m_playHandleRemovalMutex;

	// FIFO stuff
	fifo * m_fifo;
	fifoWriter * m_fifoWriter;

	MixerProfiler m_profiler;

	bool m_metronomeActive;

	friend class Engine;
	friend class MixerWorkerThread;

} ;


#endif
