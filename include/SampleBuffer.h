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

#include <QtCore/QReadWriteLock>
#include <QtCore/QObject>

#include <samplerate.h>

#include "lmms_export.h"
#include <vector>

#include "interpolation.h"
#include "lmms_basics.h"
#include "lmms_math.h"
#include "shared_object.h"
#include "MemoryManager.h"
#include "JournallingObject.h"


class QPainter;
class QRect;

// values for buffer margins, used for various libsamplerate interpolation modes
// the array positions correspond to the converter_type parameter values in libsamplerate
// if there appears problems with playback on some interpolation mode, then the value for that mode
// may need to be higher - conversely, to optimize, some may work with lower values
const f_cnt_t MARGIN[] = { 64, 64, 64, 4, 4 };

class LMMS_EXPORT SampleBuffer : public QObject, public JournallingObject
{
	Q_OBJECT
	MM_OPERATORS
public:
	typedef std::vector<sampleFrame, MmAllocator<sampleFrame>> DataVector;

	enum LoopMode {
		LoopOff = 0,
		LoopOn,
		LoopPingPong
	};
	class LMMS_EXPORT handleState
	{
		MM_OPERATORS
	public:
		handleState( bool _varying_pitch = false, int interpolation_mode = SRC_LINEAR );
		virtual ~handleState();

		const f_cnt_t frameIndex() const
		{
			return m_frameIndex;
		}

		void setFrameIndex( f_cnt_t _index )
		{
			m_frameIndex = _index;
		}

		bool isBackwards() const
		{
			return m_isBackwards;
		}

		void setBackwards( bool _backwards )
		{
			m_isBackwards = _backwards;
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
	SampleBuffer(const QString & _audio_file, bool _is_base64_data, sample_rate_t sampleRate=0);
	SampleBuffer(DataVector &&movedData, sample_rate_t sampleRate);

	inline virtual QString nodeName() const override
	{
		return "samplebuffer";
	}
	virtual void saveSettings(QDomDocument& doc, QDomElement& _this ) override;
	virtual void loadSettings(const QDomElement& _this ) override;

	bool play( sampleFrame * _ab, handleState * _state,
				const fpp_t _frames,
				const float _freq,
				const LoopMode _loopmode = LoopOff );

	void visualize( QPainter & _p, const QRect & _dr, const QRect & _clip, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0 );
	inline void visualize( QPainter & _p, const QRect & _dr, f_cnt_t _from_frame = 0, f_cnt_t _to_frame = 0 )
	{
		visualize( _p, _dr, _dr, _from_frame, _to_frame );
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

	void setLoopStartFrame( f_cnt_t _start )
	{
		m_loopStartFrame = _start;
	}

	void setLoopEndFrame( f_cnt_t _end )
	{
		m_loopEndFrame = _end;
	}

	void setAllPointFrames( f_cnt_t _start, f_cnt_t _end, f_cnt_t _loopstart, f_cnt_t _loopend )
	{
		m_startFrame = _start;
		m_endFrame = _end;
		m_loopStartFrame = _loopstart;
		m_loopEndFrame = _loopend;
	}

	inline f_cnt_t frames() const
	{
		return m_data.size ();
	}

	inline float amplification() const
	{
		return m_amplification;
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
		return double( m_endFrame - m_startFrame ) / m_sampleRate * 1000;
	}

	inline void setFrequency( float _freq )
	{
		m_frequency = _freq;
	}

	inline const sampleFrame * data() const
	{
		return m_data.data ();
	}

	QString openAudioFile() const;
	QString openAndSetAudioFile();
	QString openAndSetWaveformFile();

	// protect calls from the GUI to this function with dataReadLock() and
	// dataUnlock(), out of loops for efficiency
	inline sample_t userWaveSample( const float _sample ) const
	{
		f_cnt_t dataFrames = frames ();
		const sampleFrame * data = this->data();
		const float frame = _sample * dataFrames;
		f_cnt_t f1 = static_cast<f_cnt_t>( frame ) % dataFrames;
		if( f1 < 0 )
		{
			f1 += dataFrames;
		}
		return linearInterpolate( data[f1][0], data[ (f1 + 1) % dataFrames ][0], fraction( frame ) );
	}

	bool tryDataReadLock ()
	{
		return m_varLock.tryLockForRead ();
	}


	void dataReadLock()
	{
		m_varLock.lockForRead();
	}

	void dataUnlock()
	{
		m_varLock.unlock();
	}

	static QString tryToMakeRelative( const QString & _file );
	static QString tryToMakeAbsolute(const QString & file);

	/**
	 * @brief Add data to the buffer,
	 * @param begin	Beginning of an InputIterator.
	 * @param end	End of an InputIterator.
	 * @param shouldLockMixer	Should we call requestChangeInModel?
	 *
	 * @warning That locks m_varLock for write.
	 */
	void addData(const DataVector &vector, sample_rate_t sampleRate, bool shouldLockMixer=true);

	/**
	 * @brief Reset the class and initialize it with @a newData.
	 * @param newData	mm, that's the new data.
	 * @param dataSampleRate	Sample rate for @a newData.
	 * @param shouldLockMixer	Should we call requestChangeInModel?
	 */
	void resetData(DataVector &&newData, sample_rate_t dataSampleRate, bool shouldLockMixer=true);

	/**
	 * @brief Just reverse the current buffer.
	 * @param shouldLockMixer	Should we call requestChangeInModel?
	 *
	 * This function simply calls `std::reverse` on m_data.
	 */
	void reverse(bool shouldLockMixer=true);

	void loadFromBase64(const QString & _data , sample_rate_t sampleRate);


public slots:
	void setAudioFile( const QString & _audio_file );
	void setStartFrame( const f_cnt_t _s );
	void setEndFrame( const f_cnt_t _e );
	void setAmplification( float _a );
	void sampleRateChanged();

protected:
	QString & toBase64( QString & _dst ) const;
	inline void setSampleRate( sample_rate_t _rate )
	{
		m_sampleRate = _rate;
	}

	static sample_rate_t mixerSampleRate();

	// HACK: libsamplerate < 0.1.8 doesn't get read-only variables
	//	     as const. It has been fixed in 0.1.9 but has not been
	//		 shipped for some distributions.
	//		 This function just returns a variable that should have
	//		 been `const` as non-const to bypass using 0.1.9.
	inline static sampleFrame * libSampleRateSrc (const sampleFrame *ptr)
	{
		return const_cast<sampleFrame*>(ptr);
	}

	void changeAudioFile (QString audioFile);

	static DataVector convertIntToFloat(int_sample_t * & _ibuf, f_cnt_t _frames, int _channels);

	static DataVector decodeSampleSF( QString _f, ch_cnt_t & _channels, sample_rate_t & _sample_rate);
#ifdef LMMS_HAVE_OGGVORBIS
	static DataVector decodeSampleOGGVorbis( QString _f,
						ch_cnt_t & _channels,
						sample_rate_t & _sample_rate);
#endif
	static DataVector decodeSampleDS( QString _f, ch_cnt_t & _channels, sample_rate_t & _sample_rate);

	inline sampleFrame * data()
	{
		return m_data.data ();
	}

	QString m_audioFile;
	DataVector m_data;
	QReadWriteLock m_varLock;
	f_cnt_t m_startFrame;
	f_cnt_t m_endFrame;
	f_cnt_t m_loopStartFrame;
	f_cnt_t m_loopEndFrame;
	float m_amplification;
	float m_frequency;
	sample_rate_t m_sampleRate;

	const
	sampleFrame * getSampleFragment( f_cnt_t _index, f_cnt_t _frames,
						LoopMode _loopmode,
						sampleFrame * * _tmp,
						bool * _backwards, f_cnt_t _loopstart, f_cnt_t _loopend,
						f_cnt_t _end ) const;
	f_cnt_t getLoopedIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf  ) const;
	f_cnt_t getPingPongIndex( f_cnt_t _index, f_cnt_t _startf, f_cnt_t _endf  ) const;


	static DataVector resampleData(const DataVector &inputData, sample_rate_t inputSampleRate, sample_rate_t requiredSampleRate);

	/**
	 * @brief Do the actions necessary before changing m_data.
	 * @param shouldLock	Is anyone else might be using m_data?
	 */
	void beginBufferChange (bool shouldLock, bool shouldLockMixer=true);

	/**
	 * @brief Do some actions necessary after changing m_data.
	 * @param shouldUnlock	The same value you've used on @a beginBufferChange.
	 * @param shouldKeepSettings	Should we keep playback settings?
	 * @param bufferSampleRate		The new m_data's sample rate.
	 */
	void doneBufferChange (bool shouldUnlock,
						   sample_rate_t bufferSampleRate,
						   bool shouldUnlockMixer=true);
signals:
	void sampleUpdated();

} ;


#endif
