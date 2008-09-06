/*
 * audio_pulseaudio.h - device-class which implements PulseAudio-output
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

#include "audio_device.h"


class lcdSpinBox;
class QLineEdit;


class audioPulseAudio : public audioDevice, public QThread
{
public:
	audioPulseAudio( bool & _success_ful, mixer * _mixer );
	virtual ~audioPulseAudio();

	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
						"PulseAudio (bad latency!)" ) );
	}

	static QString probeDevice( void );


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		QLineEdit * m_device;
		lcdSpinBox * m_channels;

	} ;


	void streamWriteCallback( pa_stream * s, size_t length );

	pa_stream * m_s;
	pa_sample_spec m_sampleSpec;


private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );
	virtual void applyQualitySettings( void );
	virtual void run( void );


	bool m_convertEndian;

} ;

#endif

#endif
