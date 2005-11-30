/*
 * audio_device.h - base-class for audio-devices, used by LMMS-mixer
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


#ifndef _AUDIO_DEVICE_H
#define _AUDIO_DEVICE_H

#include "qt3support.h"

#ifdef QT4

#include <QPair>
#include <QMutex>

#else

#include <qpair.h>
#include <qmutex.h>

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SAMPLERATE_H
#include <samplerate.h>
#endif


#include "mixer.h"
#include "tab_widget.h"



class audioPort;



class audioDevice
{
public:
	audioDevice( Uint32 _sample_rate, Uint8 _channels );
	virtual ~audioDevice();

	inline void lock( void )
	{
		m_devMutex.lock();
	}

	inline void unlock( void )
	{
		m_devMutex.unlock();
	}

	// called by mixer for writing final output-buffer with given sample-
	// rate and master-gain
	void FASTCALL writeBuffer( surroundSampleFrame * _ab, Uint32 _frames,
							Uint32 _src_sample_rate,
							float _master_gain );


	// if audio-driver supports ports, classes inherting audioPort
	// (e.g. channel-tracks) can register themselves for making
	// audio-driver able to collect their individual output and provide
	// them at a specific port - currently only supported by JACK
	virtual void registerPort( audioPort * _port );
	virtual void unregisterPort( audioPort * _port );
	virtual void renamePort( audioPort * _port, const QString & _name );


	inline Uint32 sampleRate( void ) const
	{
		return( m_sampleRate );
	}

	Uint8 channels( void ) const
	{
		return( m_channels );
	}



	class setupWidget : public tabWidget
	{
	public:
		setupWidget( const QString & _caption, QWidget * _parent ) :
			tabWidget( tabWidget::tr( "Settings for %1" ).arg(
							_caption ), _parent )
		{
		}

		virtual ~setupWidget()
		{
		}

		virtual void saveSettings( void ) = 0;

	} ;



protected:
	// to be implemented by audio-driver - last step in a mixer period
	virtual void FASTCALL writeBufferToDev( surroundSampleFrame * _ab,
						Uint32 _frames,
						float _master_gain ) = 0;

	// convert a given audio-buffer to a buffer in signed 16-bit samples
	// returns num of bytes in outbuf
	int FASTCALL convertToS16( surroundSampleFrame * _ab, Uint32 _frames,
					float _master_gain,
					outputSampleType * _output_buffer,
					bool _convert_endian = FALSE );

	// clear given signed-int-16-buffer
	void FASTCALL clearS16Buffer( outputSampleType * _outbuf,
					Uint32 _frames );

	// resample given buffer from samplerate _src_src to samplerate _dst_src
	void FASTCALL resample( surroundSampleFrame * _src, Uint32 _frames,
					surroundSampleFrame * _dst,
					Uint32 _src_sr, Uint32 _dst_sr );

	inline void setSampleRate( Uint32 _new_sr )
	{
		m_sampleRate = _new_sr;
	}


private:
	Uint32 m_sampleRate;
	Uint8 m_channels;
	QMutex m_devMutex;

#ifdef HAVE_SAMPLERATE_H
	SRC_DATA m_srcData;
	SRC_STATE * m_srcState;
#endif

} ;


#endif
