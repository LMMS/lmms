/*
 * AudioAlsa.cpp - device-class which implements ALSA-PCM-output
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "AudioAlsa.h"

#ifdef LMMS_HAVE_ALSA

#include "endian_handling.h"
#include "AudioEngine.h"
#include "ConfigManager.h"

namespace lmms
{

AudioAlsa::AudioAlsa( bool & _success_ful, AudioEngine*  _audioEngine ) :
	AudioDevice(std::clamp<ch_cnt_t>(
		ConfigManager::inst()->value("audioalsa", "channels").toInt(),
		DEFAULT_CHANNELS,
		DEFAULT_CHANNELS), _audioEngine),
	m_handle( nullptr ),
	m_hwParams( nullptr ),
	m_swParams( nullptr ),
	m_convertEndian( false )
{
	_success_ful = false;

	if( setenv( "PULSE_ALSA_HOOK_CONF", "/dev/null", 0 ) )
	{
		fprintf( stderr,
		"Could not avoid possible interception by PulseAudio\n" );
	}

	if (int err = snd_pcm_open(&m_handle, probeDevice().toLatin1().constData(), SND_PCM_STREAM_PLAYBACK, 0); err < 0)
	{
		printf( "Playback open error: %s\n", snd_strerror( err ) );
		return;
	}

	snd_pcm_hw_params_malloc( &m_hwParams );
	snd_pcm_sw_params_malloc( &m_swParams );

	if (int err = setHWParams(channels(), SND_PCM_ACCESS_RW_INTERLEAVED); err < 0)
	{
		printf( "Setting of hwparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}
	if (int err = setSWParams(); err < 0)
	{
		printf( "Setting of swparams failed: %s\n",
							snd_strerror( err ) );
		return;
	}

	// set FD_CLOEXEC flag for all file descriptors so forked processes
	// do not inherit them
	int count = snd_pcm_poll_descriptors_count( m_handle );
	auto ufds = new pollfd[count];
	snd_pcm_poll_descriptors( m_handle, ufds, count );
	for (int i = 0; i < std::max(3, count); ++i)
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
	if( m_handle != nullptr )
	{
		snd_pcm_close( m_handle );
	}

	if( m_hwParams != nullptr )
	{
		snd_pcm_hw_params_free( m_hwParams );
	}

	if( m_swParams != nullptr )
	{
		snd_pcm_sw_params_free( m_swParams );
	}
}




QString AudioAlsa::probeDevice()
{
	QString dev = ConfigManager::inst()->value( "audioalsa", "device" );
	if( dev == "" )
	{
		if( getenv( "AUDIODEV" ) != nullptr )
		{
			return getenv( "AUDIODEV" );
		}
		return "default";
	}
	return dev;
}




/**
 * @brief Creates a list of all available devices.
 *
 * Uses the hints API of ALSA to collect all devices. This also includes plug
 * devices. The reason to collect these and not the raw hardware devices
 * (e.g. hw:0,0) is that hardware devices often have a very limited number of
 * supported formats, etc. Plugs on the other hand are software components that
 * map all types of formats and inputs to the hardware and therefore they are
 * much more flexible and more what we want.
 *
 * Further helpful info http://jan.newmarch.name/LinuxSound/Sampled/Alsa/.
 *
 * @return A collection of devices found on the system.
 */
AudioAlsa::DeviceInfoCollection AudioAlsa::getAvailableDevices()
{
	DeviceInfoCollection deviceInfos;

	char** hints = nullptr;

	/* Enumerate sound devices */
	int err = snd_device_name_hint(-1, "pcm", (void***)&hints);
	if (err != 0)
	{
		return deviceInfos;
	}

	char** n = hints;
	while (*n != nullptr)
	{
		char *name = snd_device_name_get_hint(*n, "NAME");
		char *description = snd_device_name_get_hint(*n, "DESC");

		if (name != 0 && description != 0)
		{
			deviceInfos.push_back(DeviceInfo(QString(name), QString(description)));
		}

		free(name);
		free(description);

		n++;
	}

	//Free the hint buffer
	snd_device_name_free_hint((void**)hints);

	return deviceInfos;
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
	stopProcessingThread( this );
}

void AudioAlsa::run()
{
	auto temp = new SampleFrame[audioEngine()->framesPerPeriod()];
	auto outbuf = new int_sample_t[audioEngine()->framesPerPeriod() * channels()];
	auto pcmbuf = new int_sample_t[m_periodSize * channels()];

	int outbuf_size = audioEngine()->framesPerPeriod() * channels();
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

				convertToS16(temp, frames, outbuf, m_convertEndian);
			}
			int min_len = std::min(len, outbuf_size - outbuf_pos);
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
	// choose all parameters
	if (int err = snd_pcm_hw_params_any(m_handle, m_hwParams); err < 0)
	{
		printf( "Broken configuration for playback: no configurations "
				"available: %s\n", snd_strerror( err ) );
		return err;
	}

	// set the interleaved read/write format
	if (int err = snd_pcm_hw_params_set_access(m_handle, m_hwParams, _access); err < 0)
	{
		printf( "Access type not available for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	// set the sample format
	if (int err = snd_pcm_hw_params_set_format(m_handle, m_hwParams, SND_PCM_FORMAT_S16_LE); err < 0)
	{
		if (int err = snd_pcm_hw_params_set_format(m_handle, m_hwParams, SND_PCM_FORMAT_S16_BE); err < 0)
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
	if (int err = snd_pcm_hw_params_set_channels(m_handle, m_hwParams, _channels); err < 0)
	{
		printf( "Channel count (%i) not available for playbacks: %s\n"
				"(Does your soundcard not support surround?)\n",
					_channels, snd_strerror( err ) );
		return err;
	}

	// set the sample rate
	if (int err = snd_pcm_hw_params_set_rate(m_handle, m_hwParams, sampleRate(), 0); err < 0)
	{
		if (int err = snd_pcm_hw_params_set_rate(m_handle, m_hwParams, audioEngine()->baseSampleRate(), 0); err < 0)
		{
			printf( "Could not set sample rate: %s\n",
							snd_strerror( err ) );
			return err;
		}
	}

	m_periodSize = audioEngine()->framesPerPeriod();
	m_bufferSize = m_periodSize * 8;
	int dir;
	if (int err = snd_pcm_hw_params_set_period_size_near(m_handle, m_hwParams, &m_periodSize, &dir); err < 0)
	{
		printf( "Unable to set period size %lu for playback: %s\n",
					m_periodSize, snd_strerror( err ) );
		return err;
	}
	dir = 0;
	if (int err = snd_pcm_hw_params_get_period_size(m_hwParams, &m_periodSize, &dir); err < 0)
	{
		printf( "Unable to get period size for playback: %s\n",
							snd_strerror( err ) );
	}

	dir = 0;
	if (int err = snd_pcm_hw_params_set_buffer_size_near(m_handle, m_hwParams, &m_bufferSize); err < 0)
	{
		printf( "Unable to set buffer size %lu for playback: %s\n",
					m_bufferSize, snd_strerror( err ) );
		return ( err );
	}

	if (int err = snd_pcm_hw_params_get_buffer_size(m_hwParams, &m_bufferSize); 2 * m_periodSize > m_bufferSize)
	{
		printf( "buffer to small, could not use\n" );
		return ( err );
	}


	// write the parameters to device
	if (int err = snd_pcm_hw_params(m_handle, m_hwParams); err < 0)
	{
		printf( "Unable to set hw params for playback: %s\n",
							snd_strerror( err ) );
		return ( err );
	}

	return ( 0 );	// all ok
}




int AudioAlsa::setSWParams()
{
	// get the current swparams
	if (int err = snd_pcm_sw_params_current(m_handle, m_swParams); err < 0)
	{
		printf( "Unable to determine current swparams for playback: %s"
						"\n", snd_strerror( err ) );
		return err;
	}

	// start the transfer when a period is full
	if (int err = snd_pcm_sw_params_set_start_threshold(m_handle, m_swParams, m_periodSize); err < 0)
	{
		printf( "Unable to set start threshold mode for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	// allow the transfer when at least m_periodSize samples can be
	// processed
	if (int err = snd_pcm_sw_params_set_avail_min(m_handle, m_swParams, m_periodSize); err < 0)
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
	if (int err = snd_pcm_sw_params(m_handle, m_swParams); err < 0)
	{
		printf( "Unable to set sw params for playback: %s\n",
							snd_strerror( err ) );
		return err;
	}

	return 0;	// all ok
}

} // namespace lmms

#endif // LMMS_HAVE_ALSA
