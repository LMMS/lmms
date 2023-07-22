/*
 * AudioDeviceSetupGroupWidget.h - Base class for audio device setup widgets using group box
 *
 * Copyright (c) 2004-2015 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2004-2015 Michael Gregorius
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

#ifndef LMMS_GUI_AUDIO_DEVICE_SETUP_GROUP_WIDGET_H
#define LMMS_GUI_AUDIO_DEVICE_SETUP_GROUP_WIDGET_H

#include <QGroupBox>

namespace lmms::gui
{

class AudioDeviceSetupGroupWidget : public QGroupBox
{
    Q_OBJECT
public:
	AudioDeviceSetupGroupWidget( const QString & _caption, QWidget * _parent );

	~AudioDeviceSetupGroupWidget() override = default;

	virtual void saveSettings() = 0;

	virtual void show();
};

} // namespace lmms::gui

#endif // LMMS_GUI_AUDIO_DEVICE_SETUP_GROUP_WIDGET_H
