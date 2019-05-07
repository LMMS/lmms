/*
 * SampleBuffer.h - container-class SampleBuffer
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef SAMPLE_BUFFER_H
#define SAMPLE_BUFFER_H

#include "internal/SampleBufferData.h"
#include "internal/SampleBufferPlayInfo.h"
#include "JournallingObject.h"
#include "Threading.h"
#include "UpdatingValue.h"

class QPainter;

class QRect;

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
const f_cnt_t MARGIN[] = {64, 64, 64, 4, 4};

class LMMS_EXPORT SampleBuffer : public QObject, public JournallingObject {
Q_OBJECT
MM_OPERATORS

	enum UpdateType : int {
		Clear,
		Append
	};

	typedef std::shared_ptr<internal::SampleBufferData> SharedData;
	typedef std::shared_ptr<internal::SampleBufferPlayInfo> SharedPlayInfo;

	template<class VarType, class PtrType=std::shared_ptr<VarType>>
	class _MixerGuardedSharedPtr {
	public:
		explicit _MixerGuardedSharedPtr(PtrType ptr)
				: m_ptr(std::move(ptr)) {
		}

		VarType *operator->() {
			return m_ptr.get();
		}

		VarType &operator*() {
			return (*m_ptr);
		}


	private:
		PtrType m_ptr;
		Mixer::RequestChangesGuard m_guard = Engine::mixer()->requestChangesGuard();
	};

	using GuardedPlayInfo=_MixerGuardedSharedPtr<internal::SampleBufferPlayInfo>;
	using GuardedConstPlayInfo=_MixerGuardedSharedPtr<const internal::SampleBufferPlayInfo>;

	GuardedPlayInfo guardedPlayInfo() {
		return GuardedPlayInfo(m_playInfo);
	}

	GuardedConstPlayInfo guardedPlayInfo() const {
		return GuardedConstPlayInfo(m_playInfo);
	}


	using GuardedData=_MixerGuardedSharedPtr<internal::SampleBufferData>;
	using GuardedConstData=_MixerGuardedSharedPtr<const internal::SampleBufferData>;

	GuardedData guardedData() {
		return GuardedData(m_data);
	}

	GuardedConstData guardedData() const {
		return GuardedConstData(m_data);
	}

	struct SampleBufferInfo {
		SampleBufferInfo() = default;

		SampleBufferInfo(GuardedPlayInfo &playInfo, GuardedData &data)
				: startFrame{playInfo->getStartFrame()},
				  endFrame{playInfo->getEndFrame()},
				  loopStartFrame{playInfo->getLoopStartFrame()},
				  loopEndFrame{playInfo->getLoopEndFrame()},

				  amplification{data->getAmplification()},
				  audioFile(playInfo->getMaybeAudioFile()),
				  frames{data->frames()},
				  frequency{data->getFrequency()},
				  sampleRate{data->getSampleRate()} {
		}

		int sampleLength() const {
			double playFrames = double(endFrame - startFrame);

			return playFrames / sampleRate * 1000;
		}

		f_cnt_t startFrame;
		f_cnt_t endFrame;
		f_cnt_t loopStartFrame;
		f_cnt_t loopEndFrame;

		float amplification;
		QString audioFile;
		f_cnt_t frames;
		float frequency;
		sample_rate_t sampleRate;
	};

public:
	typedef UpdatingValue<SampleBufferInfo> InfoUpdatingValue;

	SampleBufferInfo createInfo() {
		auto playInfo = guardedPlayInfo();
		auto data = guardedData();

		return SampleBufferInfo(playInfo, data);
	}

	InfoUpdatingValue createUpdatingValue(QObject *parent) {
		return InfoUpdatingValue{*m_infoChangeNotifier, createInfo(), parent};
	}

	using DataVector=internal::SampleBufferData::DataVector;
	using LoopMode=::LoopMode;

	class LMMS_EXPORT handleState {
	MM_OPERATORS

	public:
		handleState(bool _varying_pitch = false, int interpolation_mode = SRC_LINEAR);

		virtual ~handleState();

		f_cnt_t frameIndex() const {
			return m_frameIndex;
		}

		void setFrameIndex(f_cnt_t _index) {
			m_frameIndex = _index;
		}

		bool isBackwards() const {
			return m_isBackwards;
		}

		void setBackwards(bool _backwards) {
			m_isBackwards = _backwards;
		}

		int interpolationMode() const {
			return m_interpolationMode;
		}


		f_cnt_t m_frameIndex;
		const bool m_varyingPitch;
		bool m_isBackwards;
		SRC_STATE *m_resamplingData;
		int m_interpolationMode;
	};


	explicit SampleBuffer();

	explicit SampleBuffer(internal::SampleBufferData &&data);

	/**
	 * Load this sample buffer from @a _audio_file,
	 * @param _audio_file	path to an audio file.
	 */
	explicit SampleBuffer(const QString &_audio_file, bool ignoreError=false);

	/**
	 * Load the sample buffer from a base64 encoded string.
	 * @param base64Data	The data.
	 * @param sample_rate	The data's sample rate.
	 */
	SampleBuffer(const QString &base64Data, sample_rate_t sample_rate);

	SampleBuffer(DataVector &&movedData, sample_rate_t sampleRate);

	inline virtual QString nodeName() const override {
		return "samplebuffer";
	}

	SampleBuffer(SampleBuffer &&sampleBuffer) noexcept;

	SampleBuffer &operator=(SampleBuffer &&other) noexcept;

	virtual void saveSettings(QDomDocument &doc, QDomElement &_this) override;

	virtual void loadSettings(const QDomElement &_this) override;

	bool play(sampleFrame *_ab, handleState *_state,
			  const fpp_t _frames,
			  const float _freq,
			  const LoopMode _loopmode = LoopMode::LoopOff);

	template<class LaunchType=LaunchType::Async>
	void setLoopStartFrame(f_cnt_t _start) {
		runToSetPlayInfo<LaunchType>([=](GuardedPlayInfo &playInfo) {
			playInfo->setLoopEndFrame(_start);
		});
	}

	template<class LaunchType=LaunchType::Async>
	void setLoopEndFrame(f_cnt_t _end) {
		runToSetPlayInfo<LaunchType>([=](GuardedPlayInfo &playInfo) {
			playInfo->setLoopEndFrame(_end);
		});
	}


	template<class LaunchType=LaunchType::Async>
	void setAllPointFrames(f_cnt_t _start, f_cnt_t _end, f_cnt_t _loopstart, f_cnt_t _loopend) {
		runToSetPlayInfo<LaunchType>([=](GuardedPlayInfo &playInfo) {
			playInfo->setStartFrame(_start);
			playInfo->setEndFrame(_end);
			playInfo->setLoopStartFrame(_loopstart);
			playInfo->setLoopEndFrame(_loopend);
		});
	}

	template<class LaunchType=LaunchType::Async>
	void setStartFrame(const f_cnt_t _s) {
		runToSetPlayInfo<LaunchType>([=](GuardedPlayInfo &playInfo) {
			playInfo->setStartFrame(_s);
		});
	}

	template<class LaunchType=LaunchType::Async>
	void setEndFrame(const f_cnt_t _e) {
		runToSetPlayInfo<LaunchType>([=](GuardedPlayInfo &playInfo) {
			playInfo->setEndFrame(_e);
		});
	}

	template<class LaunchType=LaunchType::Async>
	void setAmplification(float _a) {
		runToSetData<LaunchType>([_a](GuardedData &data) {
			data->setAmplification(_a);
		});
	}

	template<class LaunchType=LaunchType::Async>
	void setFrequency(float frequency) {
		runToSetData<LaunchType>([frequency](GuardedData &data) {
			data->setFrequency(frequency);
		});
	}

	void setAudioFile(const QString &audioFile, bool ignoreError = false);

	static QString openAudioFile(const QString &currentAudioFile = QString());

	QString openAndSetAudioFile(const QString &currentAudioFile = QString());

	QString openAndSetWaveformFile(QString currentAudioFile = QString());

	sample_t userWaveSample(const float _sample) const;


	static QString tryToMakeRelative(const QString &_file);

	static QString tryToMakeAbsolute(const QString &file);

	/**
	 * @brief Add data to the buffer,
	 * @param begin	Beginning of an InputIterator.
	 * @param end	End of an InputIterator.
	 * @return
	 */
	void addData(const DataVector &vector, sample_rate_t sampleRate);

	/**
	 * @brief Reset the class and initialize it with @a newData.
	 * @param newData	mm, that's the new data.
	 * @param dataSampleRate	Sample rate for @a newData.
	 */
	void resetData(DataVector &&newData, sample_rate_t dataSampleRate);

	/**
	 * @brief Just reverse the current buffer.
	 *
	 * This function simply calls `std::reverse` on m_data.
	 */
	void reverse();

	void visualize(QPainter &_p, const QRect &_dr, const QRect &_clip, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0);

	inline void visualize(QPainter &_p, const QRect &_dr, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0) {
		visualize(_p, _dr, _dr, _from_frame, _to_frame);
	}

	std::pair<QPolygonF, QPolygonF>
	visualizeToPoly(const QRect &_dr, const QRect &_clip, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0) const;


private:

	QString &toBase64(QString &_dst) const;

	// HACK: libsamplerate < 0.1.8 doesn't get read-only variables
	//	     as const. It has been fixed in 0.1.9 but has not been
	//		 shipped for some distributions.
	//		 This function just returns a variable that should have
	//		 been `const` as non-const to bypass using 0.1.9.
	inline static sampleFrame *libSampleRateSrc(const sampleFrame *ptr) {
		return const_cast<sampleFrame *>(ptr);
	}

	static f_cnt_t getLoopedIndex(f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf);

	static f_cnt_t getPingPongIndex(f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf);

	/**
	 * @brief A helper class used when changes to m_data is needed
	 */
	class DataChangeHelper {
	public:
		explicit DataChangeHelper(SampleBuffer *buffer, UpdateType updateType);

		~DataChangeHelper();

	private:
		SampleBuffer *m_buffer;
		UpdateType m_updateType;
		Mixer::RequestChangesGuard m_mixerGuard;
	};

	/**
	 * A little helper that will run a callable
	 * with async, and giving it a guarded play
	 * info as an argument and notifies about the
	 * changes in the end.
	 */
	template<class LaunchType, class Func>
	void runToSetValue(Func &&func) {
		runAccordingToLaunchType([func, this] {
			func();
			m_infoChangeNotifier->onValueUpdated(createInfo());
		}, LaunchType{});
	}

	/**
	 * Run a function and give it a GuardedPlayInfo as a parameter.
	 * @param func The function to run.
	 */
	template<class LaunchType, class Func>
	void runToSetPlayInfo(Func &&func) {
		runToSetValue<LaunchType>([func, this] {
			auto playInfo = guardedPlayInfo();
			func(playInfo);
		});
	}

	/**
	 * Run a function and give it a GuardedData as a parameter.
	 * @param func The function to run.
	 */
	template<class LaunchType, class Func>
	void runToSetData(Func &&func) {
		runToSetValue<LaunchType>([func, this] {
			auto data = guardedData();
			func(data);
		});
	}

signals:

	void sampleUpdated(int updateType);

private:
	SharedData m_data;
	SharedPlayInfo m_playInfo;

	UniqueQObjectPointer<InfoUpdatingValue::Notifier> m_infoChangeNotifier =
			makeUniqueQObject<InfoUpdatingValue::Notifier>();
};


#endif
