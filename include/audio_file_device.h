/*
 * audio_file_device.h - base-class for audio-device-classes which write 
 *                       their output into a file
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _AUDIO_FILE_DEVICE_H
#define _AUDIO_FILE_DEVICE_H


#include "qt3support.h"

#ifdef QT4

#include <QFile>

#else

#include <qfile.h>

#endif


#include "audio_device.h"



class audioFileDevice : public audioDevice
{
public:
	audioFileDevice( Uint32 _sample_rate, Uint8 _channels,
				const QString & _file, bool _use_vbr,
				Uint16 _nom_bitrate, Uint16 _min_bitrate,
				Uint16 _max_bitrate );
	virtual ~audioFileDevice();


protected:
	int FASTCALL writeData( const void * _data, int _len );
	void seekToBegin( void );

	inline bool useVBR( void ) const
	{
		return( m_useVbr );
	}
	inline Uint16 nominalBitrate( void ) const
	{
		return( m_nomBitrate );
	}
	inline Uint16 minBitrate( void ) const
	{
		return( m_minBitrate );
	}
	inline Uint16 maxBitrate( void ) const
	{
		return( m_maxBitrate );
	}
	inline bool outputFileOpened( void ) const
	{
		return( m_outputFile.isOpen() );
	}


private:
	QFile m_outputFile;

	bool m_useVbr;

	Uint16 m_nomBitrate;
	Uint16 m_minBitrate;
	Uint16 m_maxBitrate;

} ;


#endif
