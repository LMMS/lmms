/*
 * AudioDeviceSetupWidget.h - Base class for audio device setup widgets
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

#ifndef AUDIO_DEVICE_SETUP_WIDGET_H
#define AUDIO_DEVICE_SETUP_WIDGET_H

#include "TabWidget.h"


class AudioDeviceSetupWidget : public TabWidget
{
    Q_OBJECT
public:
	AudioDeviceSetupWidget( const QString & _caption, QWidget * _parent );

	virtual ~AudioDeviceSetupWidget();

	virtual void saveSettings() = 0;

	virtual void show();
};


#endif
