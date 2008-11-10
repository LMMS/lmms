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

#if defined paNeverDropInput || defined paNonInterleaved
#	define PORTAUDIO_V19
#else
#	define PORTAUDIO_V18
#endif


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

#ifdef PORTAUDIO_V19
	static int _process_callback( const void *_inputBuffer, void * _outputBuffer,
		unsigned long _framesPerBuffer,
		const PaStreamCallbackTimeInfo * _timeInfo,
		PaStreamCallbackFlags _statusFlags,
		void *arg );

#else

#define paContinue 0
#define paComplete 1
#define Pa_GetDeviceCount Pa_CountDevices
#define Pa_GetDefaultInputDevice Pa_GetDefaultInputDeviceID
#define Pa_GetDefaultOutputDevice Pa_GetDefaultOutputDeviceID
#define Pa_IsStreamActive Pa_StreamActive

	static int _process_callback( void * _inputBuffer, void * _outputBuffer,
		unsigned long _framesPerBuffer, PaTimestamp _outTime, void * _arg );


	typedef double PaTime;
	typedef PaDeviceID PaDeviceIndex;
	
	typedef struct PaStreamParameters
	{
		PaDeviceIndex device;
		int channelCount;
		PaSampleFormat sampleFormat;
		PaTime suggestedLatency;
		void *hostApiSpecificStreamInfo;

	} PaStreamParameters;
#endif

	PaStream * m_paStream;
	PaStreamParameters m_outputParameters;
	PaStreamParameters m_inputParameters;

	bool m_wasPAInitError;
 
	sampleFrameA * m_outBuf;
	int m_outBufPos;
	int m_outBufSize;

	bool m_stopped;
	QSemaphore m_stopSemaphore;

} ;

#endif

#endif
