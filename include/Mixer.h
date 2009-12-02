/*
 * Mixer.h - Mixer for audio processing and rendering
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
#include <QtCore/QVector>
#include <QtCore/QWaitCondition>


#include "lmms_basics.h"
#include "note.h"


class AudioBackend;
class AudioOutputContext;
class AudioPort;
class MidiClient;


const fpp_t DEFAULT_BUFFER_SIZE = 256;


const float BaseFreq = 440.0f;
const Keys BaseKey = Key_A;
const Octaves BaseOctave = DefaultOctave;


class MixerWorkerThread;

#include "ThreadableJob.h"
#include "play_handle.h"


/*! \brief The Mixer class is responsible for processing and rendering audio chunks. */
class EXPORT Mixer : public QObject
{
	Q_OBJECT
public:
	void initDevices();
	void clear();


	// audio-device-stuff
	inline const QString & audioDevName() const
	{
		return m_audioDevName;
	}

	/*! \brief Sets a specific AudioOutputContext to be the active context. */
	void setAudioOutputContext( AudioOutputContext * context );
	const AudioOutputContext * audioOutputContext() const
	{
		return m_audioOutputContext;
	}
	AudioOutputContext * audioOutputContext()
	{
		return m_audioOutputContext;
	}
	AudioOutputContext * defaultAudioOutputContext()
	{
		return m_defaultAudioOutputContext;
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

	inline const sampleFrameA * currentReadBuffer() const
	{
		return m_readBuf;
	}


	inline int cpuLoad() const
	{
		return m_cpuLoad;
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

	static float peakValueLeft( sampleFrame * _ab, const f_cnt_t _frames );
	static float peakValueRight( sampleFrame * _ab, const f_cnt_t _frames );


	bool criticalXRuns() const;

	void pushInputFrames( sampleFrame * _ab, const f_cnt_t _frames );

	inline const sampleFrame * inputBuffer()
	{
		return m_inputBuffer[ m_inputBufferRead ];
	}

	inline f_cnt_t inputBufferFrames() const
	{
		return m_inputBufferFrames[ m_inputBufferRead ];
	}

	/*! \brief Processes and renders next chunk of audio. */
	sampleFrameA * renderNextBuffer();


signals:
	void sampleRateChanged();
	void nextAudioBuffer();


private:
	Mixer();
	virtual ~Mixer();

	void startProcessing();
	void stopProcessing();


	AudioBackend * tryAudioBackends();
	MidiClient * tryMidiClients();


	QVector<AudioPort *> m_audioPorts;

	fpp_t m_framesPerPeriod;

	sampleFrame * m_workingBuf;

	sampleFrame * m_inputBuffer[2];
	f_cnt_t m_inputBufferFrames[2];
	f_cnt_t m_inputBufferSize[2];
	int m_inputBufferRead;
	int m_inputBufferWrite;

	sampleFrameA * m_readBuf;
	sampleFrameA * m_writeBuf;

	QVector<sampleFrameA *> m_bufferPool;
	int m_readBuffer;
	int m_writeBuffer;
	int m_poolDepth;

	sampleFrame m_maxClip;
	sampleFrame m_previousSample;
	fpp_t m_halfStart[SURROUND_CHANNELS];
	bool m_oldBuffer[SURROUND_CHANNELS];
	bool m_newBuffer[SURROUND_CHANNELS];

	int m_cpuLoad;
	QVector<MixerWorkerThread *> m_workers;
	int m_numWorkers;
	QWaitCondition m_queueReadyWaitCond;


	PlayHandleList m_playHandles;
	ConstPlayHandleList m_playHandlesToRemove;

	float m_masterGain;


	AudioOutputContext * m_audioOutputContext;
	AudioOutputContext * m_defaultAudioOutputContext;
	QString m_audioDevName;


	MidiClient * m_midiClient;
	QString m_midiClientName;


	QMutex m_globalMutex;
	QMutex m_inputFramesMutex;


	friend class engine;

} ;

#endif
