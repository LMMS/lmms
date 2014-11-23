/*
 * AudioPortAudio.h - device-class that performs PCM-output via PortAudio
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
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

#ifndef _AUDIO_PORTAUDIO_H
#define _AUDIO_PORTAUDIO_H

#include <QtCore/QObject>

#include "lmmsconfig.h"
#include "ComboBoxModel.h"

class AudioPortAudioSetupUtil : public QObject
{
	Q_OBJECT
public slots:
	void updateDevices();
	void updateChannels();
		
public:
	ComboBoxModel m_backendModel;
	ComboBoxModel m_deviceModel;
} ;


#ifdef LMMS_HAVE_PORTAUDIO

#include <portaudio.h>

#include "AudioDevice.h"

#if defined paNeverDropInput || defined paNonInterleaved
#	define PORTAUDIO_V19
#else
#	define PORTAUDIO_V18
#endif


class comboBox;
class LcdSpinBox;


class AudioPortAudio : public AudioDevice
{
public:
	AudioPortAudio( bool & _success_ful, Mixer* mixer );
	virtual ~AudioPortAudio();

	inline static QString name()
	{
		return QT_TRANSLATE_NOOP( "setupWidget", "PortAudio" );
	}


	int process_callback( const float *_inputBuffer,
		float * _outputBuffer,
		unsigned long _framesPerBuffer );


	class setupWidget : public AudioDevice::setupWidget
	{
	public:
		setupWidget( QWidget * _parent );
		virtual ~setupWidget();

		virtual void saveSettings();

	private:
		comboBox * m_backend;
		comboBox * m_device;
		AudioPortAudioSetupUtil m_setupUtil;

	} ;

private:
	virtual void startProcessing();
	virtual void stopProcessing();
	virtual void applyQualitySettings();

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
 
	surroundSampleFrame * m_outBuf;
	int m_outBufPos;
	int m_outBufSize;

	bool m_stopped;
	QSemaphore m_stopSemaphore;

} ;

#endif

#endif
