/*
 * AudioPulseAudio.h - device-class which implements PulseAudio-output
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_PULSEAUDIO_H
#define _AUDIO_PULSEAUDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_PULSEAUDIO

#include <pulse/pulseaudio.h>

#include "AudioDevice.h"


class LcdSpinBox;
class QLineEdit;


class AudioPulseAudio : public AudioDevice, public QThread
{
public:
	AudioPulseAudio( bool & _success_ful, Mixer* mixer );
	virtual ~AudioPulseAudio();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "PulseAudio (bad latency!)" );
	}

	static QString probeDevice();


	class setupWidget : public AudioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		QLineEdit * m_device;
		LcdSpinBox * m_channels;

	} ;


	void streamWriteCallback( pa_stream * s, size_t length );

	pa_stream * m_s;
	pa_sample_spec m_sampleSpec;


private:
	virtual void startProcessing();
	virtual void stopProcessing();
	virtual void applyQualitySettings();
	virtual void run();

	volatile bool m_quit;

	bool m_convertEndian;

} ;

#endif

#endif
