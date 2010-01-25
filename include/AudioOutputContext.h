/*
 * AudioOutputContext.h - centralize all audio output related functionality
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_OUTPUT_CONTEXT_H
#define _AUDIO_OUTPUT_CONTEXT_H

#include <QtCore/QSemaphore>
#include <QtCore/QThread>

#include "Mixer.h"

class AudioBackend;

/*! \brief The AudioOutputContext class centralizes all functionality
 * and data related to output of audio data.
 *
 * The process of audio output is rather complicated due to different kinds of
 * AudioBackend implementations, FIFO buffering and dedicated quality settings.
 * The AudioOutputContext class handles all this so the Mixer class can just
 * deal with actual audio rendering and processing.
 */
class AudioOutputContext
{
public:
	/*! \brief The QualitySettings class holds quality related settings.
	 *
	 * There's nothing special about it. It's just a data aggregration class.
	 */
	class QualitySettings
	{
	public:
		/*! Lists all quality presets. */
		enum Preset
		{
			Preset_Draft,		/*!< Draft quality - used for editing project */
			Preset_HighQuality,	/*!< High quality - standard setting for project export */
			Preset_FinalMix,	/*!< Final mix quality - very slow, best quality */
			NumPresets
		} ;

		/*! Lists all supported interpolation types. */
		enum Interpolation
		{
			Interpolation_Linear,		/*!< Linear interpolation - fast */
			Interpolation_SincFastest,	/*!< Fastest Sinc interpolation - good quality */
			Interpolation_SincMedium,	/*!< Medium Sinc interpolation - better quality */
			Interpolation_SincBest		/*!< High quality interpolation */
		} ;

		/*! Lists all supported oversampling ratios. */
		enum Oversampling
		{
			Oversampling_None,			/*!< No oversampling - fast */
			Oversampling_2x,			/*!< 2x oversampling - good quality */
			Oversampling_4x,			/*!< 4x oversampling - better quality */
			Oversampling_8x				/*!< 8x oversampling - best quality but might break some filters */
		} ;

		/*! \brief Constructs a QualitySettings object based on a given preset. */
		QualitySettings( Preset m )
		{
			switch( m )
			{
				case Preset_Draft:
					m_interpolation = Interpolation_Linear;
					m_oversampling = Oversampling_None;
					m_sampleExactControllers = false;
					m_aliasFreeOscillators = false;
					break;
				case Preset_HighQuality:
					m_interpolation = Interpolation_SincFastest;
					m_oversampling = Oversampling_2x;
					m_sampleExactControllers = true;
					m_aliasFreeOscillators = false;
					break;
				case Preset_FinalMix:
					m_interpolation = Interpolation_SincBest;
					m_oversampling = Oversampling_8x;
					m_sampleExactControllers = true;
					m_aliasFreeOscillators = true;
					break;
				default:
					break;
			}
		}

		/*! \brief Constructs a QualitySettings object based on specific quality settings. */
		QualitySettings( Interpolation _i, Oversampling _o, bool _sec,
								bool _afo ) :
			m_interpolation( _i ),
			m_oversampling( _o ),
			m_sampleExactControllers( _sec ),
			m_aliasFreeOscillators( _afo )
		{
		}

		/*! \brief Returns multiplier for sample rate based on oversampling settings. */
		int sampleRateMultiplier() const
		{
			switch( oversampling() )
			{
				case Oversampling_None: return 1;
				case Oversampling_2x: return 2;
				case Oversampling_4x: return 4;
				case Oversampling_8x: return 8;
			}
			return 1;
		}

		/*! \brief Maps interpolation setting to libsamplerate constants. */
		int libsrcInterpolation() const
		{
			switch( interpolation() )
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

		/*! \brief Returns current interpolation setting. */
		Interpolation interpolation() const
		{
			return m_interpolation;
		}

		/*! \brief Sets a new interpolation method. */
		void setInterpolation( Interpolation interpolation )
		{
			m_interpolation = interpolation;
		}

		/*! \brief Returns current oversampling setting. */
		Oversampling oversampling() const
		{
			return m_oversampling;
		}

		/*! \brief Sets a new oversampling factor. */
		void setOversampling( Oversampling oversampling )
		{
			m_oversampling = oversampling;
		}

		/*! \brief Returns whether to use sample exact controllers. */
		bool sampleExactControllers() const
		{
			return m_sampleExactControllers;
		}

		/*! \brief Returns whether to use alias free oscillators. */
		bool aliasFreeOscillators() const
		{
			return m_aliasFreeOscillators;
		}

	private:
		Interpolation m_interpolation;
		Oversampling m_oversampling;
		bool m_sampleExactControllers;
		bool m_aliasFreeOscillators;

	} ;

	/*! \brief The BufferFifo class provides an internal FIFO for rendered buffers.
	 *
	 * When working with buffer sizes greater than the default buffer size, one
	 * big output buffer is still splitted into smaller chunks. This is
	 * especially necessary for automation which takes place once a buffer
	 * period. Transitions would be anything else but smooth when adjusting a
	 * control just 20 times per second. BufferFifo handles the queueing of
	 * rendered buffers. */
	class BufferFifo
	{
	public:
		/*! Each buffer in the FIFO can have a special state. This is used
		 * by FifoWriter to inject NULL buffers to indicate, the FIFO has
		 * been emptied after FifoWriter was told to finish. */
		enum BufferStates
		{
			Running,	/*!< Regular buffer */
			NullBuffer	/*!< Even if the buffer returned by currentReadBuffer()
						 * is not NULL, the FIFO input was NULL. FIFO reader can
						 * use this information for own purposes. */
		} ;
		typedef BufferStates BufferState;

		/*! \brief Constructs a new BufferFifo object.
		 *
		 * \param size The number of buffers in the FIFO
		 * \param bufferSize The size of each buffer in the FIFO
		 */
		BufferFifo( int size, int bufferSize );
		~BufferFifo();

		/*! \brief Pushes a new buffer into the FIFO.
		 *
		 * You can also push NULL which will set the according buffer state
		 * to HasNullBuffer. */
		void write( sampleFrameA * buffer );

		/*! \brief Prepares for reading next buffer (might block until one is available). */
		void startRead();

		/*! \brief Returns current front buffer for reading. */
		sampleFrameA * currentReadBuffer() const
		{
			return m_buffers[m_readerIndex];
		}

		/*! \brief Returns state of current front buffer. */
		BufferState currentReadBufferState() const
		{
			return m_bufferStates[m_readerIndex];
		}

		/*! \brief Finish the current buffer read operation.
		 *
		* The buffer returned by currentReadBuffer() is not guaranteed to
		* be valid anymore after calling this function. */
		void finishRead();

		/*! \brief Returns whether FIFO is empty. */
		bool isEmpty() const
		{
			return m_readerSem.available() == false;
		}


	private:
		QSemaphore m_readerSem;
		QSemaphore m_writerSem;
		int m_readerIndex;
		int m_writerIndex;
		int m_size;
		int m_bufferSize;
		sampleFrameA * * m_buffers;
		BufferState * m_bufferStates;

	} ;

	/*! \brief The FifoWriter class provides an internal thread for feeding
	 * the FIFO read by the active AudioBackend */
	class FifoWriter : public QThread
	{
	public:
		FifoWriter( AudioOutputContext * context );

		void finish();


	private:
		AudioOutputContext * m_context;
		volatile bool m_writing;

		virtual void run();

	} ;

	/*! \brief Constructs an AudioOutputContext object for given AudioBackend.
	 *
	 * \param mixer The Mixer instance to fetch audio data from
	 * \param audioBackend The AudioBackend to write audio data to
	 * \param qualitySettings A QualitySettings object describing desired quality
	 */
	AudioOutputContext( Mixer * mixer,
						AudioBackend * audioBackend,
						const QualitySettings & qualitySettings );
	~AudioOutputContext();

	/*! \brief Sets an AudioBackend for this context. */
	void setAudioBackend( AudioBackend * backend )
	{
		m_audioBackend = backend;
	}

	/*! \brief Returns AudioBackend used by this context. */
	AudioBackend * audioBackend()
	{
		return m_audioBackend;
	}

	/*! \brief Returns const AudioBackend used by this context. */
	const AudioBackend * audioBackend() const
	{
		return m_audioBackend;
	}

	/*! \brief Returns Mixer used by this context. */
	Mixer * mixer()
	{
		return m_mixer;
	}

	/*! \brief Returns const Mixer used by this context. */
	const Mixer * mixer() const
	{
		return m_mixer;
	}

	/*! \brief Returns BufferFifo object used by this context. */
	BufferFifo * fifo()
	{
		return m_fifo;
	}

	/*! \brief Returns current quality settings. */
	const QualitySettings & qualitySettings() const
	{
		return m_qualitySettings;
	}

	/*! \brief Starts audio processing in this context. */
	void startProcessing();

	/*! \brief Stops audio processing in this context. */
	void stopProcessing();

	/*! \brief Returns whether audio processing in this context is running. */
	bool isProcessing() const;

	/*! \brief Copies current output buffer to destination buffer and optionally
	 * does resampling.
	 *
	 * If the Mixer has a running FifoWriter, it will make the FifoWriter start
	 * rendering the next buffer so it can be read from the Fifo next period
	 * without any delay. If the desired sample rate does not match current
	 * processing sample rate, resampling will be done.
	 * \param destBuffer The (aligned) destination buffer
	 * \param destSampleRate The desired output sample rate */
	int getCurrentOutputBuffer( sampleFrameA * destBuffer,
									sample_rate_t destSampleRate );


private:
	Mixer * m_mixer;
	QualitySettings m_qualitySettings;
	AudioBackend * m_audioBackend;
	BufferFifo * m_fifo;
	FifoWriter * m_fifoWriter;

	// resample data
	SRC_DATA m_srcData;
	SRC_STATE * m_srcState;


} ;


#endif
