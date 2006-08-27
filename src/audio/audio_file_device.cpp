#ifndef SINGLE_SOURCE_COMPILE

/*
 * audio_file_device.cpp - base-class for audio-device-classes which write
 *                         their output into a file
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QMessageBox>

#else

#include <qmessagebox.h>

#endif


#include "audio_file_device.h"
#include "export_project_dialog.h"
#include "buffer_allocator.h"


audioFileDevice::audioFileDevice( const sample_rate_t _sample_rate,
					const ch_cnt_t _channels,
					const QString & _file,
					const bool _use_vbr,
					const bitrate_t _nom_bitrate,
					const bitrate_t _min_bitrate,
					const bitrate_t _max_bitrate,
					mixer * _mixer ) :
	audioDevice( _sample_rate, _channels, _mixer ),
	m_outputFile( _file ),
	m_useVbr( _use_vbr ),
	m_nomBitrate( _nom_bitrate ),
	m_minBitrate( _min_bitrate ),
	m_maxBitrate( _max_bitrate )
{
#ifdef QT4
	if( m_outputFile.open( QFile::WriteOnly | QFile::Truncate ) == FALSE )
#else
	if( m_outputFile.open( IO_WriteOnly | IO_Truncate ) == FALSE )
#endif
	{
		QMessageBox::critical( NULL,
			exportProjectDialog::tr( "Could not open file" ),
			exportProjectDialog::tr( "Could not open file %1 "
						"for writing.\nPlease make "
						"sure you have write-"
						"permission to the file and "
						"the directory containing the "
						"file and try again!" ).arg(
									_file ),
					QMessageBox::Ok,
					QMessageBox::NoButton );
	}
}




audioFileDevice::~audioFileDevice()
{
	m_outputFile.close();
}




Sint32 audioFileDevice::writeData( const void * _data, Sint32 _len )
{
#ifdef QT4
	return( m_outputFile.write( (const char *) _data, _len ) );
#else
	return( m_outputFile.writeBlock( (const char *) _data, _len ) );
#endif
}




void audioFileDevice::seekToBegin( void )
{
	m_outputFile.seek( 0 );
}


#endif
