#ifndef SINGLE_SOURCE_COMPILE

/* license */

#include "AudioSndio.h"

#ifdef LMMS_HAVE_SNDIO

#include <QtCore/QFileInfo>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include "endian_handling.h"
#include "LcdSpinBox.h"
#include "Mixer.h"
#include "Engine.h"
#include "gui_templates.h"
#include "templates.h"

#ifdef LMMS_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LMMS_HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "ConfigManager.h"



AudioSndio::AudioSndio( bool & _success_ful, Mixer * _mixer ) :
	AudioDevice( tLimit<ch_cnt_t>(
			     ConfigManager::inst()->value( "audiosndio", "channels" ).toInt(),
			     DEFAULT_CHANNELS, SURROUND_CHANNELS ), _mixer )
{
	_success_ful = FALSE;
	QString dev = ConfigManager::inst()->value( "audiosndio", "device" );

	if ( dev == "" )
	{
		m_hdl = sio_open( NULL, SIO_PLAY, 0 );
	}
	else
	{
		m_hdl = sio_open( dev.toAscii().data(), SIO_PLAY, 0 );
	}

	if( m_hdl == NULL )
	{
		printf( "sndio: failed opening audio-device\n" );
		return;
	}

	sio_initpar( &m_par );
	m_par.pchan = channels();
	m_par.bits = 16;
	m_par.le = SIO_LE_NATIVE;
	m_par.rate = sampleRate();
	m_par.round = mixer()->framesPerPeriod();
	m_par.appbufsz = m_par.round * 2;
	struct sio_par reqpar = m_par;

	if ( !sio_setpar( m_hdl, &m_par ) )
	{
		printf( "sndio: sio_setpar failed\n" );
		return;
	}

	if ( !sio_getpar( m_hdl, &m_par ) )
	{
		printf( "sndio: sio_getpar failed\n" );
		return;
	}

	if ( reqpar.pchan != m_par.pchan ||
			reqpar.bits != m_par.bits ||
			reqpar.le != m_par.le ||
			( abs( reqpar.rate - m_par.rate ) * 100 ) / reqpar.rate > 2 )
	{
		printf( "sndio: returned params not as requested\n" );
		return;
	}

	if ( !sio_start( m_hdl ) )
	{
		printf( "sndio: sio_start failed\n" );
		return;
	}

	_success_ful = TRUE;
}


AudioSndio::~AudioSndio()
{
	stopProcessing();

	if ( m_hdl != NULL )
	{
		sio_close( m_hdl );
		m_hdl = NULL;
	}
}


void AudioSndio::startProcessing( void )
{
	if( !isRunning() )
	{
		start( QThread::HighPriority );
	}
}


void AudioSndio::stopProcessing( void )
{
	stopProcessingThread( this );
}


void AudioSndio::applyQualitySettings( void )
{
	if( hqAudio() )
	{
		setSampleRate( Engine::mixer()->processingSampleRate() );
		/* change sample rate to sampleRate() */
	}

	AudioDevice::applyQualitySettings();
}


void AudioSndio::run( void )
{
	surroundSampleFrame * temp =
		new surroundSampleFrame[mixer()->framesPerPeriod()];
	int_sample_t * outbuf =
		new int_sample_t[mixer()->framesPerPeriod() * channels()];

	while( TRUE )
	{
		const fpp_t frames = getNextBuffer( temp );

		if( !frames )
		{
			break;
		}

		uint bytes = convertToS16( temp, frames,
					   mixer()->masterGain(), outbuf, FALSE );

		if( sio_write( m_hdl, outbuf, bytes ) != bytes )
		{
			break;
		}
	}

	delete[] temp;
	delete[] outbuf;
}


AudioSndio::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDeviceSetupWidget( AudioSndio::name(), _parent )
{
	m_device = new QLineEdit( "", this );
	m_device->setGeometry( 10, 20, 160, 20 );
	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<6>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );
	LcdSpinBoxModel * m = new LcdSpinBoxModel( /* this */ );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audiosndio",
			"channels" ).toInt() );
	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 180, 20 );
}


AudioSndio::setupWidget::~setupWidget()
{
}


void AudioSndio::setupWidget::saveSettings( void )
{
	ConfigManager::inst()->setValue( "audiosndio", "device",
					 m_device->text() );
	ConfigManager::inst()->setValue( "audiosndio", "channels",
					 QString::number( m_channels->value<int>() ) );
}


#endif	/* LMMS_HAVE_SNDIO */

#endif	/* SINGLE_SOURCE_COMPILE */
