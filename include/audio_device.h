/*
 * audio_device.h - base-class for audio-devices, used by LMMS-mixer
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _AUDIO_DEVICE_H
#define _AUDIO_DEVICE_H

#include <QtCore/QPair>
#include <QtCore/QMutex>
#include <QtCore/QThread>

#include "mixer.h"
#include "tab_widget.h"


class audioPort;



class audioDevice
{
public:
	audioDevice( const ch_cnt_t _channels, mixer * _mixer );
	virtual ~audioDevice();

	inline void lock( void )
	{
		m_devMutex.lock();
	}

	inline void unlock( void )
	{
		m_devMutex.unlock();
	}


	// if audio-driver supports ports, classes inherting audioPort
	// (e.g. channel-tracks) can register themselves for making
	// audio-driver able to collect their individual output and provide
	// them at a specific port - currently only supported by JACK
	virtual void registerPort( audioPort * _port );
	virtual void unregisterPort( audioPort * _port );
	virtual void renamePort( audioPort * _port );


	inline sample_rate_t sampleRate( void ) const
	{
		return( m_sampleRate );
	}

	ch_cnt_t channels( void ) const
	{
		return( m_channels );
	}

	void processNextBuffer( void );

	virtual void startProcessing( void )
	{
		m_inProcess = TRUE;
	}

	virtual void stopProcessing( void );

	virtual void applyQualitySettings( void );



	class setupWidget : public tabWidget
	{
	public:
		setupWidget( const QString & _caption, QWidget * _parent ) :
			tabWidget( tabWidget::tr( "Settings for %1" ).arg(
					tabWidget::tr( _caption.toAscii() ) ).
							toUpper(), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void ) = 0;


	public slots:
		virtual void show( void )
		{
			parentWidget()->show();
			QWidget::show();
		}

	} ;



protected:
	// subclasses can re-implement this for being used in conjunction with
	// processNextBuffer()
	virtual void writeBuffer( const surroundSampleFrame * _ab,
						const fpp_t _frames,
						const float _master_gain )
	{
	}

	// called by according driver for fetching new sound-data
	fpp_t getNextBuffer( surroundSampleFrame * _ab );

	// convert a given audio-buffer to a buffer in signed 16-bit samples
	// returns num of bytes in outbuf
	Uint32 convertToS16( const surroundSampleFrame * _ab,
					const fpp_t _frames,
					const float _master_gain,
					int_sample_t * _output_buffer,
					const bool _convert_endian = FALSE );

	// clear given signed-int-16-buffer
	void clearS16Buffer( int_sample_t * _outbuf,
							const fpp_t _frames );

	// resample given buffer from samplerate _src_sr to samplerate _dst_sr
	void resample( const surroundSampleFrame * _src,
					const fpp_t _frames,
					surroundSampleFrame * _dst,
					const sample_rate_t _src_sr,
					const sample_rate_t _dst_sr );

	inline void setSampleRate( const sample_rate_t _new_sr )
	{
		m_sampleRate = _new_sr;
	}

	mixer * getMixer( void )
	{
		return( m_mixer );
	}

	bool hqAudio( void ) const;


private:
	sample_rate_t m_sampleRate;
	ch_cnt_t m_channels;
	mixer * m_mixer;
	bool m_inProcess;

	QMutex m_devMutex;

	SRC_DATA m_srcData;
	SRC_STATE * m_srcState;

	surroundSampleFrame * m_buffer;

} ;


#endif
