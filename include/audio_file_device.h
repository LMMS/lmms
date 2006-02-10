/*
 * audio_file_device.h - base-class for audio-device-classes which write 
 *                       their output into a file
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
	audioFileDevice( const sample_rate_t _sample_rate,
				const ch_cnt_t _channels, const QString & _file,
				const bool _use_vbr,
				const bitrate_t _nom_bitrate,
				const bitrate_t _min_bitrate,
				const bitrate_t _max_bitrate,
				mixer * _mixer );
	virtual ~audioFileDevice();



protected:
	Sint32 FASTCALL writeData( const void * _data, Sint32 _len );
	void seekToBegin( void );

	inline bool useVBR( void ) const
	{
		return( m_useVbr );
	}

	inline bitrate_t nominalBitrate( void ) const
	{
		return( m_nomBitrate );
	}

	inline bitrate_t minBitrate( void ) const
	{
		return( m_minBitrate );
	}

	inline bitrate_t maxBitrate( void ) const
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

	bitrate_t m_nomBitrate;
	bitrate_t m_minBitrate;
	bitrate_t m_maxBitrate;

} ;


#endif
