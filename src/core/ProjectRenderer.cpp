/*
 * ProjectRenderer.cpp - ProjectRenderer-class for easily rendering projects
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

#include "ProjectRenderer.h"
#include "Song.h"
#include "PerfLog.h"

#include "AudioFileWave.h"
#include "AudioFileOgg.h"
#include "AudioFileMP3.h"
#include "AudioFileFlac.h"


namespace lmms
{


const std::array<ProjectRenderer::FileEncodeDevice, 5> ProjectRenderer::fileEncodeDevices
{

	FileEncodeDevice{ ProjectRenderer::ExportFileFormat::Wave,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "WAV (*.wav)" ),
					".wav", &AudioFileWave::getInst },
	FileEncodeDevice{ ProjectRenderer::ExportFileFormat::Flac,
		QT_TRANSLATE_NOOP("ProjectRenderer", "FLAC (*.flac)"),
		".flac",
		&AudioFileFlac::getInst
	},
	FileEncodeDevice{ ProjectRenderer::ExportFileFormat::Ogg,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "OGG (*.ogg)" ),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&AudioFileOgg::getInst
#else
					nullptr
#endif
									},
	FileEncodeDevice{ ProjectRenderer::ExportFileFormat::MP3,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "MP3 (*.mp3)" ),
					".mp3",
#ifdef LMMS_HAVE_MP3LAME
					&AudioFileMP3::getInst
#else
					nullptr
#endif
									},
	// Insert your own file-encoder infos here.
	// Maybe one day the user can add own encoders inside the program.

	FileEncodeDevice{ ProjectRenderer::ExportFileFormat::Count, nullptr, nullptr, nullptr }

} ;

ProjectRenderer::ProjectRenderer(
	const OutputSettings& outputSettings, ExportFileFormat exportFileFormat, const QString& outputFilename)
	: QThread(Engine::audioEngine())
	, m_fileDev(nullptr)
	, m_progress(0)
	, m_abort(false)
{
	AudioFileDeviceInstantiaton audioEncoderFactory = fileEncodeDevices[static_cast<std::size_t>(exportFileFormat)].m_getDevInst;

	if (audioEncoderFactory)
	{
		bool successful = false;

		m_fileDev = audioEncoderFactory(
					outputFilename, outputSettings, DEFAULT_CHANNELS,
					Engine::audioEngine(), successful );
		if( !successful )
		{
			delete m_fileDev;
			m_fileDev = nullptr;
		}
	}
}




// Little help function for getting file format from a file extension
// (only for registered file-encoders).
ProjectRenderer::ExportFileFormat ProjectRenderer::getFileFormatFromExtension(
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




QString ProjectRenderer::getFileExtensionFromFormat(
		ExportFileFormat fmt )
{
	return fileEncodeDevices[static_cast<std::size_t>(fmt)].m_extension;
}




void ProjectRenderer::startProcessing()
{

	if( isReady() )
	{
		// Have to do audio engine stuff with GUI-thread affinity in order to
		// make slots connected to sampleRateChanged()-signals being called immediately.
		Engine::audioEngine()->setAudioDevice(m_fileDev, false, false);

		start(
#ifndef LMMS_BUILD_WIN32
			QThread::HighPriority
#endif
						);

	}
}


void ProjectRenderer::run()
{
	PerfLogTimer perfLog("Project Render");

	Engine::getSong()->startExport();
	// Skip first empty buffer.
	Engine::audioEngine()->nextBuffer();

	m_progress = 0;

	// Now start processing
	Engine::audioEngine()->startProcessing(false);

	// Continually track and emit progress percentage to listeners.
	while (!Engine::getSong()->isExportDone() && !m_abort)
	{
		m_fileDev->processNextBuffer();
		const int nprog = Engine::getSong()->getExportProgress();
		if (m_progress != nprog)
		{
			m_progress = nprog;
			emit progressChanged( m_progress );
		}
	}

	// Notify the audio engine of the end of processing.
	Engine::audioEngine()->stopProcessing();

	Engine::getSong()->stopExport();

	perfLog.end();

	// If the user aborted export-process, the file has to be deleted.
	const QString f = m_fileDev->outputFile();
	if( m_abort )
	{
		QFile( f ).remove();
	}
}




void ProjectRenderer::abortProcessing()
{
	m_abort = true;
	wait();
}



void ProjectRenderer::updateConsoleProgress()
{
	constexpr int cols = 50;
	static int rot = 0;
	auto buf = std::array<char, 80>{};
	auto prog = std::array<char, cols + 1>{};

	for( int i = 0; i < cols; ++i )
	{
		prog[i] = ( i*100/cols <= m_progress ? '-' : ' ' );
	}
	prog[cols] = 0;

	const auto activity = "|/-\\";
	std::fill(buf.begin(), buf.end(), 0);
	std::snprintf(buf.data(), buf.size(), "\r|%s|    %3d%%   %c  ", prog.data(), m_progress,
							activity[rot] );
	rot = ( rot+1 ) % 4;

	fprintf( stderr, "%s", buf.data() );
	fflush( stderr );
}


} // namespace lmms
