/*
 * AudioAlsa.cpp - device-class which implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

#include "AudioAlsa.h"

#ifdef LMMS_HAVE_ALSA

#include "endian_handling.h"
#include "ConfigManager.h"
#include "Engine.h"
#include "LcdSpinBox.h"
#include "gui_templates.h"
#include "templates.h"

#include <iostream>
#include <vector>


AudioAlsa::AudioAlsa( bool & _success_ful, Mixer*  _mixer ) :
	AudioDevice( tLimit<ch_cnt_t>(
		ConfigManager::inst()->value( "audioalsa", "channels" ).toInt(),
					DEFAULT_CHANNELS, SURROUND_CHANNELS ),
								_mixer ),
	m_handle( NULL ),
	m_hwParams( NULL ),
	m_swParams( NULL ),
	m_convertEndian( false )
{
	_success_ful = false;

	int err;

	if( ( err = snd_pcm_open( &m_handle,
					probeDevice().toLatin1().constData(),
						SND_PCM_STREAM_PLAYBACK,
						0 ) ) < 0 )
	{
		printf( "Playback open error: %s\n", snd_strerror( err ) );
		return;
	}

	snd_pcm_hw_params_malloc( &m_hwParams );
	snd_pcm_sw_params_malloc( &m_swParams );

	if( ( err = setHWParams( channels(),
					SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 )
	{
		printf( "Setting of hwparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}
	if( ( err = setSWParams() ) < 0 )
	{
		printf( "Setting of swparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}

	// set FD_CLOEXEC flag for all file descriptors so forked processes
	// do not inherit them
	struct pollfd * ufds;
	int count = snd_pcm_poll_descriptors_count( m_handle );
	ufds = new pollfd[count];
	snd_pcm_poll_descriptors( m_handle, ufds, count );
	for( int i = 0; i < qMax( 3, count ); ++i )
	{
		const int fd = ( i >= count ) ? ufds[0].fd+i : ufds[i].fd;
		int oldflags = fcntl( fd, F_GETFD, 0 );
		if( oldflags < 0 )
			continue;
		oldflags |= FD_CLOEXEC;
		fcntl( fd, F_SETFD, oldflags );
	}
	delete[] ufds;
	_success_ful = true;
}




AudioAlsa::~AudioAlsa()
{
	stopProcessing();
	if( m_handle != NULL )
	{
		snd_pcm_close( m_handle );
	}

	if( m_hwParams != NULL )
	{
		snd_pcm_hw_params_free( m_hwParams );
	}

	if( m_swParams != NULL )
	{
		snd_pcm_sw_params_free( m_swParams );
	}
}




QString AudioAlsa::probeDevice()
{
	QString dev = ConfigManager::inst()->value( "audioalsa", "device" );
	if( dev == "" )
	{
		if( getenv( "AUDIODEV" ) != NULL )
		{
			return getenv( "AUDIODEV" );
		}
		return "default";
	}
	return dev;
}




int AudioAlsa::handleError( int _err )
{
	if( _err == -EPIPE )
	{
		// under-run
		_err = snd_pcm_prepare( m_handle );
		if( _err < 0 )
			printf( "Can't recover from underrun, prepare "
					"failed: %s\n", snd_strerror( _err ) );
		return ( 0 );
	}
#ifdef ESTRPIPE
	else if( _err == -ESTRPIPE )
	{
		while( ( _err = snd_pcm_resume( m_handle ) ) == -EAGAIN )
		{
			sleep( 1 );	// wait until the suspend flag
					// is released
		}

		if( _err < 0 )
		{
			_err = snd_pcm_prepare( m_handle );
			if( _err < 0 )
				printf( "Can't recover from suspend, prepare "
					"failed: %s\n", snd_strerror( _err ) );
		}
		return ( 0 );
	}
#endif
	return _err;
}




void AudioAlsa::startProcessing()
{
	if( !isRunning() )
	{
		start( QThread::HighPriority );
	}
}




void AudioAlsa::stopProcessing()
{
	if( isRunning() )
	{
		wait( 1000 );
		terminate();
	}
}




void AudioAlsa::applyQualitySettings()
{
	if( hqAudio() )
	{
		setSampleRate( Engine::mixer()->processingSampleRate() );

		if( m_handle != NULL )
		{
			snd_pcm_close( m_handle );
		}

		int err;
		if( ( err = snd_pcm_open( &m_handle,
					probeDevice().toLatin1().constData(),
						SND_PCM_STREAM_PLAYBACK,
								0 ) ) < 0 )
		{
			printf( "Playback open error: %s\n",
							snd_strerror( err ) );
			return;
		}

		if( ( err = setHWParams( channels(),
					SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 )
		{
			printf( "Setting of hwparams failed: %s\n",
							snd_strerror( err ) );
			return;
		}
		if( ( err = setSWParams() ) < 0 )
		{
			printf( "Setting of swparams failed: %s\n",
							snd_strerror( err ) );
			return;
		}
	}

	AudioDevice::applyQualitySettings();
}




void AudioAlsa::run()
{
	surroundSampleFrame * temp =
		new surroundSampleFrame[mixer()->framesPerPeriod()];
	int_sample_t * outbuf =
			new int_sample_t[mixer()->framesPerPeriod() *
								channels()];
	int_sample_t * pcmbuf = new int_sample_t[m_periodSize * channels()];

	int outbuf_size = mixer()->framesPerPeriod() * channels();
	int outbuf_pos = 0;
	int pcmbuf_size = m_periodSize * channels();

	bool quit = false;
	while( quit == false )
	{
		int_sample_t * ptr = pcmbuf;
		int len = pcmbuf_size;
		while( len )
		{
			if( outbuf_pos == 0 )
			{
				// frames depend on the sample rate
				const fpp_t frames = getNextBuffer( temp );
				if( !frames )
				{
					quit = true;
					memset( ptr, 0, len
						* sizeof( int_sample_t ) );
					break;
				}
				outbuf_size = frames * channels();

				convertToS16( temp, frames,
						mixer()->masterGain(),
						outbuf,
						m_convertEndian );
			}
			int min_len = qMin( len, outbuf_size - outbuf_pos );
			memcpy( ptr, outbuf + outbuf_pos,
					min_len * sizeof( int_sample_t ) );
			ptr += min_len;
			len -= min_len;
			outbuf_pos += min_len;
			outbuf_pos %= outbuf_size;
		}

		f_cnt_t frames = m_periodSize;
		ptr = pcmbuf;

		while( frames )
		{
			int err = snd_pcm_writei( m_handle, ptr, frames );

			if( err == -EAGAIN )
			{
				continue;
			}

			if( err < 0 )
			{
				if( handleError( err ) < 0 )
				{
					printf( "Write error: %s\n",
							snd_strerror( err ) );
				}
				break;	// skip this buffer
			}
			ptr += err * channels();
			frames -= err;
		}
	}

	delete[] temp;
	delete[] outbuf;
	delete[] pcmbuf;
}




int AudioAlsa::setHWParams( const ch_cnt_t _channels, snd_pcm_access_t _access )
{
	int err, dir;

	// choose all parameters
	if( ( err = snd_pcm_hw_params_any( m_handle, m_hwParams ) ) < 0 )
	{
		printf( "Broken configuration for playback: no configurations "
				"available: %s\n", snd_strerror( err ) );
		return err;
	}

	// set the interleaved read/write format
	if( ( err = snd_pcm_hw_params_set_access( m_handle, m_hwParams,
							_access ) ) < 0 )
	{
		printf( "Access type not available for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	// set the sample format
	if( ( snd_pcm_hw_params_set_format( m_handle, m_hwParams,
						SND_PCM_FORMAT_S16_LE ) ) < 0 )
	{
		if( ( snd_pcm_hw_params_set_format( m_handle, m_hwParams,
						SND_PCM_FORMAT_S16_BE ) ) < 0 )
		{
			printf( "Neither little- nor big-endian available for "
					"playback: %s\n", snd_strerror( err ) );
			return err;
		}
		m_convertEndian = isLittleEndian();
	}
	else
	{
		m_convertEndian = !isLittleEndian();
	}

	// set the count of channels
	if( ( err = snd_pcm_hw_params_set_channels( m_handle, m_hwParams,
							_channels ) ) < 0 )
	{
		printf( "Channel count (%i) not available for playbacks: %s\n"
				"(Does your soundcard not support surround?)\n",
					_channels, snd_strerror( err ) );
		return err;
	}

	// set the sample rate
	if( ( err = snd_pcm_hw_params_set_rate( m_handle, m_hwParams,
						sampleRate(), 0 ) ) < 0 )
	{
		if( ( err = snd_pcm_hw_params_set_rate( m_handle, m_hwParams,
				mixer()->baseSampleRate(), 0 ) ) < 0 )
		{
			printf( "Could not set sample rate: %s\n",
							snd_strerror( err ) );
			return err;
		}
	}

	m_periodSize = mixer()->framesPerPeriod();
	m_bufferSize = m_periodSize * 8;
	dir = 0;
	err = snd_pcm_hw_params_set_period_size_near( m_handle, m_hwParams,
							&m_periodSize, &dir );
	if( err < 0 )
	{
		printf( "Unable to set period size %lu for playback: %s\n",
					m_periodSize, snd_strerror( err ) );
		return err;
	}
	dir = 0;
	err = snd_pcm_hw_params_get_period_size( m_hwParams, &m_periodSize,
									&dir );
	if( err < 0 )
	{
		printf( "Unable to get period size for playback: %s\n",
							snd_strerror( err ) );
	}

	dir = 0;
	err = snd_pcm_hw_params_set_buffer_size_near( m_handle, m_hwParams,
								&m_bufferSize );
	if( err < 0 )
	{
		printf( "Unable to set buffer size %lu for playback: %s\n",
					m_bufferSize, snd_strerror( err ) );
		return ( err );
	}
	err = snd_pcm_hw_params_get_buffer_size( m_hwParams, &m_bufferSize );

	if( 2 * m_periodSize > m_bufferSize )
	{
		printf( "buffer to small, could not use\n" );
		return ( err );
	}


	// write the parameters to device
	err = snd_pcm_hw_params( m_handle, m_hwParams );
	if( err < 0 )
	{
		printf( "Unable to set hw params for playback: %s\n",
							snd_strerror( err ) );
		return ( err );
	}

	return ( 0 );	// all ok
}




int AudioAlsa::setSWParams()
{
	int err;

	// get the current swparams
	if( ( err = snd_pcm_sw_params_current( m_handle, m_swParams ) ) < 0 )
	{
		printf( "Unable to determine current swparams for playback: %s"
						"\n", snd_strerror( err ) );
		return err;
	}

	// start the transfer when a period is full
	if( ( err = snd_pcm_sw_params_set_start_threshold( m_handle,
					m_swParams, m_periodSize ) ) < 0 )
	{
		printf( "Unable to set start threshold mode for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	// allow the transfer when at least m_periodSize samples can be
	// processed
	if( ( err = snd_pcm_sw_params_set_avail_min( m_handle, m_swParams,
							m_periodSize ) ) < 0 )
	{
		printf( "Unable to set avail min for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	// align all transfers to 1 sample
	
#if SND_LIB_VERSION < ((1<<16)|(0)|16)
	if( ( err = snd_pcm_sw_params_set_xfer_align( m_handle,
							m_swParams, 1 ) ) < 0 )
	{
		printf( "Unable to set transfer align for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}
#endif

	// write the parameters to the playback device
	if( ( err = snd_pcm_sw_params( m_handle, m_swParams ) ) < 0 )
	{
		printf( "Unable to set sw params for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	return 0;	// all ok
}



void testPrintCards()
{
	std::cout << "Listing cards: " << std::endl;
	char **hints;

	/* Enumerate sound devices */
	int err = snd_device_name_hint(-1, "pcm", (void***)&hints);
	if (err != 0)
		return;//Error! Just return

	char** n = hints;
	while (*n != NULL) {

		char *name = snd_device_name_get_hint(*n, "NAME");

		if (name != NULL && 0 != strcmp("null", name)) {
			//Copy name to another buffer and then free it
			std::cout << "Name: " << name << std::endl;

			free(name);
		}

		char *description = snd_device_name_get_hint(*n, "DESC");

		if (description != NULL && 0 != strcmp("null", name)) {
			//Copy name to another buffer and then free it
			std::cout << "Description: " << description << std::endl;

			free(description);
		}
		else
		{
			std::cout << "No description" << std::endl;
		}

		std::cout << std::endl;

		n++;
	}//End of while

	//Free hint buffer too
	snd_device_name_free_hint((void**)hints);
}



void device_list(void)
{
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	snd_ctl_t *handle;
	int card, err, dev, idx;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
	if (snd_card_next(&card) < 0 || card < 0) {
		return;
	}
	std::cout << "**** List of " << snd_pcm_stream_name(stream) << " Hardware Devices ****\n";

	while (card >= 0) {
		char name[32];
		sprintf(name, "hw:%d", card);
		if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
			// TODO Error handling
			//error("control open (%i): %s", card, snd_strerror(err));
			goto next_card;
		}
		if ((err = snd_ctl_card_info(handle, info)) < 0) {
			// TODO Error handling
			//error("control hardware info (%i): %s", card, snd_strerror(err));
			snd_ctl_close(handle);
			goto next_card;
		}
		dev = -1;
		while (1) {
			unsigned int count;
			if (snd_ctl_pcm_next_device(handle, &dev)<0)
				// TODO Error handling
				//error("snd_ctl_pcm_next_device");
				std::cerr << "snd_ctl_pcm_next_device";
			if (dev < 0)
				break;
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
				if (err != -ENOENT)
					// TODO Error handling
					//error("control digital audio info (%i): %s", card, snd_strerror(err));
					std::cerr << "Error\n";
				continue;
			}
			//std::cout << "card " << card << ": " << snd_ctl_card_info_get_id(info) << " [" << snd_ctl_card_info_get_name(info) << "], device " << dev <<
			//			 ": " << snd_pcm_info_get_id(pcminfo) << " [" << snd_pcm_info_get_name(pcminfo) << "]\n";
			std::cout << "card hw" << card << ":" << dev << " - " << "[" << snd_ctl_card_info_get_name(info) << "/" << snd_pcm_info_get_name(pcminfo) << "], " <<
						 snd_ctl_card_info_get_id(info) << "," << snd_pcm_info_get_id(pcminfo) << "\n";

			count = snd_pcm_info_get_subdevices_count(pcminfo);
			std::cout << "  Subdevices: " << snd_pcm_info_get_subdevices_avail(pcminfo) << "/" << count << std::endl;
			for (idx = 0; idx < (int)count; idx++) {
				snd_pcm_info_set_subdevice(pcminfo, idx);
				if ((err = snd_ctl_pcm_info(handle, pcminfo)) < 0) {
					// TODO Error handling
					//error("control digital audio playback info (%i): %s", card, snd_strerror(err));
					std::cerr << "Error\n";
				} else {
					std::cout << " Subdevice #" << idx << ": " << snd_pcm_info_get_subdevice_name(pcminfo) << std::endl;
				}
			}
		}
		snd_ctl_close(handle);
next_card:
		if (snd_card_next(&card) < 0) {
			// TODO Error handling
			//error("snd_card_next");
			break;
		}
	}
}



class DeviceInfo
{
public:
	DeviceInfo(int cardNumber, int deviceNumber,
			   QString const & cardName, QString const & pcmName,
			   QString const & cardId, QString const & pcmId) :
		m_cardNumber(cardNumber),
		m_deviceNumber(deviceNumber),
		m_cardName(cardName),
		m_pcmName(pcmName),
		m_cardId(cardId),
		m_pcmId(pcmId)
	{}
	~DeviceInfo() {}

	int getCardNumber() const { return m_cardNumber; }
	int getDeviceNumber() const { return m_deviceNumber; }
	QString const & getCardName() const { return m_cardName; }
	QString const & getPcmName() const { return m_pcmName; }
	QString const & getCardId() const { return m_cardId; }
	QString const & getPcmId() const { return m_pcmId; }

	QString getHWString() const { return QString("hw%1:%2").arg(m_cardNumber).arg(m_deviceNumber); }

private:
	int m_cardNumber;
	int m_deviceNumber;
	QString m_cardName;
	QString m_pcmName;
	QString m_cardId;
	QString m_pcmId;
};



void populateDeviceInfos(std::vector<DeviceInfo> &deviceInfos)
{
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	snd_ctl_t *handle;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;

	// Allocate memory for the info structs
	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	int card = -1;

	while (!snd_card_next(&card) && card >= 0)
	{
		std::cout << "Card: " << card << " found!" << std::endl;

		char name[32];
		sprintf(name, "hw:%d", card);

		if (snd_ctl_open(&handle, name, 0) < 0)
		{
			std::cerr << "Error opening ALSA card " << name << std::endl;
			continue;
		}
		if (snd_ctl_card_info(handle, info) < 0)
		{
			snd_ctl_close(handle);
			std::cerr << "Could not retrieve info for ALSA card " << name << std::endl;
			continue;
		}

		int dev = -1;

		while (!snd_ctl_pcm_next_device(handle, &dev) && dev >= 0)
		{
			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);
			if (!snd_ctl_pcm_info(handle, pcminfo))
			{
				QString cardName(snd_ctl_card_info_get_name(info));
				QString pcmName(snd_pcm_info_get_name(pcminfo));
				QString cardId(snd_ctl_card_info_get_id(info));
				QString pcmId(snd_pcm_info_get_id(pcminfo));

				DeviceInfo currentDevice(card, dev, cardName, pcmName, cardId, pcmId);
				deviceInfos.push_back(currentDevice);

				std::cout << "card hw" << card << ":" << dev << " - " << "[" << snd_ctl_card_info_get_name(info) << "/" << snd_pcm_info_get_name(pcminfo) << "], " <<
							 snd_ctl_card_info_get_id(info) << "," << snd_pcm_info_get_id(pcminfo) << std::endl;
			}
		}

		snd_ctl_close(handle);
	}
}




AudioAlsa::setupWidget::setupWidget( QWidget * _parent ) :
	AudioDevice::setupWidget( AudioAlsa::name(), _parent )
{
	typedef std::vector<DeviceInfo> DeviceInfoCollection;
	DeviceInfoCollection deviceInfos;
	populateDeviceInfos(deviceInfos);

	// Implements the "-l" from aplay
	//device_list();

	//testPrintCards();

	m_deviceComboBox = new QComboBox(this);
	for (size_t i = 0; i < deviceInfos.size(); ++i)
	{
		DeviceInfo const & currentDeviceInfo = deviceInfos[i];
		QString comboBoxText = currentDeviceInfo.getHWString() + " [" + currentDeviceInfo.getCardName() + " | " + currentDeviceInfo.getPcmName() + "]";
		m_deviceComboBox->addItem(comboBoxText, QVariant(static_cast<uint>(i)));
		m_deviceComboBox->setItemData(i, comboBoxText, Qt::ToolTipRole);
	}

	m_deviceComboBox->setGeometry( 10, 20, 160, 20 );
	connect(m_deviceComboBox, SIGNAL(currentIndexChanged(int)), SLOT(onCurrentIndexChanged(int)));

	//m_device = new QLineEdit( AudioAlsa::probeDevice(), this );
	//m_device->setGeometry( 10, 20, 160, 20 );

	QLabel * dev_lbl = new QLabel( tr( "DEVICE" ), this );
	dev_lbl->setFont( pointSize<7>( dev_lbl->font() ) );
	dev_lbl->setGeometry( 10, 40, 160, 10 );

	LcdSpinBoxModel * m = new LcdSpinBoxModel( /* this */ );
	m->setRange( DEFAULT_CHANNELS, SURROUND_CHANNELS );
	m->setStep( 2 );
	m->setValue( ConfigManager::inst()->value( "audioalsa",
							"channels" ).toInt() );

	m_channels = new LcdSpinBox( 1, this );
	m_channels->setModel( m );
	m_channels->setLabel( tr( "CHANNELS" ) );
	m_channels->move( 180, 20 );

}




AudioAlsa::setupWidget::~setupWidget()
{
	delete m_channels->model();
}




void AudioAlsa::setupWidget::saveSettings()
{
	ConfigManager::inst()->setValue( "audioalsa", "device",
							m_device->text() );
	ConfigManager::inst()->setValue( "audioalsa", "channels",
				QString::number( m_channels->value<int>() ) );
}



void AudioAlsa::setupWidget::onCurrentIndexChanged(int index)
{

}


#endif
