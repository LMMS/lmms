/*
 * AudioAlsaSetupWidget.cpp - Implements a setup widget for ALSA-PCM-output
 *
 * Copyright (c) 2004-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QComboBox>
#include <QLabel>

#include "AudioAlsaSetupWidget.h"

#ifdef LMMS_HAVE_ALSA

#include "ConfigManager.h"
#include "LcdSpinBox.h"
#include "gui_templates.h"


AudioAlsaSetupWidget::AudioAlsaSetupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioAlsa::name(), _parent ),
	m_selectedDevice(-1)
{
	m_deviceInfos = AudioAlsa::getAvailableDevices();

	QString deviceText = ConfigManager::inst()->value( "audioalsa", "device" );

	m_deviceComboBox = new QComboBox(this);
	for (size_t i = 0; i < m_deviceInfos.size(); ++i)
	{
		AudioAlsa::DeviceInfo const & currentDeviceInfo = m_deviceInfos[i];
		QString comboBoxText = currentDeviceInfo.getDeviceName();
		m_deviceComboBox->addItem(comboBoxText, QVariant(static_cast<uint>(i)));

		QString toolTipText = currentDeviceInfo.getDeviceDescription();
		m_deviceComboBox->setItemData(i, toolTipText, Qt::ToolTipRole);

		if (currentDeviceInfo.getDeviceName() == deviceText)
		{
			m_deviceComboBox->setCurrentIndex(static_cast<int>(i));
		}
	}

	m_selectedDevice = m_deviceComboBox->currentIndex();

	m_deviceComboBox->setGeometry( 10, 20, 160, 20 );
	connect(m_deviceComboBox,
			SIGNAL(currentIndexChanged(int)),
			SLOT(onCurrentIndexChanged(int)));

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
		AudioAlsa::DeviceInfo const & selectedDevice = m_deviceInfos[m_selectedDevice];
		deviceText = selectedDevice.getDeviceName();
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
