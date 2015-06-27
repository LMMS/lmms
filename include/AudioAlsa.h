/*
 * AudioAlsa.h - device-class that implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef AUDIO_ALSA_H
#define AUDIO_ALSA_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

// older ALSA-versions might require this
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

#include "AudioDevice.h"


class AudioAlsa : public AudioDevice, public QThread
{
	// Public classes and enums
public:
	/**
	 * @brief Contains the relevant information about available ALSA devices
	 */
	class DeviceInfo
	{
	public:
		DeviceInfo(int cardNumber, int deviceNumber,
				   QString const & cardName, QString const & pcmName,
				   QString const & cardId, QString const & pcmId) :
			m_cardNumber(cardNumber),
			m_deviceNumber(deviceNumber),
			m_cardName(cardName),
			m_pcmName(pcmName),
			m_cardId(cardId),
			m_pcmId(pcmId)
		{}
		~DeviceInfo() {}

		int getCardNumber() const { return m_cardNumber; }
		int getDeviceNumber() const { return m_deviceNumber; }
		QString const & getCardName() const { return m_cardName; }
		QString const & getPcmName() const { return m_pcmName; }
		QString const & getCardId() const { return m_cardId; }
		QString const & getPcmId() const { return m_pcmId; }

		QString getHWString() const { return QString("hw:%1,%2").arg(m_cardNumber).arg(m_deviceNumber); }

	private:
		int m_cardNumber;
		int m_deviceNumber;
		QString m_cardName;
		QString m_pcmName;
		QString m_cardId;
		QString m_pcmId;
	};

	typedef std::vector<DeviceInfo> DeviceInfoCollection;

public:
	AudioAlsa( bool & _success_ful, Mixer* mixer );
	virtual ~AudioAlsa();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget",
			"ALSA (Advanced Linux Sound Architecture)" );
	}

	static QString probeDevice();

	static DeviceInfoCollection getAvailableDevices();

private:
	virtual void startProcessing();
	virtual void stopProcessing();
	virtual void applyQualitySettings();
	virtual void run();

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

#endif

#endif
