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

#include <memory>
#include <QReadWriteLock>
#include <QObject>

#include <samplerate.h>

#include "lmms_export.h"
#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "shared_object.h"
#include "OscillatorConstants.h"
#include "core/MemoryManager.h"


class QPainter;
class QRect;

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
const f_cnt_t MARGIN[] = { 64, 64, 64, 4, 4 };

class LMMS_EXPORT SampleBuffer : public QObject, public sharedObject
{
	Q_OBJECT
	MM_OPERATORS
public:
	enum LoopMode {
		LoopOff = 0,
		LoopOn,
		LoopPingPong
	};
	class LMMS_EXPORT handleState
	{
		MM_OPERATORS
	public:
		handleState(bool varyingPitch = false, int interpolationMode = SRC_LINEAR);
		virtual ~handleState();

		const f_cnt_t frameIndex() const
		{
			return m_frameIndex;
		}

		void setFrameIndex(f_cnt_t index)
		{
			m_frameIndex = index;
		}

		bool isBackwards() const
		{
			return m_isBackwards;
		}

		void setBackwards(bool backwards)
		{
			m_isBackwards = backwards;
		}

		int interpolationMode() const
		{
			return m_interpolationMode;
		}


	private:
		f_cnt_t m_frameIndex;
		const bool m_varyingPitch;
		bool m_isBackwards;
		SRC_STATE * m_resamplingData;
		int m_interpolationMode;

		friend class SampleBuffer;

	} ;


	SampleBuffer();
	// constructor which either loads sample _audio_file or decodes
	// base64-data out of string
	SampleBuffer(const QString & audioFile, bool isBase64Data = false);
	SampleBuffer(const sampleFrame * data, const f_cnt_t frames);
	explicit SampleBuffer(const f_cnt_t frames);
	SampleBuffer(const SampleBuffer & orig);

	friend void swap(SampleBuffer & first, SampleBuffer & second) noexcept;
	SampleBuffer& operator= (const SampleBuffer that);

	virtual ~SampleBuffer();

	bool play(
		sampleFrame * ab,
		handleState * state,
		const fpp_t frames,
		const float freq,
		const LoopMode loopMode = LoopOff
	);

	void visualize(
		QPainter & p,
		const QRect & dr,
		const QRect & clip,
		f_cnt_t fromFrame = 0,
		f_cnt_t toFrame = 0
	);
	inline void visualize(
		QPainter & p,
		const QRect & dr,
		f_cnt_t fromFrame = 0,
		f_cnt_t toFrame = 0
	)
	{
		visualize(p, dr, dr, fromFrame, toFrame);
	}

	inline const QString & audioFile() const
	{
		return m_audioFile;
	}

	inline f_cnt_t startFrame() const
	{
		return m_startFrame;
	}

	inline f_cnt_t endFrame() const
	{
		return m_endFrame;
	}

	inline f_cnt_t loopStartFrame() const
	{
		return m_loopStartFrame;
	}

	inline f_cnt_t loopEndFrame() const
	{
		return m_loopEndFrame;
	}

	void setLoopStartFrame(f_cnt_t start)
	{
		m_loopStartFrame = start;
	}

	void setLoopEndFrame(f_cnt_t end)
	{
		m_loopEndFrame = end;
	}

	void setAllPointFrames(
		f_cnt_t start,
		f_cnt_t end,
		f_cnt_t loopStart,
		f_cnt_t loopEnd
	)
	{
		m_startFrame = start;
		m_endFrame = end;
		m_loopStartFrame = loopStart;
		m_loopEndFrame = loopEnd;
	}

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
		return double(m_endFrame - m_startFrame) / m_sampleRate * 1000;
	}

	inline void setFrequency(float freq)
	{
		m_frequency = freq;
	}

	inline void setSampleRate(sample_rate_t rate)
	{
		m_sampleRate = rate;
	}

	inline const sampleFrame * data() const
	{
		return m_data;
	}

	QString openAudioFile() const;
	QString openAndSetAudioFile();
	QString openAndSetWaveformFile();

	QString & toBase64(QString & dst) const;


	// protect calls from the GUI to this function with dataReadLock() and
	// dataUnlock()
	SampleBuffer * resample(const sample_rate_t srcSR, const sample_rate_t dstSR);

	void normalizeSampleRate(const sample_rate_t srcSR, bool keepSettings = false);

	// protect calls from the GUI to this function with dataReadLock() and
	// dataUnlock(), out of loops for efficiency
	inline sample_t userWaveSample(const float sample) const
	{
		f_cnt_t frames = m_frames;
		sampleFrame * data = m_data;
		const float frame = sample * frames;
		f_cnt_t f1 = static_cast<f_cnt_t>(frame) % frames;
		if (f1 < 0)
		{
			f1 += frames;
		}
		return linearInterpolate(data[f1][0], data[(f1 + 1) % frames][0], fraction(frame));
	}

	void dataReadLock()
	{
		m_varLock.lockForRead();
	}

	void dataUnlock()
	{
		m_varLock.unlock();
	}


	std::unique_ptr<OscillatorConstants::waveform_t> m_userAntiAliasWaveTable;


public slots:
	void setAudioFile(const QString & audioFile);
	void loadFromBase64(const QString & data);
	void setStartFrame(const f_cnt_t s);
	void setEndFrame(const f_cnt_t e);
	void setAmplification(float a);
	void setReversed(bool on);
	void sampleRateChanged();

private:
	static sample_rate_t audioEngineSampleRate();

	void update(bool keepSettings = false);

	void convertIntToFloat(int_sample_t * & ibuf, f_cnt_t frames, int channels);
	void directFloatWrite(sample_t * & fbuf, f_cnt_t frames, int channels);

	f_cnt_t decodeSampleSF(
		QString fileName,
		sample_t * & buf,
		ch_cnt_t & channels,
		sample_rate_t & samplerate
	);
#ifdef LMMS_HAVE_OGGVORBIS
	f_cnt_t decodeSampleOGGVorbis(
		QString fileName,
		int_sample_t * & buf,
		ch_cnt_t & channels,
		sample_rate_t & samplerate
	);
#endif
	f_cnt_t decodeSampleDS(
		QString fileName,
		int_sample_t * & buf,
		ch_cnt_t & channels,
		sample_rate_t & samplerate
	);

	QString m_audioFile;
	sampleFrame * m_origData;
	f_cnt_t m_origFrames;
	sampleFrame * m_data;
	mutable QReadWriteLock m_varLock;
	f_cnt_t m_frames;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loopStartFrame;
	f_cnt_t m_loopEndFrame;
	float m_amplification;
	bool m_reversed;
	float m_frequency;
	sample_rate_t m_sampleRate;

	sampleFrame * getSampleFragment(
		f_cnt_t index,
		f_cnt_t frames,
		LoopMode loopMode,
		sampleFrame * * tmp,
		bool * backwards,
		f_cnt_t loopStart,
		f_cnt_t loopEnd,
		f_cnt_t end
	) const;

	f_cnt_t getLoopedIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;
	f_cnt_t getPingPongIndex(f_cnt_t index, f_cnt_t startf, f_cnt_t endf) const;


signals:
	void sampleUpdated();

} ;


#endif
