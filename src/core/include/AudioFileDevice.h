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

#include "AudioDevice.h"
#include "OutputSettings.h"

namespace lmms
{

class AudioFileDevice : public AudioDevice
{
public:
	AudioFileDevice(OutputSettings const & outputSettings,
			const ch_cnt_t _channels, const QString & _file,
			AudioEngine* audioEngine );
	~AudioFileDevice() override;

	QString outputFile() const
	{
		return m_outputFile.fileName();
	}

	OutputSettings const & getOutputSettings() const { return m_outputSettings; }


protected:
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
} ;

using AudioFileDeviceInstantiaton
	= AudioFileDevice* (*)(const QString&, const OutputSettings&, const ch_cnt_t, AudioEngine*, bool&);

} // namespace lmms

#endif // LMMS_AUDIO_FILE_DEVICE_H
