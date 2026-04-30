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

#ifndef LMMS_AUDIO_ENGINE_H
#define LMMS_AUDIO_ENGINE_H

#include <mutex>

#include <QThread>
#include <samplerate.h>

#include <memory>
#include <vector>

#include "AudioBufferView.h"
#include "AudioDevice.h"
#include "LmmsTypes.h"
#include "SampleFrame.h"
#include "LocklessList.h"
#include "AudioEngineProfiler.h"
#include "PlayHandle.h"


namespace lmms
{

class MidiClient;
class AudioBusHandle;  // IWYU pragma: keep
class AudioEngineWorkerThread;

constexpr f_cnt_t MINIMUM_BUFFER_SIZE = 32;
constexpr f_cnt_t DEFAULT_BUFFER_SIZE = 256;
constexpr f_cnt_t MAXIMUM_BUFFER_SIZE = 4096;

constexpr int BYTES_PER_SAMPLE = sizeof(sample_t);
constexpr int BYTES_PER_INT_SAMPLE = sizeof(int_sample_t);
constexpr int BYTES_PER_FRAME = sizeof(SampleFrame);

constexpr float OUTPUT_SAMPLE_MULTIPLIER = 32767.0f;

constexpr auto SUPPORTED_SAMPLERATES = std::array{44100, 48000, 88200, 96000, 192000};
constexpr auto SUPPORTED_BITRATES = std::array{64, 128, 160, 192, 256, 320};

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

	void initDevices();
	void clear();
	void clearNewPlayHandles();


	// audio-device-stuff

	bool renderOnly() const { return m_renderOnly; }
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
	void setAudioDevice(AudioDevice* _dev, bool startNow);
	void storeAudioDevice();
	void restoreAudioDevice();
	inline AudioDevice * audioDev()
	{
		return m_audioDev;
	}


	// audio-bus-handle-stuff
	inline void addAudioBusHandle(AudioBusHandle* busHandle)
	{
		requestChangeInModel();
		m_audioBusHandles.push_back(busHandle);
		doneChangeInModel();
	}

	void removeAudioBusHandle(AudioBusHandle* busHandle);


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

	void removePlayHandlesOfTypes(Track * track, PlayHandle::Types types);

	AudioEngineProfiler& profiler()
	{
		return m_profiler;
	}

	int cpuLoad() const
	{
		return m_profiler.cpuLoad();
	}

	int detailLoad(const AudioEngineProfiler::DetailType type) const
	{
		return m_profiler.detailLoad(type);
	}

	sample_rate_t baseSampleRate() const { return m_baseSampleRate; }


	sample_rate_t outputSampleRate() const
	{
		return m_audioDev != nullptr ? m_audioDev->sampleRate() : m_baseSampleRate;
	}
	

	sample_rate_t inputSampleRate() const	
	{
		return m_audioDev != nullptr ? m_audioDev->sampleRate() : m_baseSampleRate;
	}


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


	bool criticalXRuns() const;

	void pushInputFrames( SampleFrame* _ab, const f_cnt_t _frames );

	inline const SampleFrame* inputBuffer()
	{
		return m_inputBuffer[ m_inputBufferRead ];
	}

	inline f_cnt_t inputBufferFrames() const
	{
		return m_inputBufferFrames[ m_inputBufferRead ];
	}

	/**
	 * @returns The internal buffer size used by audio plugins and other processing done within the audio engine.
	 * Its value is @ref DEFAULT_BUFFER_SIZE or @ref framesPerAudioBuffer(), whichever is lower.
	 * @see renderNextPeriod()
	 */
	f_cnt_t framesPerPeriod() const { return m_framesPerPeriod; }

	/**
	 * @returns The buffer size used by the configured @ref AudioDevice "output audio device". It is in the range
	 * of @ref MINIMUM_BUFFER_SIZE and @ref MAXIMUM_BUFFER_SIZE.
	 * @note This should not be needed for a majority of cases. Currently, it's only being used to set up the
	 * audio devices. This member function may be removed in a later refactor.
	 */
	f_cnt_t framesPerAudioBuffer() const { return m_framesPerAudioBuffer; }

	/**
	 * @brief Renders the next audio period.
	 *
	 * An audio period is a fixed-size chunk of audio the engine generates, and represents a single cycle of the
	 * engine's output. The rendering is chunked into smaller periods to timely handle per-buffer updates like
	 * non-sample-accurate automation, as well as to improve memory cache performance.
	 *
	 * The audio period generated is interleaved and stereo.
	 *
	 * @note The audio period returned is non-owning and will be changed on subsequent calls to @ref renderNextPeriod()
	 * and @ref renderNextBuffer(). Callers must copy the data into their own local buffers if they need it to persist.
	 *
	 * @returns A non-owning buffer to the next audio period.
	 */
	std::span<const SampleFrame> renderNextPeriod();

	/**
	 * @brief Renders an audio buffer into @a dst.
	 *
	 * Renders @ref renderNextAudioPeriod() "audio periods" into @a dst. If @a dst is not a multiple of the period
	 * size, the remaining frames are partially rendered (an extra period may be rendered in such cases, which can
	 * degrade performance).
	 *
	 * If @a dst has 1 channel, the channels are averaged to mono.
	 * If @a dst has 2 channels, the channels are directly copied.
	 * If @a dst has more than 2 channels, the stereo channels are copied and the rest are zero-filled.
	 *
	 * @param dst An audio buffer view to write into. Both interleaved and planar overloads are provided.
	 */
	void renderNextBuffer(InterleavedBufferView<float> dst) { renderNextBuffer<InterleavedBufferView<float>>(dst); }

	//! @copydoc renderNextBuffer(InterleavedBufferView<float>)
	void renderNextBuffer(PlanarBufferView<float> dst) { renderNextBuffer<PlanarBufferView<float>>(dst); }

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
	void nextAudioBuffer(const lmms::SampleFrame* buffer);


private:
	void renderNextBuffer(AudioBufferView<float> auto dst)
	{
		for (auto frame = f_cnt_t{0}; frame < dst.frames(); ++frame)
		{
			if (m_outputBufferReadIndex == m_framesPerPeriod) { m_outputBufferReadIndex = 0; }
			if (m_outputBufferReadIndex == 0) { renderNextPeriod(); }

			switch (dst.channels())
			{
			case 0:
				assert(false);
				break;
			case 1:
				dst.sample(0, frame) = m_outputBufferRead[m_outputBufferReadIndex].average();
				break;
			case 2:
				dst.sample(0, frame) = m_outputBufferRead[m_outputBufferReadIndex][0];
				dst.sample(1, frame) = m_outputBufferRead[m_outputBufferReadIndex][1];
				break;
			default:
				dst.sample(0, frame) = m_outputBufferRead[m_outputBufferReadIndex][0];
				dst.sample(1, frame) = m_outputBufferRead[m_outputBufferReadIndex][1];
				for (auto channel = 2; channel < dst.channels(); ++channel)
				{
					dst.sample(channel, frame) = 0.f;
				}
				break;
			}

			++m_outputBufferReadIndex;
		}
	}

	AudioEngine( bool renderOnly );
	~AudioEngine() override;

	void startProcessing() { m_audioDev->startProcessing(); }
	void stopProcessing() { m_audioDev->stopProcessing(); }


	AudioDevice * tryAudioDevices();
	MidiClient * tryMidiClients();

	void renderStageNoteSetup();
	void renderStageInstruments();
	void renderStageEffects();
	void renderStageMix();


	void swapBuffers();

	void clearInternal();

	bool m_renderOnly;

	std::vector<AudioBusHandle*> m_audioBusHandles;

	f_cnt_t m_framesPerAudioBuffer;
	f_cnt_t m_framesPerPeriod;
	sample_rate_t m_baseSampleRate;

	SampleFrame* m_inputBuffer[2];
	f_cnt_t m_inputBufferFrames[2];
	f_cnt_t m_inputBufferSize[2];
	int m_inputBufferRead;
	int m_inputBufferWrite;

	std::unique_ptr<SampleFrame[]> m_outputBufferRead;
	std::unique_ptr<SampleFrame[]> m_outputBufferWrite;
	f_cnt_t m_outputBufferReadIndex;

	// worker thread stuff
	std::vector<AudioEngineWorkerThread *> m_workers;
	int m_numWorkers;

	// playhandle stuff
	PlayHandleList m_playHandles;
	// place where new playhandles are added temporarily
	LocklessList<PlayHandle *> m_newPlayHandles;
	ConstPlayHandleList m_playHandlesToRemove;

	float m_masterGain;

	// audio device stuff
	void doSetAudioDevice( AudioDevice *_dev );
	AudioDevice * m_audioDev;
	AudioDevice * m_oldAudioDev;
	QString m_audioDevName;
	bool m_audioDevStartFailed;

	// MIDI device stuff
	MidiClient * m_midiClient;
	QString m_midiClientName;

	AudioEngineProfiler m_profiler;

	bool m_clearSignal;

	std::recursive_mutex m_changeMutex;

	friend class Engine;
	friend class AudioEngineWorkerThread;
	friend class ProjectRenderer;
} ;

} // namespace lmms

#endif // LMMS_AUDIO_ENGINE_H
