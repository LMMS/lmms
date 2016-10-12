/*
 * ProjectRenderer.cpp - ProjectRenderer-class for easily rendering projects
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QFile>

#include "ProjectRenderer.h"
#include "Song.h"
#include "Engine.h"

#include "AudioFileWave.h"
#include "AudioFileOgg.h"

#ifdef LMMS_HAVE_SCHED_H
#include <sched.h>
#endif
#include <QMutexLocker>

const ProjectRenderer::FileEncodeDevice ProjectRenderer::fileEncodeDevices[] =
{

	{
		ProjectRenderer::WaveFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "WAV-File (*.wav)" ),
		".wav", &AudioFileWave::getInst
	},
	{
		ProjectRenderer::OggFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "Compressed OGG-File (*.ogg)" ),
		".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
		& AudioFileOgg::getInst
#else
		NULL
#endif
	},
	// ... insert your own file-encoder-infos here... may be one day the
	// user can add own encoders inside the program...

	{ ProjectRenderer::NumFileFormats, NULL, NULL, NULL }

} ;




ProjectRenderer::ProjectRenderer( const Mixer::qualitySettings & _qs,
				  const OutputSettings & _os,
				  ExportFileFormats _file_format,
				  const QString & _out_file ) :
	QThread( Engine::mixer() ),
	m_fileDev( NULL ),
	m_qualitySettings( _qs ),
	m_oldQualitySettings( Engine::mixer()->currentQualitySettings() ),
	m_progress( 0 ),
	m_abort( false )
{
	if( fileEncodeDevices[_file_format].m_getDevInst == NULL )
	{
		return;
	}

	bool success_ful = false;
	m_fileDev = fileEncodeDevices[_file_format].m_getDevInst(
			    _os.samplerate, DEFAULT_CHANNELS, success_ful,
			    _out_file, _os.vbr,
			    _os.bitrate, _os.bitrate - 64, _os.bitrate + 64,
			    _os.depth == Depth_32Bit ? 32 : 16,
			    Engine::mixer() );

	if( success_ful == false )
	{
		delete m_fileDev;
		m_fileDev = NULL;
	}
}




ProjectRenderer::~ProjectRenderer()
{
}




// little help-function for getting file-format from a file-extension (only for
// registered file-encoders)
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

	return( WaveFile );	// default
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
		// have to do mixer stuff with GUI-thread-affinity in order to
		// make slots connected to sampleRateChanged()-signals being
		// called immediately
		Engine::mixer()->setAudioDevice( m_fileDev,
						 m_qualitySettings, false );
		start(
#ifndef LMMS_BUILD_WIN32
			QThread::HighPriority
#endif
		);
	}
}


void ProjectRenderer::run()
{
#if 0
#ifdef LMMS_BUILD_LINUX
#ifdef LMMS_HAVE_SCHED_H
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	sched_setaffinity( 0, sizeof( mask ), &mask );
#endif
#endif
#endif
	Engine::getSong()->startExport();
	//skip first empty buffer
	Engine::mixer()->nextBuffer();
	const Song::PlayPos & exportPos = Engine::getSong()->getPlayPos(
			Song::Mode_PlaySong );
	m_progress = 0;
	std::pair<MidiTime, MidiTime> exportEndpoints = Engine::getSong()->getExportEndpoints();
	tick_t startTick = exportEndpoints.first.getTicks();
	tick_t lengthTicks = exportEndpoints.second.getTicks() - startTick;

	// Continually track and emit progress percentage to listeners
	while( Engine::getSong()->isExportDone() == false &&
			Engine::getSong()->isExporting() == true
			&& !m_abort )
	{
		m_fileDev->processNextBuffer();
		const int nprog = lengthTicks == 0 ? 100 : ( exportPos.getTicks() - startTick ) * 100 / lengthTicks;

		if( m_progress != nprog )
		{
			m_progress = nprog;
			emit progressChanged( m_progress );
		}
	}

	Engine::getSong()->stopExport();
	const QString f = m_fileDev->outputFile();
	Engine::mixer()->restoreAudioDevice();  // also deletes audio-dev
	Engine::mixer()->changeQuality( m_oldQualitySettings );

	// if the user aborted export-process, the file has to be deleted
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
	char prog[cols + 1];

	for( int i = 0; i < cols; ++i )
	{
		prog[i] = ( i * 100 / cols <= m_progress ? '-' : ' ' );
	}

	prog[cols] = 0;
	const char * activity = ( const char * ) "|/-\\";
	memset( buf, 0, sizeof( buf ) );
	sprintf( buf, "\r|%s|    %3d%%   %c  ", prog, m_progress,
		 activity[rot] );
	rot = ( rot + 1 ) % 4;
	fprintf( stderr, "%s", buf );
	fflush( stderr );
}



