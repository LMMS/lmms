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


ProjectRenderer::ProjectRenderer(
					const AudioOutputContext::QualitySettings & _qs,
					const EncoderSettings & es,
					ExportFileFormats fileFormat,
					const QString & outFile ) :
	QThread( engine::getMixer() ),
	m_fileDev( NULL ),
	m_progress( 0 ),
	m_abort( false )
{
	m_context = new AudioOutputContext( engine::getMixer(),
										NULL,
										_qs );
	if( __fileEncodeDevices[fileFormat].m_getDevInst == NULL )
	{
		return;
	}

	bool success_ful = false;
	m_fileDev = __fileEncodeDevices[fileFormat].m_getDevInst(
				es.samplerate, DEFAULT_CHANNELS, success_ful,
				outFile, es.vbr,
				es.bitrate, es.bitrate - 64, es.bitrate + 64,
				es.depth == Depth_32Bit ? 32 :
					( es.depth == Depth_24Bit ? 24 : 16 ),
							m_context );
	if( success_ful == false )
	{
		delete m_fileDev;
		m_fileDev = NULL;
	}

	m_context->setAudioBackend( m_fileDev );

}




ProjectRenderer::~ProjectRenderer()
{
	delete m_fileDev;
	delete m_context;
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
			return __fileEncodeDevices[idx].m_fileFormat;
		}
		++idx;
	}

	return WaveFile;	// default
}




void ProjectRenderer::startProcessing()
{
	if( isReady() )
	{
		connect( this, SIGNAL( finished() ), this, SLOT( finishProcessing() ) );

		// have to do mixer stuff with GUI-thread-affinity in order to
		// make slots connected to sampleRateChanged()-signals being
		// called immediately
		engine::mixer()->setAudioOutputContext( m_context );

		start();
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

    if( m_fileDev == NULL )
	{
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

	// have to lock Mixer when touching Song's state as the FIFO writer thread
	// may call Mixer::renderNextBuffer() (which calls Song::doActions())
	// simultaneously
	engine::mixer()->lock();
	engine::getSong()->startExport();
	engine::mixer()->unlock();

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

	engine::mixer()->lock();
	engine::getSong()->stopExport();
	engine::mixer()->unlock();
}




void ProjectRenderer::finishProcessing()
{
	const QString f = m_fileDev->outputFile();

	engine::mixer()->setAudioOutputContext(
								engine::mixer()->defaultAudioOutputContext() );

	// if the user aborted export-process, the file has to be deleted
	if( m_abort )
	{
		QFile( f ).remove();
	}
}



#include "moc_ProjectRenderer.cxx"

