/*
 * AudioEngine.h - device-independent audio engine for LMMS
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <QMutex>

#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	#include <QRecursiveMutex>
#endif

#include <QThread>
#include <QVector>
#include <QWaitCondition>
#include <samplerate.h>


#include "lmms_basics.h"
#include "LocklessList.h"
#include "FifoBuffer.h"
#include "AudioEngineProfiler.h"
#include "PlayHandle.h"


namespace lmms
{

class AudioDevice;
class MidiClient;
class AudioPort;
class AudioEngineWorkerThread;


const fpp_t MINIMUM_BUFFER_SIZE = 32;
const fpp_t DEFAULT_BUFFER_SIZE = 256;

const int BYTES_PER_SAMPLE = sizeof( sample_t );
const int BYTES_PER_INT_SAMPLE = sizeof( int_sample_t );
const int BYTES_PER_FRAME = sizeof( sampleFrame );
const int BYTES_PER_SURROUND_FRAME = sizeof( surroundSampleFrame );

const float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;

class LMMS_EXPORT AudioEngine : public QObject
{
	Q_OBJECT
public:
	/**
	 * @brief RAII helper for requestChangesInModel.
	 * Used by AudioEngine::requestChangesGuard.
	 */
	class RequestChangesGuard {
		friend class AudioEngine;

	private:
		RequestChangesGuard(AudioEngine* audioEngine)
			: m_audioEngine{audioEngine}
		{
			m_audioEngine->requestChangeInModel();
		}
	public:

		RequestChangesGuard()
			: m_audioEngine{nullptr}
		{
		}

		RequestChangesGuard(RequestChangesGuard&& other)
			: RequestChangesGuard()
		{
			std::swap(other.m_audioEngine, m_audioEngine);
		}

		// Disallow copy.
		RequestChangesGuard(const RequestChangesGuard&) = delete;
		RequestChangesGuard& operator=(const RequestChangesGuard&) = delete;

		~RequestChangesGuard() {
			if (m_audioEngine) {
				m_audioEngine->doneChangeInModel();
			}
		}

	private:
		AudioEngine* m_audioEngine;
	};

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

		qualitySettings(Mode m)
		{
			switch (m)
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

		qualitySettings(Interpolation i, Oversampling o) :
			interpolation(i),
			oversampling(o)
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
	void clearNewPlayHandles();


	// audio-device-stuff

	// Returns the current audio device's name. This is not necessarily
	// the user's preferred audio device, in case you were thinking that.
	inline const QString & audioDevName() const
	{
		return m_audioDevName;
	}
	inline bool audioDevStartFailed() const
	{
		return m_audioDevStartFailed;
	}

	//! Set new audio device. Old device will be deleted,
	//! unless it's stored using storeAudioDevice
	void setAudioDevice( AudioDevice * _dev,
				const struct qualitySettings & _qs,
				bool _needs_fifo,
				bool startNow );
	void storeAudioDevice();
	void restoreAudioDevice();
	inline AudioDevice * audioDev()
	{
		return m_audioDev;
	}


	// audio-port-stuff
	inline void addAudioPort(AudioPort * port)
	{
		requestChangeInModel();
		m_audioPorts.push_back(port);
		doneChangeInModel();
	}

	void removeAudioPort(AudioPort * port);


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

	void removePlayHandlesOfTypes(Track * track, const quint8 types);


	// methods providing information for other classes
	inline fpp_t framesPerPeriod() const
	{
		return m_framesPerPeriod;
	}


	AudioEngineProfiler& profiler()
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

	inline void setMasterGain(const float mo)
	{
		m_masterGain = mo;
	}


	static inline sample_t clip(const sample_t s)
	{
		if (s > 1.0f)
		{
			return 1.0f;
		}
		else if (s < -1.0f)
		{
			return -1.0f;
		}
		return s;
	}


	struct StereoSample
	{
		StereoSample(sample_t _left, sample_t _right) : left(_left), right(_right) {}
		sample_t left;
		sample_t right;
	};
	StereoSample getPeakValues(sampleFrame * ab, const f_cnt_t _frames) const;


	bool criticalXRuns() const;

	inline bool hasFifoWriter() const
	{
		return m_fifoWriter != nullptr;
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

	void changeQuality(const struct qualitySettings & qs);

	inline bool isMetronomeActive() const { return m_metronomeActive; }
	inline void setMetronomeActive(bool value = true) { m_metronomeActive = value; }

	//! Block until a change in model can be done (i.e. wait for audio thread)
	void requestChangeInModel();
	void doneChangeInModel();

	RequestChangesGuard requestChangesGuard()
	{
		return RequestChangesGuard{this};
	}

	static bool isAudioDevNameValid(QString name);
	static bool isMidiDevNameValid(QString name);


signals:
	void qualitySettingsChanged();
	void sampleRateChanged();
	void nextAudioBuffer( const lmms::surroundSampleFrame * buffer );


private:
	typedef FifoBuffer<surroundSampleFrame *> Fifo;

	class fifoWriter : public QThread
	{
	public:
		fifoWriter( AudioEngine * audioEngine, Fifo * fifo );

		void finish();


	private:
		AudioEngine * m_audioEngine;
		Fifo * m_fifo;
		volatile bool m_writing;

		void run() override;

		void write( surroundSampleFrame * buffer );
	} ;


	AudioEngine( bool renderOnly );
	~AudioEngine() override;

	void startProcessing(bool needsFifo = true);
	void stopProcessing();


	AudioDevice * tryAudioDevices();
	MidiClient * tryMidiClients();


	const surroundSampleFrame * renderNextBuffer();

	void swapBuffers();

	void handleMetronome();

	void clearInternal();

	//! Called by the audio thread to give control to other threads,
	//! such that they can do changes in the model (like e.g. removing effects)
	void runChangesInModel();

	bool m_renderOnly;

	QVector<AudioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

	sampleFrame * m_inputBuffer[2];
	f_cnt_t m_inputBufferFrames[2];
	f_cnt_t m_inputBufferSize[2];
	int m_inputBufferRead;
	int m_inputBufferWrite;

	surroundSampleFrame * m_outputBufferRead;
	surroundSampleFrame * m_outputBufferWrite;

	// worker thread stuff
	QVector<AudioEngineWorkerThread *> m_workers;
	int m_numWorkers;

	// playhandle stuff
	PlayHandleList m_playHandles;
	// place where new playhandles are added temporarily
	LocklessList<PlayHandle *> m_newPlayHandles;
	ConstPlayHandleList m_playHandlesToRemove;


	struct qualitySettings m_qualitySettings;
	float m_masterGain;

	bool m_isProcessing;

	// audio device stuff
	void doSetAudioDevice( AudioDevice *_dev );
	AudioDevice * m_audioDev;
	AudioDevice * m_oldAudioDev;
	QString m_audioDevName;
	bool m_audioDevStartFailed;

	// MIDI device stuff
	MidiClient * m_midiClient;
	QString m_midiClientName;

	// FIFO stuff
	Fifo * m_fifo;
	fifoWriter * m_fifoWriter;

	AudioEngineProfiler m_profiler;

	bool m_metronomeActive;

	bool m_clearSignal;

	bool m_changesSignal;
	unsigned int m_changes;
	QMutex m_changesMutex;
#if (QT_VERSION >= QT_VERSION_CHECK(5,14,0))
	QRecursiveMutex m_doChangesMutex;
#else
	QMutex m_doChangesMutex;
#endif
	QMutex m_waitChangesMutex;
	QWaitCondition m_changesAudioEngineCondition;
	QWaitCondition m_changesRequestCondition;

	bool m_waitingForWrite;

	friend class Engine;
	friend class AudioEngineWorkerThread;
	friend class ProjectRenderer;
} ;

} // namespace lmms

#endif
