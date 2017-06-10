/*
 * ExportProjectDialog.cpp - implementation of dialog for exporting project
 *
 * Copyright (c) 2004-2013 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

#include "ExportProjectDialog.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "OutputSettings.h"


ExportProjectDialog::ExportProjectDialog( const QString & _file_name,
							QWidget * _parent, bool multi_export=false ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_fileExtension(),
	m_multiExport( multi_export ),
	m_renderManager( NULL )
{
	setupUi( this );
	setWindowTitle( tr( "Export project to %1" ).arg(
					QFileInfo( _file_name ).fileName() ) );

	// get the extension of the chosen file
	QStringList parts = _file_name.split( '.' );
	QString fileExt;
	if( parts.size() > 0 )
	{
		fileExt = "." + parts[parts.size()-1];
	}

	int cbIndex = 0;
	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( ProjectRenderer::fileEncodeDevices[i].isAvailable() )
		{
			// get the extension of this format
			QString renderExt = ProjectRenderer::fileEncodeDevices[i].m_extension;

			// add to combo box
			fileFormatCB->addItem( ProjectRenderer::tr(
				ProjectRenderer::fileEncodeDevices[i].m_description ) );

			// if this is our extension, select it
			if( QString::compare( renderExt, fileExt,
									Qt::CaseInsensitive ) == 0 )
			{
				fileFormatCB->setCurrentIndex( cbIndex );
			}

			cbIndex++;
		}
	}

	connect( startButton, SIGNAL( clicked() ),
			this, SLOT( startBtnClicked() ) );
}




ExportProjectDialog::~ExportProjectDialog()
{
	delete m_renderManager;
}




void ExportProjectDialog::reject()
{
	if( m_renderManager ) {
		m_renderManager->abortProcessing();
	}

	delete m_renderManager;
	m_renderManager = NULL;

	QDialog::reject();
}



void ExportProjectDialog::accept()
{
	delete m_renderManager;
	m_renderManager = NULL;

	QDialog::accept();
}




void ExportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	if( m_renderManager ) {
		m_renderManager->abortProcessing();
	}

	QDialog::closeEvent( _ce );
}


OutputSettings::StereoMode mapToStereoMode(int index)
{
	switch (index)
	{
	case 0:
		return OutputSettings::StereoMode_Stereo;
	case 1:
		return OutputSettings::StereoMode_JointStereo;
	case 2:
		return OutputSettings::StereoMode_Mono;
	default:
		return OutputSettings::StereoMode_Stereo;
	}
}

void ExportProjectDialog::startExport()
{
	Mixer::qualitySettings qs =
			Mixer::qualitySettings(
					static_cast<Mixer::qualitySettings::Interpolation>(interpolationCB->currentIndex()),
					static_cast<Mixer::qualitySettings::Oversampling>(oversamplingCB->currentIndex()) );

	const int samplerates[5] = { 44100, 48000, 88200, 96000, 192000 };
	const bitrate_t bitrates[6] = { 64, 128, 160, 192, 256, 320 };

	bool useVariableBitRate = checkBoxVariableBitRate->isChecked();

	OutputSettings::BitRateSettings bitRateSettings(bitrates[ bitrateCB->currentIndex() ], useVariableBitRate);
	OutputSettings os = OutputSettings(
			samplerates[ samplerateCB->currentIndex() ],
			bitRateSettings,
			static_cast<OutputSettings::BitDepth>( depthCB->currentIndex() ),
			mapToStereoMode(stereoModeComboBox->currentIndex()) );

	m_renderManager = new RenderManager( qs, os, m_ft, m_fileName );

	Engine::getSong()->setExportLoop( exportLoopCB->isChecked() );
	Engine::getSong()->setRenderBetweenMarkers( renderMarkersCB->isChecked() );

	connect( m_renderManager, SIGNAL( progressChanged( int ) ),
			progressBar, SLOT( setValue( int ) ) );
	connect( m_renderManager, SIGNAL( progressChanged( int ) ),
			this, SLOT( updateTitleBar( int ) )) ;
	connect( m_renderManager, SIGNAL( finished() ),
			this, SLOT( accept() ) );
	connect( m_renderManager, SIGNAL( finished() ),
			gui->mainWindow(), SLOT( resetWindowTitle() ) );

	if ( m_multiExport )
	{
		m_renderManager->renderTracks();
	}
	else
	{
		m_renderManager->renderProject();
	}
}


ProjectRenderer::ExportFileFormats convertIndexToExportFileFormat(int index)
{
	switch (index)
	{
	case 0:
		return ProjectRenderer::WaveFile;
	case 1:
		return ProjectRenderer::OggFile;
	case 2:
		return ProjectRenderer::MP3File;
	default:
		Q_ASSERT(false);
		break;
	}

	return ProjectRenderer::NumFileFormats;
}


void ExportProjectDialog::onFileFormatChanged(int index)
{
	ProjectRenderer::ExportFileFormats exportFormat =
			convertIndexToExportFileFormat(index);

	bool stereoModeVisible = exportFormat == ProjectRenderer::MP3File;

	bool sampleRateControlsVisible = exportFormat != ProjectRenderer::MP3File;

	bool bitRateControlsEnabled =
			(exportFormat == ProjectRenderer::OggFile ||
			 exportFormat == ProjectRenderer::MP3File);

	bool bitDepthControlEnabled = exportFormat == ProjectRenderer::WaveFile;

	bool variableBitrateVisible = exportFormat != ProjectRenderer::MP3File;

	stereoModeWidget->setVisible(stereoModeVisible);
	sampleRateWidget->setVisible(sampleRateControlsVisible);

	bitrateWidget->setVisible(bitRateControlsEnabled);
	checkBoxVariableBitRate->setVisible(variableBitrateVisible);

	depthWidget->setVisible(bitDepthControlEnabled);
}

void ExportProjectDialog::startBtnClicked()
{
	m_ft = ProjectRenderer::NumFileFormats;

	for( int i = 0; i < ProjectRenderer::NumFileFormats; ++i )
	{
		if( fileFormatCB->currentText() ==
			ProjectRenderer::tr(
				ProjectRenderer::fileEncodeDevices[i].m_description ) )
		{
			m_ft = ProjectRenderer::fileEncodeDevices[i].m_fileFormat;
			m_fileExtension = QString( QLatin1String( ProjectRenderer::fileEncodeDevices[i].m_extension ) );
			break;
		}
	}

	if( m_ft == ProjectRenderer::NumFileFormats )
	{
		QMessageBox::information( this, tr( "Error" ),
			tr( "Error while determining file-encoder device. "
				"Please try to choose a different output "
							"format." ) );
		reject();
		return;
	}

	startButton->setEnabled( false );
	progressBar->setEnabled( true );

	updateTitleBar( 0 );

	startExport();
}




void ExportProjectDialog::updateTitleBar( int _prog )
{
	gui->mainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}
