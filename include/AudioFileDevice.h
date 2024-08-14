/*
 * AudioFileDevice.h - base-class for audio-device-classes which write
 *                     their output into a file
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

#ifndef LMMS_AUDIO_FILE_DEVICE_H
#define LMMS_AUDIO_FILE_DEVICE_H

#include <QFile>

#include "OutputSettings.h"

namespace lmms
{

class SampleFrame;

class AudioFileDevice
{
public:
	// getBufferFunction
	using BufferFn = std::function<void(SampleFrame*, fpp_t*, fpp_t)>;

	AudioFileDevice(OutputSettings const & outputSettings,
			const QString & _file, const ch_cnt_t _channels,
			const fpp_t defaultBufferSize);
	virtual ~AudioFileDevice();

	QString outputFile() const
	{
		return m_outputFile.fileName();
	}

	OutputSettings const & getOutputSettings() const { return m_outputSettings; }

	sample_rate_t getSampleRate();
	ch_cnt_t getChannel();
	// how many samples to store in a buffer
	const fpp_t getDefaultFrameCount();

	void setSampleRate(sample_rate_t newSampleRate);

	// save audio
	void processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount);


protected:
	// subclasses can re-implement this for being used in conjunction with
	virtual void writeBuffer(const SampleFrame* /* _buf*/, const fpp_t /*_frames*/) {}

	int writeData( const void* data, int len );

	inline bool outputFileOpened() const
	{
		return m_outputFile.isOpen();
	}

	inline int outputFileHandle() const
	{
		return m_outputFile.handle();
	}

private:
	QFile m_outputFile;
	OutputSettings m_outputSettings;

	const fpp_t m_defaultFrameCount;
	ch_cnt_t m_channelCount;
} ;

using AudioFileDeviceInstantiaton
	= AudioFileDevice* (*)(const OutputSettings&m, bool&, const QString&, const ch_cnt_t, const fpp_t);

} // namespace lmms

#endif // LMMS_AUDIO_FILE_DEVICE_H
