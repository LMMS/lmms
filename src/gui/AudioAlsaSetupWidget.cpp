/*
 * AudioAlsa.cpp - device-class which implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

#include "AudioAlsa.h"
#include "AudioAlsaSetupWidget.h"

#ifdef LMMS_HAVE_ALSA

#include "ConfigManager.h"
#include "LcdSpinBox.h"
#include "gui_templates.h"

#include <iostream>


void device_list(void)
{
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	snd_ctl_t *handle;
	int card, err, dev, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		return;
	}
	std::cout << "**** List of " << snd_pcm_stream_name(stream) << " Hardware Devices ****\n";

	while (card >= 0) {
		char name[32];
		sprintf(name, "hw:%d", card);
		if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
			// TODO Error handling
			//error("control open (%i): %s", card, snd_strerror(err));
			goto next_card;
		}
		if ((err = snd_ctl_card_info(handle, info)) < 0) {
			// TODO Error handling
			//error("control hardware info (%i): %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			goto next_card;
		}
		dev = -1;
		while (1) {
			unsigned int count;
			if (snd_ctl_pcm_next_device(handle, &dev)<0)
				// TODO Error handling
				//error("snd_ctl_pcm_next_device");
				std::cerr << "snd_ctl_pcm_next_device";
			if (dev < 0)
				break;
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
				if (err != -ENOENT)
					// TODO Error handling
					//error("control digital audio info (%i): %s", card, snd_strerror(err));
					std::cerr << "Error\n";
				continue;
			}
			//std::cout << "card " << card << ": " << snd_ctl_card_info_get_id(info) << " [" << snd_ctl_card_info_get_name(info) << "], device " << dev <<
			//			 ": " << snd_pcm_info_get_id(pcminfo) << " [" << snd_pcm_info_get_name(pcminfo) << "]\n";
			std::cout << "card hw" << card << ":" << dev << " - " << "[" << snd_ctl_card_info_get_name(info) << "/" << snd_pcm_info_get_name(pcminfo) << "], " <<
						 snd_ctl_card_info_get_id(info) << "," << snd_pcm_info_get_id(pcminfo) << "\n";

			count = snd_pcm_info_get_subdevices_count(pcminfo);
			std::cout << "  Subdevices: " << snd_pcm_info_get_subdevices_avail(pcminfo) << "/" << count << std::endl;
			for (idx = 0; idx < (int)count; idx++) {
				snd_pcm_info_set_subdevice(pcminfo, idx);
				if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
					// TODO Error handling
					//error("control digital audio playback info (%i): %s", card, snd_strerror(err));
					std::cerr << "Error\n";
				} else {
					std::cout << " Subdevice #" << idx << ": " << snd_pcm_info_get_subdevice_name(pcminfo) << std::endl;
				}
			}
		}
		snd_ctl_close(handle);
next_card:
		if (snd_card_next(&card) < 0) {
			// TODO Error handling
			//error("snd_card_next");
			break;
		}
	}
}



void AudioAlsaSetupWidget::populateDeviceInfos(std::vector<DeviceInfo> &deviceInfos)
{
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	snd_ctl_t *handle;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;

	// Allocate memory for the info structs
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	int card = -1;

	while (!snd_card_next(&card) && card >= 0)
	{
		std::cout << "Card: " << card << " found!" << std::endl;

		char name[32];
		sprintf(name, "hw:%d", card);

		if (snd_ctl_open(&handle, name, 0) < 0)
		{
			std::cerr << "Error opening ALSA card " << name << std::endl;
			continue;
		}
		if (snd_ctl_card_info(handle, info) < 0)
		{
			snd_ctl_close(handle);
			std::cerr << "Could not retrieve info for ALSA card " << name << std::endl;
			continue;
		}

		int dev = -1;

		while (!snd_ctl_pcm_next_device(handle, &dev) && dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			if (!snd_ctl_pcm_info(handle, pcminfo))
			{
				QString cardName(snd_ctl_card_info_get_name(info));
				QString pcmName(snd_pcm_info_get_name(pcminfo));
				QString cardId(snd_ctl_card_info_get_id(info));
				QString pcmId(snd_pcm_info_get_id(pcminfo));

				DeviceInfo currentDevice(card, dev, cardName, pcmName, cardId, pcmId);
				deviceInfos.push_back(currentDevice);

				std::cout << "card hw" << card << ":" << dev << " - " << "[" << snd_ctl_card_info_get_name(info) << "/" << snd_pcm_info_get_name(pcminfo) << "], " <<
							 snd_ctl_card_info_get_id(info) << "," << snd_pcm_info_get_id(pcminfo) << std::endl;
			}
		}

		snd_ctl_close(handle);
	}
}




AudioAlsaSetupWidget::AudioAlsaSetupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioAlsa::name(), _parent ),
	m_selectedDevice(-1)
{
	populateDeviceInfos(m_deviceInfos);

	QString deviceText = ConfigManager::inst()->value( "audioalsa", "device" );

	// Implements the "-l" from aplay
	//device_list();

	m_deviceComboBox = new QComboBox(this);
	for (size_t i = 0; i < m_deviceInfos.size(); ++i)
	{
		DeviceInfo const & currentDeviceInfo = m_deviceInfos[i];
		QString comboBoxText = currentDeviceInfo.getHWString() + " [" + currentDeviceInfo.getCardName() + " | " + currentDeviceInfo.getPcmName() + "]";
		m_deviceComboBox->addItem(comboBoxText, QVariant(static_cast<uint>(i)));
		m_deviceComboBox->setItemData(i, comboBoxText, Qt::ToolTipRole);

		if (currentDeviceInfo.getHWString() == deviceText)
		{
			m_deviceComboBox->setCurrentIndex(static_cast<int>(i));
		}
	}

	m_selectedDevice = m_deviceComboBox->currentIndex();

	m_deviceComboBox->setGeometry( 10, 20, 160, 20 );
	connect(m_deviceComboBox,
			SIGNAL(currentIndexChanged(int)),
			SLOT(onCurrentIndexChanged(int)));

	//m_device = new QLineEdit( AudioAlsa::probeDevice(), this );
	//m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	LcdSpinBoxModel * m = new LcdSpinBoxModel( /* this */ );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audioalsa",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 180, 20 );

}




AudioAlsaSetupWidget::~AudioAlsaSetupWidget()
{
	delete m_channels->model();
}




void AudioAlsaSetupWidget::saveSettings()
{
	QString deviceText;

	if (m_selectedDevice != -1)
	{
		DeviceInfo const & selectedDevice = m_deviceInfos[m_selectedDevice];
		deviceText = selectedDevice.getHWString();
	}

	ConfigManager::inst()->setValue( "audioalsa", "device", deviceText );
	ConfigManager::inst()->setValue( "audioalsa", "channels",
				QString::number( m_channels->value<int>() ) );
}



void AudioAlsaSetupWidget::onCurrentIndexChanged(int index)
{
	m_selectedDevice = index;
}


#endif
