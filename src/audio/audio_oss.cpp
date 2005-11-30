/*
 * audio_oss.cpp - device-class that implements OSS-PCM-output
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "audio_oss.h"


#ifdef OSS_SUPPORT

#include "qt3support.h"

#ifdef QT4

#include <QFileInfo>
#include <QLineEdit>
#include <QLabel>

#else

#include <qfileinfo.h>
#include <qlineedit.h>
#include <qlabel.h>

#endif


#include "buffer_allocator.h"
#include "endian_handling.h"
#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "templates.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
// This is recommended by OSS
#include <sys/soundcard.h>
#elif HAVE_SOUNDCARD_H
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



audioOSS::audioOSS( Uint32 _sample_rate, bool & _success_ful ) :
	audioDevice( _sample_rate, tLimit<int>( configManager::inst()->value(
					"audiooss", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ) ),
	m_convertEndian( FALSE )
{
	_success_ful = FALSE;

	m_audioFD = open(
#ifdef QT4
				probeDevice().toAscii().constData(),
#else
				probeDevice().ascii(),
#endif
				O_WRONLY, 0 );

	if( m_audioFD == -1 )
	{
		printf( "audioOSS: failed opening audio-device\n" );
		return;
	}


	// Make the file descriptor use blocking writes with fcntl()
	if ( fcntl( m_audioFD, F_SETFL, fcntl( m_audioFD, F_GETFL ) &
							~O_NONBLOCK ) < 0 )
	{
		printf( "could not set audio blocking mode\n" );
		return;
	}

	int frag_spec;
	for( frag_spec = 0; static_cast<unsigned int>( 0x01 << frag_spec ) <
		mixer::inst()->framesPerAudioBuffer() * channels() *
							BYTES_PER_OUTPUT_SAMPLE;
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
		m_convertEndian = TRUE;
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
		//printf( "Soundcard uses different sample-rate than LMMS "
		//						"does!\n" );
		int q = 0;
		if( sampleRate() == SAMPLE_RATES[1] )
		{
			q = 1;
		}
		if( sampleRate() == 44100 || sampleRate() == 88200 )
		{
			SAMPLE_RATES[0] = 48000;
			SAMPLE_RATES[1] = 96000;
		}
		else
		{
			SAMPLE_RATES[0] = 44100;
			SAMPLE_RATES[1] = 82000;
		}
		setSampleRate( SAMPLE_RATES[q] );
		value = sampleRate();
		if ( ioctl( m_audioFD, SNDCTL_DSP_SPEED, &value ) < 0 )
		{
			perror( "SNDCTL_DSP_SPEED" );
			printf( "Couldn't set audio frequency\n" );
			return;
		}
	}

	_success_ful = TRUE;
}




audioOSS::~audioOSS()
{
	close( m_audioFD );
}




QString audioOSS::probeDevice( void )
{
	QString dev = configManager::inst()->value( "audiooss", "device" );
	if( dev == "" )
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
	if ( QFileInfo( dev ).isWritable() == FALSE )
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
	return( dev );
}




void audioOSS::writeBufferToDev( surroundSampleFrame * _ab, Uint32 _frames,
							float _master_gain )
{
	outputSampleType * outbuf = bufferAllocator::alloc<outputSampleType>(
							_frames * channels() );
	int bytes = convertToS16( _ab, _frames, _master_gain, outbuf,
							m_convertEndian );
	write( m_audioFD, outbuf, bytes );

	bufferAllocator::free( outbuf );
}




audioOSS::setupWidget::setupWidget( QWidget * _parent ) :
	audioDevice::setupWidget( audioOSS::name(), _parent )
{
	m_device = new QLineEdit( probeDevice(), this );
	m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	m_channels = new lcdSpinBox( DEFAULT_CHANNELS, SURROUND_CHANNELS, 1,
									this );
	m_channels->setStep( 2 );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->setValue( configManager::inst()->value( "audiooss",
							"channels" ).toInt() );
	m_channels->move( 180, 20 );

}




audioOSS::setupWidget::~setupWidget()
{

}




void audioOSS::setupWidget::saveSettings( void )
{
	configManager::inst()->setValue( "audiooss", "device",
							m_device->text() );
	configManager::inst()->setValue( "audiooss", "channels",
				QString::number( m_channels->value() ) );
}


#endif

