/*
 * AudioFileDeviceSample.cpp - base-class for audio-device-classes which write
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

#include "AudioFileDeviceSample.h"
#include "ExportProjectDialog.h"
#include "GuiApplication.h"

namespace lmms
{

AudioFileDeviceSample::AudioFileDeviceSample(OutputSettings const& outputSettings,
	const QString& _file,
	const fpp_t defaultBufferSize) :
	m_outputFile( _file ),
	m_outputSettings(outputSettings),
	m_defaultFrameCount(defaultBufferSize)
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




AudioFileDeviceSample::~AudioFileDeviceSample()
{
	m_outputFile.close();
}

sample_rate_t AudioFileDeviceSample::getSampleRate()
{
	return m_outputSettings.getSampleRate();
}

const fpp_t AudioFileDeviceSample::getDefaultFrameCount()
{
	return m_defaultFrameCount;
}

void AudioFileDeviceSample::setSampleRate(sample_rate_t newSampleRate)
{
	 m_outputSettings.setSampleRate(newSampleRate);
}

void AudioFileDeviceSample::processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount)
{
    writeBuffer(frameBuffer, frameCount);
}


int AudioFileDeviceSample::writeData( const void* data, int len )
{
	if( m_outputFile.isOpen() )
	{
		return m_outputFile.write( (const char *) data, len );
	}

	return -1;
}

int AudioFileDeviceSample::convertToS16(const SampleFrame* _ab,
	const fpp_t _frames,
	int_sample_t * _output_buffer,
	const bool _convert_endian)
{
	constexpr unsigned int channels = 2;
	if(_convert_endian)
	{
		for( fpp_t frame = 0; frame < _frames; ++frame )
		{
			for (ch_cnt_t chnl = 0; chnl < channels; ++chnl)
			{
				auto temp = static_cast<int_sample_t>(AudioEngine::clip(_ab[frame][chnl]) * OUTPUT_SAMPLE_MULTIPLIER);

				(_output_buffer + frame * channels)[chnl] =
						(temp & 0x00ff) << 8 |
						(temp & 0xff00) >> 8;
			}
		}
	}
	else
	{
		for (fpp_t frame = 0; frame < _frames; ++frame)
		{
			for (ch_cnt_t chnl = 0; chnl < channels; ++chnl)
			{
				(_output_buffer + frame * channels)[chnl]
					= static_cast<int_sample_t>(AudioEngine::clip(_ab[frame][chnl]) * OUTPUT_SAMPLE_MULTIPLIER);
			}
		}
	}
	return _frames * channels * BYTES_PER_INT_SAMPLE;
}




void AudioFileDeviceSample::clearS16Buffer(int_sample_t * _outbuf, const fpp_t _frames)
{
	assert(_outbuf != nullptr);

	constexpr unsigned int channels = 2;
	memset(_outbuf, 0,  _frames * channels * BYTES_PER_INT_SAMPLE);
}

} // namespace lmms
