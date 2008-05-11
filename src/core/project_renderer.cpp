/*
 * project_renderer.cpp - projectRenderer-class for easily rendering projects
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


struct fileEncodeDevice
{
	projectRenderer::ExportFileTypes m_fileType;
//		const char * m_description;
	const char * m_extension;
	audioFileDeviceInstantiaton m_getDevInst;
} ;


static fileEncodeDevice __fileEncodeDevices[] =
{

	{ projectRenderer::WaveFile,/* QT_TRANSLATE_NOOP( "exportProjectDialog",
					"Uncompressed Wave-File (*.wav)" ),*/
					".wav", &audioFileWave::getInst },
#ifdef HAVE_VORBIS_CODEC_H
	{ projectRenderer::OggFile, /*QT_TRANSLATE_NOOP( "exportProjectDialog",
					"Compressed OGG-File (*.ogg)" ),*/
					".ogg", &audioFileOgg::getInst },
#endif
	// ... insert your own file-encoder-infos here... may be one day the
	// user can add own encoders inside the program...

	{ projectRenderer::NullFile, NULL, NULL }

} ;




projectRenderer::projectRenderer( const mixer::qualitySettings & _qs,
					const outputSettings & _os,
					ExportFileTypes _file_type,
					const QString & _out_file ) :
	QThread( engine::getMixer() ),
	m_qualitySettings( _qs ),
	m_oldQualitySettings( engine::getMixer()->currentQualitySettings() ),
	m_abort( FALSE )
{
	int idx = 0;
	while( __fileEncodeDevices[idx].m_fileType != NullFile )
	{
		if( __fileEncodeDevices[idx].m_fileType == _file_type )
		{
			break;
		}
		++idx;
	}

	if( __fileEncodeDevices[idx].m_fileType == NullFile )
	{
		return;
	}

	bool success_ful = FALSE;
	m_fileDev = __fileEncodeDevices[idx].m_getDevInst(
				_os.samplerate, DEFAULT_CHANNELS, success_ful,
				_out_file, _os.vbr,
				_os.bitrate, _os.bitrate - 64, _os.bitrate + 64,
							engine::getMixer() );
	if( success_ful == FALSE )
	{
/*		QMessageBox::information( this,
					tr( "Export failed" ),
					tr( "The project-export failed, "
						"because the output-file/-"
						"device could not be opened.\n"
						"Make sure, you have write "
						"access to the selected "
						"file/device!" ),
							QMessageBox::Ok );
		return;*/
	}

}




projectRenderer::~projectRenderer()
{
}




// little help-function for getting file-type from a file-extension (only for
// registered file-encoders)
projectRenderer::ExportFileTypes projectRenderer::getFileTypeFromExtension(
							const QString & _ext )
{
	int idx = 0;
	while( __fileEncodeDevices[idx].m_fileType != NullFile )
	{
		if( QString( __fileEncodeDevices[idx].m_extension ) == _ext )
		{
			return( __fileEncodeDevices[idx].m_fileType );
		}
		++idx;
	}

	return( WaveFile );	// default
}




void projectRenderer::startProcessing( void )
{
	// have to do mixer stuff with GUI-thread-affinity in order to make
	// slots connected to sampleRateChanged()-signals being called
	// immediately
	engine::getMixer()->setAudioDevice( m_fileDev, m_qualitySettings );

	start( QThread::HighestPriority );
}




void projectRenderer::run( void )
{
	engine::getSong()->startExport();

	song::playPos & pp = engine::getSong()->getPlayPos(
							song::Mode_PlaySong );
	int progress = 0;
	while( engine::getSong()->isExportDone() == FALSE &&
				engine::getSong()->isExporting() == TRUE
							&& !m_abort )
	{
		m_fileDev->processNextBuffer();
		const int nprog = pp * 100 /
				( ( engine::getSong()->length() + 1 ) * 192 );
		if( progress != nprog )
		{
			progress = nprog;
			emit progressChanged( progress );
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




#include "project_renderer.moc"

