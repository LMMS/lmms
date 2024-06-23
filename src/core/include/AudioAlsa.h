/*
 * AudioAlsa.h - device-class that implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_AUDIO_ALSA_H
#define LMMS_AUDIO_ALSA_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

// older ALSA-versions might require this
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>
#include <QThread>

#include "AudioDevice.h"

namespace lmms
{

class AudioAlsa : public QThread, public AudioDevice
{
	Q_OBJECT
public:
	/**
	 * @brief Contains the relevant information about available ALSA devices
	 */
	class DeviceInfo
	{
	public:
		DeviceInfo(QString const & deviceName, QString const & deviceDescription) :
			m_deviceName(deviceName),
			m_deviceDescription(deviceDescription)
		{}
		~DeviceInfo() = default;

		QString const & getDeviceName() const { return m_deviceName; }
		QString const & getDeviceDescription() const { return m_deviceDescription; }

	private:
		QString m_deviceName;
		QString m_deviceDescription;

	};

	using DeviceInfoCollection = std::vector<DeviceInfo>;

public:
	AudioAlsa( bool & _success_ful, AudioEngine* audioEngine );
	~AudioAlsa() override;

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "AudioDeviceSetupWidget",
			"ALSA (Advanced Linux Sound Architecture)" );
	}

	static QString probeDevice();

	static DeviceInfoCollection getAvailableDevices();

private:
	void startProcessing() override;
	void stopProcessing() override;
	void run() override;

	int setHWParams( const ch_cnt_t _channels, snd_pcm_access_t _access );
	int setSWParams();
	int handleError( int _err );


	snd_pcm_t * m_handle;

	snd_pcm_uframes_t m_bufferSize;
	snd_pcm_uframes_t m_periodSize;

	snd_pcm_hw_params_t * m_hwParams;
	snd_pcm_sw_params_t * m_swParams;

	bool m_convertEndian;

} ;

} // namespace lmms

#endif // LMMS_HAVE_ALSA

#endif // LMMS_AUDIO_ALSA_H
