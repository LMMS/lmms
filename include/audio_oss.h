/*
 * audio_oss.h - device-class that implements OSS-PCM-output
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_OSS_H
#define _AUDIO_OSS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined HAVE_SYS_SOUNDCARD_H || defined HAVE_SOUNDCARD_H

#define OSS_SUPPORT


#include "audio_device.h"
#include "tab_widget.h"



class lcdSpinBox;
class QLineEdit;


class audioOSS : public audioDevice, public QThread
{
public:
	audioOSS( const sample_rate_t _sample_rate, bool & _success_ful,
							mixer * _mixer );
	virtual ~audioOSS();

	inline static QString name( void )
	{
		return( setupWidget::tr( "OSS (Open Sound System)" ) );
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
	virtual void run( void );

	int m_audioFD;

	bool m_convertEndian;
	volatile bool m_quit;

} ;


#endif

#endif
