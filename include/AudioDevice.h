/*
 * AudioDevice.h - base-class for audio-devices, used by LMMS audio engine
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

#ifndef LMMS_AUDIO_DEVICE_H
#define LMMS_AUDIO_DEVICE_H

#include <QMutex>
#include <samplerate.h>

#include "lmms_basics.h"

class QThread;

namespace lmms
{

class AudioEngine;
class AudioPort;
class SampleFrame;


class AudioDevice
{
public:
	AudioDevice( const ch_cnt_t _channels, AudioEngine* audioEngine );
	virtual ~AudioDevice();

	inline void lock()
	{
		m_devMutex.lock();
	}

	inline void unlock()
	{
		m_devMutex.unlock();
	}


	// if audio-driver supports ports, classes inherting AudioPort
	// (e.g. channel-tracks) can register themselves for making
	// audio-driver able to collect their individual output and provide
	// them at a specific port - currently only supported by JACK
	virtual void registerPort( AudioPort * _port );
	virtual void unregisterPort( AudioPort * _port );
	virtual void renamePort( AudioPort * _port );


	inline bool supportsCapture() const
	{
		return m_supportsCapture;
	}

	inline sample_rate_t sampleRate() const
	{
		return m_sampleRate;
	}

	ch_cnt_t channels() const
	{
		return m_channels;
	}

	void processNextBuffer();

	virtual void startProcessing()
	{
		m_inProcess = true;
	}

	virtual void stopProcessing();


	// convert a given audio-buffer to a buffer in signed 16-bit samples
	// returns num of bytes in outbuf
	static int convertToS16(const SampleFrame* _ab,
						const fpp_t _frames,
						int_sample_t * _output_buffer,
						ch_cnt_t channels,
						const bool _convert_endian = false );

	// clear given signed-int-16-buffer
	static void clearS16Buffer( int_sample_t * _outbuf,
							const fpp_t _frames,
							ch_cnt_t channels);
protected:
	// subclasses can re-implement this for being used in conjunction with
	// processNextBuffer()
	virtual void writeBuffer(const SampleFrame* /* _buf*/, const fpp_t /*_frames*/) {}

	// called by according driver for fetching new sound-data
	fpp_t getNextBuffer(SampleFrame* _ab);

	inline void setSampleRate(const sample_rate_t _new_sr)
	{
		m_sampleRate = _new_sr;
	}

	AudioEngine* audioEngine()
	{
		return m_audioEngine;
	}

	static void stopProcessingThread( QThread * thread );


protected:
	bool m_supportsCapture;


private:
	sample_rate_t m_sampleRate;
	ch_cnt_t m_channels;
	AudioEngine* m_audioEngine;
	bool m_inProcess;

	QMutex m_devMutex;

	SampleFrame* m_buffer;

};

} // namespace lmms

#endif // LMMS_AUDIO_DEVICE_H
