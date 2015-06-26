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

#ifndef AUDIO_ALSA_SETUP_WIDGET_H
#define AUDIO_ALSA_SETUP_WIDGET_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

#include "AudioDeviceSetupWidget.h"

#include <vector>


class QComboBox;
class LcdSpinBox;
class QLineEdit;


class AudioAlsaSetupWidget : public AudioDeviceSetupWidget
{
	Q_OBJECT

public:
	AudioAlsaSetupWidget( QWidget * _parent );
	virtual ~AudioAlsaSetupWidget();

	virtual void saveSettings();

public slots:
	void onCurrentIndexChanged(int index);

private:
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

	void populateDeviceInfos(std::vector<DeviceInfo> &deviceInfos);

private:
	QComboBox * m_deviceComboBox;
	QLineEdit * m_device;
	LcdSpinBox * m_channels;

	int m_selectedDevice;
	typedef std::vector<DeviceInfo> DeviceInfoCollection;
	DeviceInfoCollection m_deviceInfos;
} ;

#endif

#endif
