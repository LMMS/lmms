/*
 * AudioOss.cpp - device-class that implements OSS-PCM-output
 *
 * Copyright (c) 2004-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "AudioOss.h"

#ifdef LMMS_HAVE_OSS

#include <QtCore/QFileInfo>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "endian_handling.h"
#include "LcdSpinBox.h"
#include "engine.h"
#include "gui_templates.h"
#include "templates.h"

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LMMS_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef LMMS_HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef LMMS_HAVE_SYS_SOUNDCARD_H
// This is recommended by OSS
#include <sys/soundcard.h>
#elif defined(LMMS_HAVE_SOUNDCARD_H)
// This is installed on some systems
#include <soundcard.h>
#endif


#include "config_mgr.h"


#ifndef _PATH_DEV_DSP
#ifdef __OpenBSD__
#define _PATH_DEV_DSP  "/dev/audio"
#else
#define _PATH_DEV_DSP  "/dev/dsp"
#endif
#endif



AudioOss::AudioOss( bool & _success_ful, Mixer*  _mixer ) :
	AudioDevice( tLimit<ch_cnt_t>(
		configManager::inst()->value( "audiooss", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_convertEndian( false )
{
	_success_ful = false;

	m_audioFD = open( probeDevice().toAscii().constData(), O_WRONLY, 0 );

	if( m_audioFD == -1 )
	{
		printf( "AudioOss: failed opening audio-device\n" );
		return;
	}


	// Make the file descriptor use blocking writes with fcntl()
	if ( fcntl( m_audioFD, F_SETFL, fcntl( m_audioFD, F_GETFL ) &
							~O_NONBLOCK ) < 0 )
	{
		printf( "could not set audio blocking mode\n" );
		return;
	}

	// set FD_CLOEXEC flag for file descriptor so forked processes
	// do not inherit it
	fcntl( m_audioFD, F_SETFD, fcntl( m_audioFD, F_GETFD ) | FD_CLOEXEC );

	int frag_spec;
	for( frag_spec = 0; static_cast<int>( 0x01 << frag_spec ) <
		mixer()->framesPerPeriod() * channels() *
							BYTES_PER_INT_SAMPLE;
		++frag_spec )
	{
	}

	frag_spec |= 0x00020000;	// two fragments, for low latency

	if ( ioctl( m_audioFD, SNDCTL_DSP_SETFRAGMENT, &frag_spec ) < 0 )
	{
		perror( "SNDCTL_DSP_SETFRAGMENT" );
		printf( "Warning: Couldn't set audio fragment size\n" );
	}

	unsigned int value;
	// Get a list of supported hardware formats
	if ( ioctl( m_audioFD, SNDCTL_DSP_GETFMTS, &value ) < 0 )
	{
		perror( "SNDCTL_DSP_GETFMTS" );
		printf( "Couldn't get audio format list\n" );
		return;
	}

	// Set the audio format
	if( value & AFMT_S16_LE )
	{
		value = AFMT_S16_LE;
	}
	else if( value & AFMT_S16_BE )
	{
		value = AFMT_S16_BE;
	}
	else
	{
		printf(" Soundcard doesn't support signed 16-bit-data\n");
	}
	if ( ioctl( m_audioFD, SNDCTL_DSP_SETFMT, &value ) < 0 )
	{
		perror( "SNDCTL_DSP_SETFMT" );
		printf( "Couldn't set audio format\n" );
		return;
	}
	if( ( isLittleEndian() && ( value == AFMT_S16_BE ) ) ||
			( !isLittleEndian() && ( value == AFMT_S16_LE ) ) )
	{
		m_convertEndian = true;
	}

	// Set the number of channels of output
	value = channels();
	if ( ioctl( m_audioFD, SNDCTL_DSP_CHANNELS, &value ) < 0 )
	{
		perror( "SNDCTL_DSP_CHANNELS" );
		printf( "Cannot set the number of channels\n" );
		return;
	}
	if( value != channels() )
	{
		printf( "Couldn't set number of channels\n" );
		return;
	}

	// Set the DSP frequency
	value = sampleRate();
	if ( ioctl( m_audioFD, SNDCTL_DSP_SPEED, &value ) < 0 )
	{
		perror( "SNDCTL_DSP_SPEED" );
		printf( "Couldn't set audio frequency\n" );
		return;
	}
	if( value != sampleRate() )
	{
		value = mixer()->baseSampleRate();
		if ( ioctl( m_audioFD, SNDCTL_DSP_SPEED, &value ) < 0 )
		{
			perror( "SNDCTL_DSP_SPEED" );
			printf( "Couldn't set audio frequency\n" );
			return;
		}
		setSampleRate( value );
	}

	_success_ful = true;
}




AudioOss::~AudioOss()
{
	stopProcessing();
	close( m_audioFD );
}




QString AudioOss::probeDevice()
{
	QString dev = configManager::inst()->value( "AudioOss", "Device" );
	if( dev.isEmpty() )
	{
		char * adev = getenv( "AUDIODEV" );	// Is there a standard
							// variable name?
		if( adev != NULL )
		{
			dev = adev;
		}
		else
		{
			dev = _PATH_DEV_DSP;		// default device
		}
	}

	// if the first open fails, look for other devices
	if( QFileInfo( dev ).isWritable() == false )
	{
		int instance = -1;
		while( 1 )
		{
			dev = _PATH_DEV_DSP + QString::number( ++instance );
			if( !QFileInfo( dev ).exists() )
			{
				dev = _PATH_DEV_DSP;
				break;
			}
			if( QFileInfo( dev ).isWritable() )
			{
				break;
			}
		}
	}
	return dev;
}




void AudioOss::startProcessing()
{
	if( !isRunning() )
	{
		start( QThread::HighPriority );
	}
}




void AudioOss::stopProcessing()
{
	if( isRunning() )
	{
		wait( 1000 );
		terminate();
	}
}




void AudioOss::applyQualitySettings()
{
	if( hqAudio() )
	{
		setSampleRate( engine::mixer()->processingSampleRate() );

		unsigned int value = sampleRate();
		if ( ioctl( m_audioFD, SNDCTL_DSP_SPEED, &value ) < 0 )
		{
			perror( "SNDCTL_DSP_SPEED" );
			printf( "Couldn't set audio frequency\n" );
			return;
		}
		if( value != sampleRate() )
		{
			value = mixer()->baseSampleRate();
			if ( ioctl( m_audioFD, SNDCTL_DSP_SPEED, &value ) < 0 )
			{
				perror( "SNDCTL_DSP_SPEED" );
				printf( "Couldn't set audio frequency\n" );
				return;
			}
			setSampleRate( value );
		}
	}

	AudioDevice::applyQualitySettings();
}




void AudioOss::run()
{
	surroundSampleFrame * temp =
		new surroundSampleFrame[mixer()->framesPerPeriod()];
	int_sample_t * outbuf =
			new int_sample_t[mixer()->framesPerPeriod() *
								channels()];

	while( true )
	{
		const fpp_t frames = getNextBuffer( temp );
		if( !frames )
		{
			break;
		}

		int bytes = convertToS16( temp, frames,
				mixer()->masterGain(), outbuf,
							m_convertEndian );
		if( write( m_audioFD, outbuf, bytes ) != bytes )
		{
			break;
		}
	}

	delete[] temp;
	delete[] outbuf;
}




AudioOss::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDevice::setupWidget( AudioOss::name(), _parent )
{
	m_device = new QLineEdit( probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	LcdSpinBoxModel * m = new LcdSpinBoxModel( /* this */ );	
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( configManager::inst()->value( "audiooss",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 180, 20 );

}




AudioOss::setupWidget::~setupWidget()
{
	delete m_channels->model();
}




void AudioOss::setupWidget::saveSettings()
{
	configManager::inst()->setValue( "audiooss", "device",
							m_device->text() );
	configManager::inst()->setValue( "audiooss", "channels",
				QString::number( m_channels->value<int>() ) );
}


#endif

