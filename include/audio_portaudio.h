/*
 * audio_portaudio.h - device-class that performs PCM-output via PortAudio
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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


#ifndef _AUDIO_PORTAUDIO_H
#define _AUDIO_PORTAUDIO_H

#include <QtCore/QObject>

#include "lmmsconfig.h"
#include "combobox_model.h"

class audioPortAudioSetupUtil : public QObject
{
	Q_OBJECT
public slots:
	void updateDevices( void );
	void updateChannels( void );
		
public:
	comboBoxModel m_backendModel;
	comboBoxModel m_deviceModel;
} ;


#ifdef LMMS_HAVE_PORTAUDIO

#include <portaudio.h>

#include "audio_device.h"


class comboBox;
class lcdSpinBox;


class audioPortAudio : public audioDevice
{
public:
	audioPortAudio( bool & _success_ful, mixer * _mixer );
	virtual ~audioPortAudio();

	inline static QString name( void )
	{
		return( QT_TRANSLATE_NOOP( "setupWidget",
					"PortAudio" ) );
	}


	int process_callback( const float *_inputBuffer,
		float * _outputBuffer,
		unsigned long _framesPerBuffer );


	class setupWidget : public audioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings( void );

	private:
		comboBox * m_backend;
		comboBox * m_device;
		lcdSpinBox * m_channels;
		audioPortAudioSetupUtil m_setupUtil;

	} ;

private:
	virtual void startProcessing( void );
	virtual void stopProcessing( void );
	virtual void applyQualitySettings( void );

	static int _process_callback( const void *_inputBuffer, void *_outputBuffer,
		unsigned long _framesPerBuffer,
		const PaStreamCallbackTimeInfo* _timeInfo,
		PaStreamCallbackFlags _statusFlags,
		void *arg );

	PaStream * m_paStream;
	PaStreamParameters m_outputParameters;
	PaStreamParameters m_inputParameters;
	bool m_wasPAInitError;
 
	surroundSampleFrame * m_outBuf;
	int m_outBuf_pos;
	int m_outBuf_size;

	bool m_stopped;
	QSemaphore m_stop_semaphore;

} ;

#endif

#endif
