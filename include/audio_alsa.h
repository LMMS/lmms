/*
 * audio_alsa.h - device-class that implements ALSA-PCM-output
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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


class audioALSA : public audioDevice
{
public:
	audioALSA( Uint32 _sample_rate, bool & _success_ful );
	~audioALSA();

	inline static QString name( void )
	{
		return( setupWidget::tr( "ALSA (Advanced Linux Sound "
							"Architecture)" ) );
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
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame * _ab,
						Uint32 _frames,
						float _master_output );

	int FASTCALL setHWParams( Uint32 _sample_rate, Uint32 _channels,
						snd_pcm_access_t _access );
	int setSWParams( void );
	int FASTCALL handleError( int _err );


	snd_pcm_t * m_handle;

	snd_pcm_uframes_t m_bufferSize;
	snd_pcm_uframes_t m_periodSize;

	snd_pcm_hw_params_t * m_hwParams;
	snd_pcm_sw_params_t * m_swParams;

	bool m_littleEndian;

} ;

#endif

#endif
