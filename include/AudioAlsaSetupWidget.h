/*
 * AudioAlsaSetupWidget.h - Implements a setup widget for ALSA-PCM-output
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

#ifndef LMMS_GUI_AUDIO_ALSA_SETUP_WIDGET_H
#define LMMS_GUI_AUDIO_ALSA_SETUP_WIDGET_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_ALSA

#include "AudioDeviceSetupWidget.h"

#include "AudioAlsa.h"


class QComboBox;

namespace lmms::gui
{

class LcdSpinBox;

class AudioAlsaSetupWidget : public AudioDeviceSetupWidget
{
	Q_OBJECT

public:
	AudioAlsaSetupWidget( QWidget * _parent );
	~AudioAlsaSetupWidget() override;

	void saveSettings() override;

public slots:
	void onCurrentIndexChanged(int index);

private:
	QComboBox * m_deviceComboBox;
	LcdSpinBox * m_channels;

	int m_selectedDevice;
	AudioAlsa::DeviceInfoCollection m_deviceInfos;
};

} // namespace lmms::gui

#endif // LMMS_HAVE_ALSA

#endif // LMMS_GUI_AUDIO_ALSA_SETUP_WIDGET_H
