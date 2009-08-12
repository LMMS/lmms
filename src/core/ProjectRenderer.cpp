/*
 * ProjectRenderer.cpp - ProjectRenderer-class for easily rendering projects
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QFile>
#include <QTimer>

#include "ProjectRenderer.h"
#include "song.h"
#include "engine.h"

#include "AudioFileWave.h"
#include "AudioFileOgg.h"
#include "AudioFileMp3.h"
#include "AudioFileFlac.h"

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif


FileEncodeDevice __fileEncodeDevices[] =
{

	{ ProjectRenderer::WaveFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "WAV-File (*.wav)" ),
					".wav", &AudioFileWave::getInst },
	{ ProjectRenderer::OggFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "Compressed OGG-File (*.ogg)" ),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&AudioFileOgg::getInst
#else
					NULL
#endif
									},
    { ProjectRenderer::Mp3File,
        QT_TRANSLATE_NOOP( "ProjectRenderer", "MP3 File (*.mp3)" ),
        ".mp3", &AudioFileMp3::getInst },
	{ ProjectRenderer::FlacFile,
		QT_TRANSLATE_NOOP( "ProjectRenderer", "FLAC File (*.flac)" ),
		".flac", 
#ifdef LMMS_HAVE_FLAC
					&AudioFileFlac::getInst 
#else
					NULL
#endif
									},
	// ... insert your own file-encoder-infos here... may be one day the
	// user can add own encoders inside the program...

	{ ProjectRenderer::NumFileFormats, NULL, NULL, NULL }

} ;

const char * ProjectRenderer::EFF_ext[] = {"wav", "ogg", "mp3", "flac"};


ProjectRenderer::ProjectRenderer( const mixer::qualitySettings & _qs,
					const OutputSettings & _os,
					ExportFileFormats _file_format,
					const QString & _out_file ) :
	QThread( engine::getMixer() ),
	m_fileDev( NULL ),
	m_qualitySettings( _qs ),
	m_oldQualitySettings( engine::getMixer()->currentQualitySettings() ),
	m_progress( 0 ),
	m_abort( false )
{
	if( __fileEncodeDevices[_file_format].m_getDevInst == NULL )
	{
		return;
	}

	bool success_ful = false;
	m_fileDev = __fileEncodeDevices[_file_format].m_getDevInst(
				_os.samplerate, DEFAULT_CHANNELS, success_ful,
				_out_file, _os.vbr,
				_os.bitrate, _os.bitrate - 64, _os.bitrate + 64,
				_os.depth == Depth_32Bit ? 32 :
					( _os.depth == Depth_24Bit ? 24 : 16 ),
							engine::getMixer() );
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
	while( __fileEncodeDevices[idx].m_fileFormat != NumFileFormats )
	{
		if( QString( __fileEncodeDevices[idx].m_extension ) == _ext )
		{
			return( __fileEncodeDevices[idx].m_fileFormat );
		}
		++idx;
	}

	return( WaveFile );	// default
}




void ProjectRenderer::startProcessing()
{
	if( isReady() )
	{
		// have to do mixer stuff with GUI-thread-affinity in order to
		// make slots connected to sampleRateChanged()-signals being
		// called immediately
		engine::getMixer()->setAudioDevice( m_fileDev,
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
#ifdef LMMS_HAVE_PTHREAD_H
	cpu_set_t mask;
	CPU_ZERO( &mask );
	CPU_SET( 0, &mask );
	pthread_setaffinity_np( pthread_self(), sizeof( mask ), &mask );
#endif
#endif
#endif

	engine::getSong()->startExport();

	song::playPos & pp = engine::getSong()->getPlayPos(
							song::Mode_PlaySong );
	m_progress = 0;
	const int sl = ( engine::getSong()->length() + 1 ) * 192;

	while( engine::getSong()->isExportDone() == false &&
				engine::getSong()->isExporting() == true
							&& !m_abort )
	{
		m_fileDev->processNextBuffer();
		const int nprog = pp * 100 / sl;
		if( m_progress != nprog )
		{
			m_progress = nprog;
			emit progressChanged( m_progress );
		}
	}

	engine::getSong()->stopExport();

	const QString f = m_fileDev->outputFile();

	engine::getMixer()->restoreAudioDevice();  // also deletes audio-dev
	engine::getMixer()->changeQuality( m_oldQualitySettings );

	// if the user aborted export-process, the file has to be deleted
	if( m_abort )
	{
		QFile( f ).remove();
	}
}




void ProjectRenderer::abortProcessing()
{
	m_abort = true;
}



void ProjectRenderer::updateConsoleProgress()
{
	const int cols = 50;
	static int rot = 0;
	char buf[80];
	char prog[cols+1];

    if( m_fileDev == NULL ){
        qWarning("Error occured. Aborting render.");
        m_consoleUpdateTimer->stop();
        delete m_consoleUpdateTimer;
        // TODO: kill the program. I can't figure out how to do it...
        return;
    }

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



#include "moc_ProjectRenderer.cxx"

