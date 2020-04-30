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

#ifdef LMMS_HAVE_SCHED_H
#include "sched.h"
#endif

const ProjectRenderer::FileEncodeDevice ProjectRenderer::fileEncodeDevices[] =
{

	{ ProjectRenderer::WaveFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "WAV (*.wav)" ),
					".wav", &AudioFileWave::getInst },
	{ ProjectRenderer::FlacFile,
		QT_TRANSLATE_NOOP("ProjectRenderer", "FLAC (*.flac)"),
		".flac",
		&AudioFileFlac::getInst
	},
	{ ProjectRenderer::OggFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "OGG (*.ogg)" ),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&AudioFileOgg::getInst
#else
					NULL
#endif
									},
	{ ProjectRenderer::MP3File,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "MP3 (*.mp3)" ),
					".mp3",
#ifdef LMMS_HAVE_MP3LAME
					&AudioFileMP3::getInst
#else
					NULL
#endif
									},
	// Insert your own file-encoder infos here.
	// Maybe one day the user can add own encoders inside the program.

	{ ProjectRenderer::NumFileFormats, NULL, NULL, NULL }

} ;




ProjectRenderer::ProjectRenderer( const Mixer::qualitySettings & qualitySettings,
					const OutputSettings & outputSettings,
					ExportFileFormats exportFileFormat,
					const QString & outputFilename ) :
	QThread( Engine::mixer() ),
	m_fileDev( NULL ),
	m_qualitySettings( qualitySettings ),
	m_progress( 0 ),
	m_abort( false )
{
	AudioFileDeviceInstantiaton audioEncoderFactory = fileEncodeDevices[exportFileFormat].m_getDevInst;

	if (audioEncoderFactory)
	{
		bool successful = false;

		m_fileDev = audioEncoderFactory(
					outputFilename, outputSettings, DEFAULT_CHANNELS,
					Engine::mixer(), successful );
		if( !successful )
		{
			delete m_fileDev;
			m_fileDev = NULL;
		}
	}
}




ProjectRenderer::~ProjectRenderer()
{
}




// Little help function for getting file format from a file extension
// (only for registered file-encoders).
ProjectRenderer::ExportFileFormats ProjectRenderer::getFileFormatFromExtension(
							const QString & _ext )
{
	int idx = 0;
	while( fileEncodeDevices[idx].m_fileFormat != NumFileFormats )
	{
		if( QString( fileEncodeDevices[idx].m_extension ) == _ext )
		{
			return( fileEncodeDevices[idx].m_fileFormat );
		}
		++idx;
	}

	return( WaveFile ); // Default.
}




QString ProjectRenderer::getFileExtensionFromFormat(
		ExportFileFormats fmt )
{
	return fileEncodeDevices[fmt].m_extension;
}




void ProjectRenderer::startProcessing()
{

	if( isReady() )
	{
		// Have to do mixer stuff with GUI-thread affinity in order to
		// make slots connected to sampleRateChanged()-signals being called immediately.
		Engine::mixer()->setAudioDevice( m_fileDev,
						m_qualitySettings, false, false );

		start(
#ifndef LMMS_BUILD_WIN32
			QThread::HighPriority
#endif
						);

	}
}


void ProjectRenderer::run()
{
	MemoryManager::ThreadGuard mmThreadGuard; Q_UNUSED(mmThreadGuard);
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

	Engine::getSong()->startExport();
	Engine::getSong()->updateLength();
	// Skip first empty buffer.
	Engine::mixer()->nextBuffer();

	m_progress = 0;

	// Now start processing
	Engine::mixer()->startProcessing(false);

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

	// Notify mixer of the end of processing.
	Engine::mixer()->stopProcessing();

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
	const int cols = 50;
	static int rot = 0;
	char buf[80];
	char prog[cols+1];

	for( int i = 0; i < cols; ++i )
	{
		prog[i] = ( i*100/cols <= m_progress ? '-' : ' ' );
	}
	prog[cols] = 0;

	const char * activity = (const char *) "|/-\\";
	memset( buf, 0, sizeof( buf ) );
	sprintf( buf, "\r|%s|    %3d%%   %c  ", prog, m_progress,
							activity[rot] );
	rot = ( rot+1 ) % 4;

	fprintf( stderr, "%s", buf );
	fflush( stderr );
}


