#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_portaudio.cpp - device-class that performs PCM-output via PortAudio
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



#include "audio_portaudio.h"


#ifndef LMMS_HAVE_PORTAUDIO
void audioPortAudioSetupUtil::updateDevices( void )
{
}

void audioPortAudioSetupUtil::updateChannels( void )
{
}
#endif

#ifdef LMMS_HAVE_PORTAUDIO

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "engine.h"
#include "debug.h"
#include "config_mgr.h"
#include "gui_templates.h"
#include "templates.h"
#include "combobox.h"
#include "lcd_spinbox.h"




audioPortAudio::audioPortAudio( bool & _success_ful, mixer * _mixer ) :
	audioDevice( tLimit<ch_cnt_t>(
		configManager::inst()->value( "audioportaudio", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_outBuf( new surroundSampleFrame[getMixer()->framesPerPeriod()] ),
	m_outBuf_pos( 0 ),
	m_stop_semaphore( 1 ),
	m_wasPAInitError( FALSE )
{
	_success_ful = FALSE;

	m_outBuf_size = getMixer()->framesPerPeriod();

	PaError err = Pa_Initialize();
	
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		m_wasPAInitError = TRUE;
		return;
	}

	if( Pa_GetDeviceCount() <= 0 )
	{
		return;
	}
	
	const QString& backend = configManager::inst()->value( "audioportaudio",
		"backend" );
	const QString& device = configManager::inst()->value( "audioportaudio",
		"device" );
		
	PaDeviceIndex inDevIdx = -1;
	PaDeviceIndex outDevIdx = -1;
	const PaDeviceInfo * di;
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		di = Pa_GetDeviceInfo( i );
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

	double inLatency = (double)getMixer()->framesPerPeriod() / (double)sampleRate();
	double outLatency = (double)getMixer()->framesPerPeriod() / (double)sampleRate();

	// FIXME: remove this
	inLatency = Pa_GetDeviceInfo( inDevIdx )->defaultLowInputLatency;
	outLatency = Pa_GetDeviceInfo( outDevIdx )->defaultLowOutputLatency;
	int samples = qMax( 1024, (int)getMixer()->framesPerPeriod() );
	
	// Configure output parameters.
	m_outputParameters.device = outDevIdx;
	m_outputParameters.channelCount = channels();
	m_outputParameters.sampleFormat = paFloat32; // 32 bit floating point output
	m_outputParameters.suggestedLatency = inLatency;
	m_outputParameters.hostApiSpecificStreamInfo = NULL;
	
	// Configure input parameters.
	m_inputParameters.device = inDevIdx;
	m_inputParameters.channelCount = DEFAULT_CHANNELS;
	m_inputParameters.sampleFormat = paFloat32; // 32 bit floating point input
	m_inputParameters.suggestedLatency = outLatency;
	m_inputParameters.hostApiSpecificStreamInfo = NULL;
	
	// Open an audio I/O stream. 
	err = Pa_OpenStream(
			&m_paStream,
			&m_inputParameters,	// The input parameter
			&m_outputParameters,	// The outputparameter
			sampleRate(),
			samples,
			paNoFlag,		// Don't use any flags
			_process_callback, 	// our callback function
			this );
	
	if( err != paNoError )
	{
		printf( "Couldn't open PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	printf( "Input device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( inDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( inDevIdx )->hostApi )->name );
	printf( "Output device: '%s' backend: '%s'\n", Pa_GetDeviceInfo( outDevIdx )->name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( outDevIdx )->hostApi )->name );

	m_stop_semaphore.acquire();

	m_supportsCapture = TRUE;
	_success_ful = TRUE;
}




audioPortAudio::~audioPortAudio()
{
	stopProcessing();
	m_stop_semaphore.release();

	if( !m_wasPAInitError )
	{
		Pa_Terminate();
	}
	delete[] m_outBuf;
}




void audioPortAudio::startProcessing( void )
{
	PaError err = Pa_StartStream( m_paStream );
	
	if( err != paNoError )
	{
		printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
		return;
	}

	m_stopped = FALSE;
}




void audioPortAudio::stopProcessing( void )
{

	if( Pa_IsStreamActive( m_paStream ) )
	{
		m_stop_semaphore.acquire();
		
		PaError err = Pa_CloseStream( m_paStream );
	
		if( err != paNoError )
		{
			printf( "PortAudio error: %s\n", Pa_GetErrorText( err ) );
		}
	}
}




void audioPortAudio::applyQualitySettings( void )
{
	if( hqAudio() )
	{

		setSampleRate( engine::getMixer()->processingSampleRate() );
		int samples = qMax( 1024, (int)getMixer()->framesPerPeriod() );

		PaError err = Pa_OpenStream(
				&m_paStream,
				&m_inputParameters,	// The input parameter
				&m_outputParameters,	// The outputparameter
				sampleRate(),
				samples,
				paNoFlag,		// Don't use any flags
				_process_callback, 	// our callback function
				this );
	
		if( err != paNoError )
		{
			printf( "Couldn't open PortAudio: %s\n", Pa_GetErrorText( err ) );
			return;
		}
	}

	audioDevice::applyQualitySettings();
}

int audioPortAudio::process_callback(
	const float *_inputBuffer,
	float * _outputBuffer,
	unsigned long _framesPerBuffer )
{
	getMixer()->pushInputFrames( (sampleFrame*)_inputBuffer, _framesPerBuffer );

	if( m_stopped )
	{
		memset( _outputBuffer, 0, _framesPerBuffer *
			channels() * sizeof(float) );
		return 0;
	}

	while( _framesPerBuffer )
	{
		if( m_outBuf_pos == 0 )
		{
			// frames depend on the sample rate
			const fpp_t frames = getNextBuffer( m_outBuf );
			if( !frames )
			{
				m_stopped = TRUE;
				m_stop_semaphore.release();
				memset( _outputBuffer, 0, _framesPerBuffer *
					channels() * sizeof(float) );
				return 0;
			}
			m_outBuf_size = frames;
		}
		const int min_len = tMin( (int)_framesPerBuffer,
			m_outBuf_size - m_outBuf_pos );

		float master_gain = getMixer()->masterGain();

		for( fpp_t frame = 0; frame < min_len; ++frame )
		{
			for( ch_cnt_t chnl = 0; chnl < channels(); ++chnl )
			{
				( _outputBuffer + frame * channels() )[chnl] =
						mixer::clip( m_outBuf[frame][chnl] *
						master_gain );
			}
		}

		_outputBuffer += min_len * channels();
		_framesPerBuffer -= min_len;
		m_outBuf_pos += min_len;
		m_outBuf_pos %= m_outBuf_size;
	}

}



int audioPortAudio::_process_callback(
	const void *_inputBuffer,
	void * _outputBuffer,
	unsigned long _framesPerBuffer,
	const PaStreamCallbackTimeInfo * _timeInfo,
	PaStreamCallbackFlags _statusFlags,
	void * _arg )
{
	Q_UNUSED(_timeInfo);
	Q_UNUSED(_statusFlags);
	
	audioPortAudio * _this  = static_cast<audioPortAudio *> (_arg);
	_this->process_callback( (const float*)_inputBuffer,
		(float*)_outputBuffer, _framesPerBuffer );
	
	return 0;
}




void audioPortAudioSetupUtil::updateDevices( void )
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}
	// get active backend 
	const QString& backend = m_backendModel.currentText();
	int hostApi = 0;
	const PaHostApiInfo * hi;
	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		hi = Pa_GetHostApiInfo( i );
		if( backend == hi->name )
		{
			hostApi = i;
			break;
		}
	}

	// get devices for selected backend
	m_deviceModel.clear();
	const PaDeviceInfo * di;
	for( int i = 0; i < Pa_GetDeviceCount(); ++i )
	{
		di = Pa_GetDeviceInfo( i );
		if( di->hostApi == hostApi )
		{
			m_deviceModel.addItem( di->name );
		}
	}
	Pa_Terminate();
}




void audioPortAudioSetupUtil::updateChannels( void )
{
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}
	// get active backend 
	Pa_Terminate();
}




audioPortAudio::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioPortAudio::name(), _parent )
{
	m_backend = new comboBox( this, "BACKEND" );
	m_backend->setGeometry( 52, 15, 120, 20 );

	QLabel * backend_lbl = new QLabel( tr( "BACKEND" ), this );
	backend_lbl->setFont( pointSize<6>( backend_lbl->font() ) );
	backend_lbl->move( 10, 17 );
	
	m_device = new comboBox( this, "DEVICE" );
	m_device->setGeometry( 52, 35, 250, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->move( 10, 37 );
	
	lcdSpinBoxModel * m = new lcdSpinBoxModel(  );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( configManager::inst()->value( "audioportaudio",
							"channels" ).toInt() );

	m_channels = new lcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 308, 20 );

	// Setup models
	PaError err = Pa_Initialize();
	if( err != paNoError ) {
		printf( "Couldn't initialize PortAudio: %s\n", Pa_GetErrorText( err ) );
		return;
	}
	
	// todo: setup backend model
	const PaHostApiInfo * hi;
	for( int i = 0; i < Pa_GetHostApiCount(); ++i )
	{
		hi = Pa_GetHostApiInfo( i );
		m_setupUtil.m_backendModel.addItem( hi->name );
	}

	Pa_Terminate();


	const QString& backend = configManager::inst()->value( "audioportaudio",
		"backend" );
	const QString& device = configManager::inst()->value( "audioportaudio",
		"device" );
	
	int i = tMax( 0, m_setupUtil.m_backendModel.findText( backend ) );
	m_setupUtil.m_backendModel.setValue( i );
	
	m_setupUtil.updateDevices();
	
	i = tMax( 0, m_setupUtil.m_deviceModel.findText( device ) );
	m_setupUtil.m_deviceModel.setValue( i );

	connect( &m_setupUtil.m_backendModel, SIGNAL( dataChanged() ),
			&m_setupUtil, SLOT( updateDevices() ) );
			
	connect( &m_setupUtil.m_deviceModel, SIGNAL( dataChanged() ),
			&m_setupUtil, SLOT( updateChannels() ) );
			
	m_backend->setModel( &m_setupUtil.m_backendModel );
	m_device->setModel( &m_setupUtil.m_deviceModel );
}




audioPortAudio::setupWidget::~setupWidget()
{
	disconnect( &m_setupUtil.m_backendModel, SIGNAL( dataChanged() ),
			&m_setupUtil, SLOT( updateDevices() ) );
			
	disconnect( &m_setupUtil.m_deviceModel, SIGNAL( dataChanged() ),
			&m_setupUtil, SLOT( updateChannels() ) );
}




void audioPortAudio::setupWidget::saveSettings( void )
{

	configManager::inst()->setValue( "audioportaudio", "backend",
							m_setupUtil.m_backendModel.currentText() );
	configManager::inst()->setValue( "audioportaudio", "device",
							m_setupUtil.m_deviceModel.currentText() );
	configManager::inst()->setValue( "audioportaudio", "channels",
				QString::number( m_channels->value<int>() ) );

}


#endif

#include "moc_audio_portaudio.cxx"

#endif
