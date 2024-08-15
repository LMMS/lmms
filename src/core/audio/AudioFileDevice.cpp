/*
 * AudioFileDevice.cpp - base-class for audio-device-classes which write
 *                       their output into a file
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

#include <QMessageBox>

#include "AudioFileDevice.h"
#include "ExportProjectDialog.h"
#include "GuiApplication.h"

namespace lmms
{

AudioFileDevice::AudioFileDevice(OutputSettings const & outputSettings,
	const QString & _file, const ch_cnt_t _channels,
	const fpp_t defaultBufferSize) :
	m_outputFile( _file ),
	m_outputSettings(outputSettings),
	m_defaultFrameCount(defaultBufferSize),
	m_channelCount(_channels)
{
	using gui::ExportProjectDialog;

	if( m_outputFile.open( QFile::WriteOnly | QFile::Truncate ) == false )
	{
		QString title, message;
		title = ExportProjectDialog::tr( "Could not open file" );
		message = ExportProjectDialog::tr( "Could not open file %1 "
						"for writing.\nPlease make "
						"sure you have write "
						"permission to the file and "
						"the directory containing the "
						"file and try again!"
								).arg( _file );

		if (gui::getGUI() != nullptr)
		{
			QMessageBox::critical( nullptr, title, message,
						QMessageBox::Ok,
						QMessageBox::NoButton );
		}
		else
		{
			fprintf( stderr, "%s\n", message.toUtf8().constData() );
			exit( EXIT_FAILURE );
		}
	}
}




AudioFileDevice::~AudioFileDevice()
{
	m_outputFile.close();
}

sample_rate_t AudioFileDevice::getSampleRate()
{
	return m_outputSettings.getSampleRate();
}

ch_cnt_t AudioFileDevice::getChannel()
{
	return m_channelCount;
}

const fpp_t AudioFileDevice::getDefaultFrameCount()
{
	return m_defaultFrameCount;
}

void AudioFileDevice::setSampleRate(sample_rate_t newSampleRate)
{
	 m_outputSettings.setSampleRate(newSampleRate);
}

void AudioFileDevice::processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount)
{
    writeBuffer(frameBuffer, frameCount);
}


int AudioFileDevice::writeData( const void* data, int len )
{
	if( m_outputFile.isOpen() )
	{
		return m_outputFile.write( (const char *) data, len );
	}

	return -1;
}

} // namespace lmms
