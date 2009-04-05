/*
 * project_renderer.cpp - projectRenderer-class for easily rendering projects
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

#include "project_renderer.h"
#include "song.h"
#include "engine.h"

#include "audio_file_wave.h"
#include "audio_file_ogg.h"

#ifdef LMMS_HAVE_PTHREAD_H
#include <pthread.h>
#endif



fileEncodeDevice __fileEncodeDevices[] =
{

	{ projectRenderer::WaveFile,
		QT_TRANSLATE_NOOP( "projectRenderer", "WAV-File (*.wav)" ),
					".wav", &audioFileWave::getInst },
	{ projectRenderer::OggFile,
		QT_TRANSLATE_NOOP( "projectRenderer", "Compressed OGG-File (*.ogg)" ),
					".ogg",
#ifdef LMMS_HAVE_OGGVORBIS
					&audioFileOgg::getInst
#else
					NULL
#endif
									},
	// ... insert your own file-encoder-infos here... may be one day the
	// user can add own encoders inside the program...

	{ projectRenderer::NumFileFormats, NULL, NULL, NULL }

} ;




projectRenderer::projectRenderer( const mixer::qualitySettings & _qs,
					const outputSettings & _os,
					ExportFileFormats _file_format,
					const QString & _out_file ) :
	QThread( engine::getMixer() ),
	m_fileDev( NULL ),
	m_qualitySettings( _qs ),
	m_oldQualitySettings( engine::getMixer()->currentQualitySettings() ),
	m_progress( 0 ),
	m_abort( FALSE )
{
	if( __fileEncodeDevices[_file_format].m_getDevInst == NULL )
	{
		return;
	}

	bool success_ful = FALSE;
	m_fileDev = __fileEncodeDevices[_file_format].m_getDevInst(
				_os.samplerate, DEFAULT_CHANNELS, success_ful,
				_out_file, _os.vbr,
				_os.bitrate, _os.bitrate - 64, _os.bitrate + 64,
				_os.depth == Depth_32Bit ? 32 : (Depth_24Bit ? 24 : 16),
							engine::getMixer() );
	if( success_ful == FALSE )
	{
		delete m_fileDev;
		m_fileDev = NULL;
	}

}




projectRenderer::~projectRenderer()
{
}




// little help-function for getting file-format from a file-extension (only for
// registered file-encoders)
projectRenderer::ExportFileFormats projectRenderer::getFileFormatFromExtension(
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




void projectRenderer::startProcessing( void )
{
	if( isReady() )
	{
		// have to do mixer stuff with GUI-thread-affinity in order to
		// make slots connected to sampleRateChanged()-signals being
		// called immediately
		engine::getMixer()->setAudioDevice( m_fileDev,
						m_qualitySettings, FALSE );

		start(
#ifndef LMMS_BUILD_WIN32
			QThread::HighPriority
#endif
						);
	}
}



void projectRenderer::run( void )
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

	while( engine::getSong()->isExportDone() == FALSE &&
				engine::getSong()->isExporting() == TRUE
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




void projectRenderer::abortProcessing( void )
{
	m_abort = TRUE;
}



void projectRenderer::updateConsoleProgress( void )
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



#include "moc_project_renderer.cxx"

