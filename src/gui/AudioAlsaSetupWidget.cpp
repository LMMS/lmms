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
#include <QFormLayout>

#include "AudioAlsaSetupWidget.h"

#ifdef LMMS_HAVE_ALSA

#include "ConfigManager.h"
#include "LcdSpinBox.h"
#include "lmms_constants.h"

namespace lmms::gui
{

AudioAlsaSetupWidget::AudioAlsaSetupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioAlsa::name(), _parent ),
	m_selectedDevice(-1)
{
	QFormLayout * form = new QFormLayout(this);

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

	connect(m_deviceComboBox,
			SIGNAL(currentIndexChanged(int)),
			SLOT(onCurrentIndexChanged(int)));

	form->addRow(tr("Device"), m_deviceComboBox);

	auto m = new LcdSpinBoxModel(/* this */);
	m->setRange(DEFAULT_CHANNELS, DEFAULT_CHANNELS);
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audioalsa",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );

	form->addRow(tr("Channels"), m_channels);
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


} // namespace lmms::gui

#endif // LMMS_HAVE_ALSA
