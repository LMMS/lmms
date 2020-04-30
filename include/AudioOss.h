/*
 * AudioOss.h - device-class that implements OSS-PCM-output
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

#ifndef AUDIO_OSS_H
#define AUDIO_OSS_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_OSS

#include <QThread>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"


class LcdSpinBox;
class QLineEdit;


class AudioOss : public QThread, public AudioDevice
{
	Q_OBJECT
public:
	AudioOss( bool & _success_ful, Mixer* mixer );
	virtual ~AudioOss();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "OSS (Open Sound System)" );
	}

	static QString probeDevice();


	class setupWidget : public AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		void saveSettings() override;

	private:
		QLineEdit * m_device;
		LcdSpinBox * m_channels;

	} ;


private:
	void startProcessing() override;
	void stopProcessing() override;
	void applyQualitySettings() override;
	void run() override;

	int m_audioFD;

	bool m_convertEndian;

} ;


#endif

#endif
