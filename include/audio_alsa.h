/*
 * audio_alsa.h - device-class that implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_ALSA_H
#define _AUDIO_ALSA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ALSA_ASOUNDLIB_H

#ifndef ALSA_SUPPORT
#define ALSA_SUPPORT
#endif

// older ALSA-versions might require this
#define ALSA_PCM_NEW_HW_PARAMS_API

#include <alsa/asoundlib.h>

#include "audio_device.h"


class lcdSpinBox;
class QLineEdit;


class audioALSA : public audioDevice, public QThread
{
public:
	audioALSA( bool & _success_ful, mixer * _mixer );
	virtual ~audioALSA();

	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
			"ALSA (Advanced Linux Sound Architecture)" ) );
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


private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );
	virtual void applyQualitySettings( void );
	virtual void run( void );

	int setHWParams( const ch_cnt_t _channels, snd_pcm_access_t _access );
	int setSWParams( void );
	int handleError( int _err );


	snd_pcm_t * m_handle;

	snd_pcm_uframes_t m_bufferSize;
	snd_pcm_uframes_t m_periodSize;

	snd_pcm_hw_params_t * m_hwParams;
	snd_pcm_sw_params_t * m_swParams;

	bool m_convertEndian;

} ;

#endif

#endif
