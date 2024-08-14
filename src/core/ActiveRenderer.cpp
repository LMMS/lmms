/*
 * ActiveRenderer.cpp - ActiveRenderer-class for easily rendering projects
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QFile>

#include "ActiveRenderer.h"
#include "Song.h"
#include "PerfLog.h"

#include "AudioFileWave.h"
#include "AudioFileOgg.h"
#include "AudioFileMP3.h"
#include "AudioFileFlac.h"


namespace lmms
{


const std::array<ActiveRenderer::FileEncodeDevice, 5> ActiveRenderer::fileEncodeDevices
{

	FileEncodeDevice{ActiveRenderer::ExportFileFormat::Wave,
		QT_TRANSLATE_NOOP("ActiveRenderer", "WAV (*.wav)"),
					".wav", &AudioFileWave::getInst },
	FileEncodeDevice{ActiveRenderer::ExportFileFormat::Flac,
		QT_TRANSLATE_NOOP("ActiveRenderer", "FLAC (*.flac)"),
		".flac",
		&AudioFileFlac::getInst
	},
	FileEncodeDevice{ActiveRenderer::ExportFileFormat::Ogg,
		QT_TRANSLATE_NOOP("ActiveRenderer", "OGG (*.ogg)"),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&AudioFileOgg::getInst
#else
					nullptr
#endif
									},
	FileEncodeDevice{ActiveRenderer::ExportFileFormat::MP3,
		QT_TRANSLATE_NOOP("ActiveRenderer", "MP3 (*.mp3)"),
					".mp3",
#ifdef LMMS_HAVE_MP3LAME
					&AudioFileMP3::getInst
#else
					nullptr
#endif
									},
	// Insert your own file-encoder infos here.
	// Maybe one day the user can add own encoders inside the program.

	FileEncodeDevice{ActiveRenderer::ExportFileFormat::Count, nullptr, nullptr, nullptr}

} ;




ActiveRenderer::ActiveRenderer( const AudioEngine::qualitySettings & qualitySettings,
					const OutputSettings & outputSettings,
					ExportFileFormat exportFileFormat,
					const QString & outputFilename,
					const fpp_t defaultFrameCount,
					BufferFn getBufferFunction,
					EndFn endFunction,
					void* getBufferData) :
	QThread( Engine::audioEngine() ),
	m_fileDev( nullptr ),
	m_qualitySettings( qualitySettings ),
	m_abort(false),
	m_getBufferFunction(getBufferFunction),
	m_endFunction(endFunction),
	m_getBufferData(getBufferData),
	m_buffer(0)
{
	AudioFileDeviceInstantiaton audioEncoderFactory = fileEncodeDevices[static_cast<std::size_t>(exportFileFormat)].m_getDevInst;

	if (audioEncoderFactory)
	{
		bool successful = false;

		m_fileDev = audioEncoderFactory(
					outputSettings, successful, outputFilename, DEFAULT_CHANNELS,
					defaultFrameCount);
		if( !successful )
		{
			delete m_fileDev;
			m_fileDev = nullptr;
		}
	}
}




// Little help function for getting file format from a file extension
// (only for registered file-encoders).
ActiveRenderer::ExportFileFormat ActiveRenderer::getFileFormatFromExtension(
							const QString & _ext )
{
	int idx = 0;
	while( fileEncodeDevices[idx].m_fileFormat != ExportFileFormat::Count )
	{
		if( QString( fileEncodeDevices[idx].m_extension ) == _ext )
		{
			return( fileEncodeDevices[idx].m_fileFormat );
		}
		++idx;
	}

	return( ExportFileFormat::Wave ); // Default.
}




QString ActiveRenderer::getFileExtensionFromFormat(
		ExportFileFormat fmt )
{
	return fileEncodeDevices[static_cast<std::size_t>(fmt)].m_extension;
}


bool ActiveRenderer::processNextBuffer()
{
	m_getBufferFunction(&m_buffer, m_getBufferData);
	if (m_buffer.size() <= 0) { return true; }
	
	qDebug("write buffer, count: %ld", m_buffer.size());

	return false;
}

bool ActiveRenderer::processThisBuffer(SampleFrame* frameBuffer, const fpp_t frameCount)
{
	if (frameCount <= 0) { m_buffer.clear(); return true; }
	if (m_buffer.size() != frameCount)
	{
		m_buffer.resize(frameCount);
	}

	memcpy(m_buffer.data(), frameBuffer, m_buffer.size() * sizeof(SampleFrame));
	return false;
}


void ActiveRenderer::startProcessing()
{

	if( isReady() )
	{
		start(
#ifndef LMMS_BUILD_WIN32
			QThread::HighPriority
#endif
						);

	}
}


void ActiveRenderer::run()
{
#if 0
#if defined(LMMS_BUILD_LINUX) || defined(LMMS_BUILD_FREEBSD)
#ifdef LMMS_HAVE_SCHED_H
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	sched_setaffinity( 0, sizeof( mask ), &mask );
#endif
#endif
#endif

	PerfLogTimer perfLog("Project Render");

	qDebug("start running");

	while (!m_abort)
	{
		// if a function pointer was provided
		// use that to fill m_buffer
		qDebug("m_getBufferFunction if null");
		if (m_getBufferFunction != nullptr)
		{
			qDebug("m_getBufferFunction not null");
			processNextBuffer();
		}
		
		// if m_buffer wasn't filled by
		// processNextBuffer() or processThisBuffer() before
		if (m_buffer.size() <= 0)
		{
			qDebug("m_getBufferFunction break because of size");
			break;
		}

		m_fileDev->processThisBuffer(m_buffer.data(), m_buffer.size());
		// Continually track and emit progress percentage to listeners.
		progressChanged();
		
		// if no function pointer was provided
		if (m_getBufferFunction == nullptr)
		{
			// clear buffer to end while loop
			m_buffer.clear();
			break;
		}
	}

	if (m_endFunction != nullptr)
	{
		m_endFunction(m_getBufferData);
	}

	perfLog.end();

	// If the user aborted export-process, the file has to be deleted.
	const QString f = m_fileDev->outputFile();
	if( m_abort )
	{
		QFile( f ).remove();
	}
}




void ActiveRenderer::abortProcessing()
{
	m_abort = true;
	wait();
}



} // namespace lmms
