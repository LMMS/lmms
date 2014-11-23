/*
 * AudioFileDevice.cpp - base-class for audio-device-classes which write
 *                       their output into a file
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

#include <QtGui/QMessageBox>

#include "AudioFileDevice.h"
#include "export_project_dialog.h"


AudioFileDevice::AudioFileDevice( const sample_rate_t _sample_rate,
					const ch_cnt_t _channels,
					const QString & _file,
					const bool _use_vbr,
					const bitrate_t _nom_bitrate,
					const bitrate_t _min_bitrate,
					const bitrate_t _max_bitrate,
					const int _depth,
					Mixer*  _mixer ) :
	AudioDevice( _channels, _mixer ),
	m_outputFile( _file ),
	m_useVbr( _use_vbr ),
	m_nomBitrate( _nom_bitrate ),
	m_minBitrate( _min_bitrate ),
	m_maxBitrate( _max_bitrate ),
	m_depth( _depth )
{
	setSampleRate( _sample_rate );

	if( m_outputFile.open( QFile::WriteOnly | QFile::Truncate ) == false )
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




AudioFileDevice::~AudioFileDevice()
{
	m_outputFile.close();
}




int AudioFileDevice::writeData( const void* data, int len )
{
	if( m_outputFile.isOpen() )
	{
		return m_outputFile.write( (const char *) data, len );
	}

	return -1;
}

