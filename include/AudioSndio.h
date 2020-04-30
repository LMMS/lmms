/*
 * AudioSndio.h - base-class that implements sndio audio support
 *
 * Copyright (c) 2010-2016 jackmsr@openbsd.net
 * Copyright (c) 2016-2017 David Carlier <devnexen@gmail.com>
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

#ifndef _AUDIO_SNDIO_H
#define _AUDIO_SNDIO_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SNDIO

#include <QThread>
#include <sndio.h>

#include "AudioDevice.h"
#include "AudioDeviceSetupWidget.h"

class LcdSpinBox;
class QLineEdit;


class AudioSndio : public QThread, public AudioDevice
{
	Q_OBJECT
public:
	AudioSndio( bool & _success_ful, Mixer * _mixer );
	virtual ~AudioSndio();

	inline static QString name( void )
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "sndio" );
	}

	class setupWidget : public AudioDeviceSetupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		void saveSettings( void ) override;

	private:
		QLineEdit * m_device;
		LcdSpinBox * m_channels;
	} ;

private:
	void startProcessing( void ) override;
	void stopProcessing( void ) override;
	void applyQualitySettings( void ) override;
	void run( void ) override;

	struct sio_hdl *m_hdl;
	struct sio_par m_par;

	bool m_convertEndian;
} ;


#endif	/* LMMS_HAVE_SNDIO */

#endif	/* _AUDIO_SNDIO_H */
