/*
 * AudioPortAudio.cpp - device-class that performs PCM-output via PortAudio
 *
 * Copyright (c) 2008 Csaba Hruska <csaba.hruska/at/gmail.com>
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioPortAudio.h"

#ifndef LMMS_HAVE_PORTAUDIO
namespace lmms
{


void AudioPortAudioSetupUtil::updateBackends()
{
}

void AudioPortAudioSetupUtil::updateDevices()
{
}

void AudioPortAudioSetupUtil::updateChannels()
{
}


} // namespace lmms
#endif

#ifdef LMMS_HAVE_PORTAUDIO

#include <QFormLayout>

#include "ConfigManager.h"
#include "ComboBox.h"
#include "AudioEngine.h"

namespace lmms
{


AudioPortAudio::AudioPortAudio( bool & _success_ful, AudioEngine * _audioEngine ) :
	AudioDevice(std::clamp<ch_cnt_t>(
		ConfigManager::inst()->value("audioportaudio", "channels").toInt(),
		DEFAULT_CHANNELS,
		DEFAULT_CHANNELS), _audioEngine),
	m_paStream( nullptr ),
	m_wasPAInitError( false ),
	m_outBuf(new SampleFrame[audioEngine()->framesPerPeriod()]),
	m_outBufPos( 0 )
{
	_success_ful = false;

	m_outBufSize = audioEngine()->framesPerPeriod();

	PaError err = Pa_Initialize();
	
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		m_wasPAInitError = true;
		return;
	}

	if( Pa_GetDeviceCount() <= 0 )
	{
		return;
	}
	
	const QString& backend = ConfigManager::inst()->value( "audioportaudio", "backend" );
	const QString& device = ConfigManager::inst()->value( "audioportaudio", "device" );
		
	PaDeviceIndex inDevIdx = -1;
	PaDeviceIndex outDevIdx = -1;
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		const auto di = Pa_GetDeviceInfo(i);
		if( di->name == device &&
			Pa_GetHostApiInfo( di->hostApi )->name == backend )
		{
			inDevIdx = i;
			outDevIdx = i;
		}
	}

	if( inDevIdx < 0 )
	{
		inDevIdx = Pa_GetDefaultInputDevice();
	}
	
	if( outDevIdx < 0 )
	{
		outDevIdx = Pa_GetDefaultOutputDevice();
	}

	if( inDevIdx < 0 || outDevIdx < 0 )
	{
		return;
	}

	double inLatency = 0;//(double)audioEngine()->framesPerPeriod() / (double)sampleRate();
	double outLatency = 0;//(double)audioEngine()->framesPerPeriod() / (double)sampleRate();

	//inLatency = Pa_GetDeviceInfo( inDevIdx )->defaultLowInputLatency;
	//outLatency = Pa_GetDeviceInfo( outDevIdx )->defaultLowOutputLatency;
	const int samples = audioEngine()->framesPerPeriod();
	
	// Configure output parameters.
	m_outputParameters.device = outDevIdx;
	m_outputParameters.channelCount = channels();
	m_outputParameters.sampleFormat = paFloat32; // 32 bit floating point output
	m_outputParameters.suggestedLatency = outLatency;
	m_outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	// Configure input parameters.
	m_inputParameters.device = inDevIdx;
	m_inputParameters.channelCount = DEFAULT_CHANNELS;
	m_inputParameters.sampleFormat = paFloat32; // 32 bit floating point input
	m_inputParameters.suggestedLatency = inLatency;
	m_inputParameters.hostApiSpecificStreamInfo = nullptr;
	
	// Open an audio I/O stream. 
	err = Pa_OpenStream(
			&m_paStream,
			supportsCapture() ? &m_inputParameters : nullptr,	// The input parameter
			&m_outputParameters,	// The outputparameter
			sampleRate(),
			samples,
			paNoFlag,		// Don't use any flags
			_process_callback, 	// our callback function
			this );

	if( err == paInvalidDevice && sampleRate() < 48000 )
	{
		printf("Pa_OpenStream() failed with 44,1 KHz, trying again with 48 KHz\n");
		// some backends or drivers do not allow 32 bit floating point data
		// with a samplerate of 44100 Hz
		setSampleRate( 48000 );
		err = Pa_OpenStream(
				&m_paStream,
				supportsCapture() ? &m_inputParameters : nullptr,	// The input parameter
				&m_outputParameters,	// The outputparameter
				sampleRate(),
				samples,
				paNoFlag,		// Don't use any flags
				_process_callback, 	// our callback function
				this );
	}

	if( err != paNoError )
	{
		printf( "Couldn't open PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	printf( "Input device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( inDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( inDevIdx )->hostApi )->name );
	printf( "Output device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( outDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( outDevIdx )->hostApi )->name );

	// TODO: debug AudioEngine::pushInputFrames()
	//m_supportsCapture = true;

	_success_ful = true;
}




AudioPortAudio::~AudioPortAudio()
{
	stopProcessing();

	if( !m_wasPAInitError )
	{
		Pa_Terminate();
	}
	delete[] m_outBuf;
}




void AudioPortAudio::startProcessing()
{
	m_stopped = false;
	PaError err = Pa_StartStream( m_paStream );
	
	if( err != paNoError )
	{
		m_stopped = true;
		printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
	}
}




void AudioPortAudio::stopProcessing()
{
	if( m_paStream && Pa_IsStreamActive( m_paStream ) )
	{
		m_stopped = true;
		PaError err = Pa_StopStream( m_paStream );
	
		if( err != paNoError )
		{
			printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
		}
	}
}


int AudioPortAudio::process_callback(const float* _inputBuffer, float* _outputBuffer, f_cnt_t _framesPerBuffer)
{
	if( supportsCapture() )
	{
		audioEngine()->pushInputFrames( (SampleFrame*)_inputBuffer, _framesPerBuffer );
	}

	if( m_stopped )
	{
		memset( _outputBuffer, 0, _framesPerBuffer *
			channels() * sizeof(float) );
		return paComplete;
	}

	while( _framesPerBuffer )
	{
		if( m_outBufPos == 0 )
		{
			// frames depend on the sample rate
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				m_stopped = true;
				memset( _outputBuffer, 0, _framesPerBuffer *
					channels() * sizeof(float) );
				return paComplete;
			}
			m_outBufSize = frames;
		}
		const auto min_len = std::min(_framesPerBuffer, m_outBufSize - m_outBufPos);

		for( fpp_t frame = 0; frame < min_len; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				(_outputBuffer + frame * channels())[chnl] = AudioEngine::clip(m_outBuf[frame][chnl]);
			}
		}

		_outputBuffer += min_len * channels();
		_framesPerBuffer -= min_len;
		m_outBufPos += min_len;
		m_outBufPos %= m_outBufSize;
	}

	return paContinue;
}



int AudioPortAudio::_process_callback(
	const void *_inputBuffer,
	void * _outputBuffer,
	unsigned long _framesPerBuffer,
	const PaStreamCallbackTimeInfo * _timeInfo,
	PaStreamCallbackFlags _statusFlags,
	void * _arg )
{
	Q_UNUSED(_timeInfo);
	Q_UNUSED(_statusFlags);

	auto _this = static_cast<AudioPortAudio*>(_arg);
	return _this->process_callback( (const float*)_inputBuffer,
		(float*)_outputBuffer, _framesPerBuffer );
}




void AudioPortAudioSetupUtil::updateBackends()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		const auto hi = Pa_GetHostApiInfo(i);
		m_backendModel.addItem( hi->name );
	}

	Pa_Terminate();
}




void AudioPortAudioSetupUtil::updateDevices()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	// get active backend 
	const QString& backend = m_backendModel.currentText();
	int hostApi = 0;
	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		const auto hi = Pa_GetHostApiInfo(i);
		if( backend == hi->name )
		{
			hostApi = i;
			break;
		}
	}

	// get devices for selected backend
	m_deviceModel.clear();
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		const auto di = Pa_GetDeviceInfo(i);
		if( di->hostApi == hostApi )
		{
			m_deviceModel.addItem( di->name );
		}
	}
	Pa_Terminate();
}




void AudioPortAudioSetupUtil::updateChannels()
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}
	// get active backend 
	Pa_Terminate();
}




AudioPortAudio::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioPortAudio::name(), _parent )
{
	using gui::ComboBox;

	QFormLayout * form = new QFormLayout(this);

	m_backend = new ComboBox( this, "BACKEND" );
	form->addRow(tr("Backend"), m_backend);

	m_device = new ComboBox( this, "DEVICE" );
	form->addRow(tr("Device"), m_device);
	
/*	LcdSpinBoxModel * m = new LcdSpinBoxModel(  );
	m->setRange( DEFAULT_CHANNELS, DEFAULT_CHANNELS );
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audioportaudio",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "Channels" ) );
	m_channels->move( 308, 20 );*/

	connect( &m_setupUtil.m_backendModel, SIGNAL(dataChanged()),
			&m_setupUtil, SLOT(updateDevices()));
			
	connect( &m_setupUtil.m_deviceModel, SIGNAL(dataChanged()),
			&m_setupUtil, SLOT(updateChannels()));
			
	m_backend->setModel( &m_setupUtil.m_backendModel );
	m_device->setModel( &m_setupUtil.m_deviceModel );
}




AudioPortAudio::setupWidget::~setupWidget()
{
	disconnect( &m_setupUtil.m_backendModel, SIGNAL(dataChanged()),
			&m_setupUtil, SLOT(updateDevices()));
			
	disconnect( &m_setupUtil.m_deviceModel, SIGNAL(dataChanged()),
			&m_setupUtil, SLOT(updateChannels()));
}




void AudioPortAudio::setupWidget::saveSettings()
{

	ConfigManager::inst()->setValue( "audioportaudio", "backend",
							m_setupUtil.m_backendModel.currentText() );
	ConfigManager::inst()->setValue( "audioportaudio", "device",
							m_setupUtil.m_deviceModel.currentText() );
/*	ConfigManager::inst()->setValue( "audioportaudio", "channels",
				QString::number( m_channels->value<int>() ) );*/

}




void AudioPortAudio::setupWidget::show()
{
	if( m_setupUtil.m_backendModel.size() == 0 )
	{
		// populate the backend model the first time we are shown
		m_setupUtil.updateBackends();

		const QString& backend = ConfigManager::inst()->value(
			"audioportaudio", "backend" );
		const QString& device = ConfigManager::inst()->value(
			"audioportaudio", "device" );
		
		int i = std::max(0, m_setupUtil.m_backendModel.findText(backend));
		m_setupUtil.m_backendModel.setValue( i );
		
		m_setupUtil.updateDevices();
		
		i = std::max(0, m_setupUtil.m_deviceModel.findText(device));
		m_setupUtil.m_deviceModel.setValue( i );
	}

	AudioDeviceSetupWidget::show();
}

} // namespace lmms


#endif // LMMS_HAVE_PORTAUDIO



