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
#include <QMessageBox>

#include "ExportProjectDialog.h"
#include "Song.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "OutputSettings.h"

namespace lmms::gui
{

ExportProjectDialog::ExportProjectDialog( const QString & _file_name,
							QWidget * _parent, bool multi_export=false ) :
	QDialog( _parent ),
	Ui::ExportProjectDialog(),
	m_fileName( _file_name ),
	m_fileExtension(),
	m_multiExport( multi_export ),
	m_renderManager( nullptr )
{
	setupUi( this );
	setWindowTitle( tr( "Export project to %1" ).arg(
					QFileInfo( _file_name ).fileName() ) );

	// Get the extension of the chosen file.
	QStringList parts = _file_name.split( '.' );
	QString fileExt;
	if( parts.size() > 0 )
	{
		fileExt = "." + parts[parts.size()-1];
	}

	int cbIndex = 0;
	for (const auto& format : AudioFileFormats)
	{
		// Get the extension of this format.
		QString renderExt = format.extension.data();

		// Add to combo box.
		fileFormatCB->addItem(QString{"%1 (*%2)"}.arg(format.name.data(), format.extension.data()),
			QVariant(static_cast<int>(format.format)) // Format tag; later used for identification.
		);

		// If this is our extension, select it.
		if( QString::compare( renderExt, fileExt,
								Qt::CaseInsensitive ) == 0 )
		{
			fileFormatCB->setCurrentIndex( cbIndex );
		}

		cbIndex++;
	}

	int const MAX_LEVEL=8;
	for(int i=0; i<=MAX_LEVEL; ++i)
	{
		QString info="";
		if ( i==0 ){ info = tr( "( Fastest - biggest )" ); }
		else if ( i==MAX_LEVEL ){ info = tr( "( Slowest - smallest )" ); }

		compLevelCB->addItem(
			QString::number(i)+" "+info,
			QVariant(i/static_cast<double>(MAX_LEVEL))
		);
	}
	compLevelCB->setCurrentIndex(5);
#ifndef LMMS_HAVE_SF_COMPLEVEL
	// Disable this widget; the setting would be ignored by the renderer.
	compressionWidget->setVisible(false);
#endif

	for (const auto sampleRate : SUPPORTED_SAMPLERATES)
	{
		samplerateCB->addItem(tr("%1 Hz").arg(sampleRate), sampleRate);
	}

	const auto currentIndex = std::max(0, samplerateCB->findData(Engine::audioEngine()->outputSampleRate()));
	samplerateCB->setCurrentIndex(currentIndex);

	connect( startButton, SIGNAL(clicked()),
			this, SLOT(startBtnClicked()));
}


void ExportProjectDialog::reject()
{
	if( m_renderManager ) {
		m_renderManager->abortProcessing();
	}
	m_renderManager.reset(nullptr);

	QDialog::reject();
}



void ExportProjectDialog::accept()
{
	m_renderManager.reset(nullptr);
	QDialog::accept();

	getGUI()->mainWindow()->resetWindowTitle();
}




void ExportProjectDialog::closeEvent( QCloseEvent * _ce )
{
	Engine::getSong()->setLoopRenderCount(1);
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
		return OutputSettings::StereoMode::Mono;
	case 1:
		return OutputSettings::StereoMode::Stereo;
	case 2:
		return OutputSettings::StereoMode::JointStereo;
	default:
		return OutputSettings::StereoMode::Stereo;
	}
}

void ExportProjectDialog::startExport()
{
	const auto bitrates = std::array{64, 128, 160, 192, 256, 320};

	OutputSettings os = OutputSettings(samplerateCB->currentData().toInt(), bitrates[bitrateCB->currentIndex()],
		static_cast<OutputSettings::BitDepth>(depthCB->currentIndex()),
		mapToStereoMode(stereoModeComboBox->currentIndex()));

	if (compressionWidget->isVisible())
	{
		double level = compLevelCB->itemData(compLevelCB->currentIndex()).toDouble();
		os.setCompressionLevel(level);
	}

	// Make sure we have the the correct file extension
	// so there's no confusion about the codec in use.
	auto output_name = m_fileName;
	if (!(m_multiExport || output_name.endsWith(m_fileExtension,Qt::CaseInsensitive)))
	{
		output_name+=m_fileExtension;
	}


	if (auto f = QFile{output_name}; !f.open(QFile::WriteOnly | QFile::Truncate))
	{
		const auto title = tr("Could not open file");
		const auto message = tr("Could not open file %1 "
						  "for writing.\nPlease make "
						  "sure you have write "
						  "permission to the file and "
						  "the directory containing the "
						  "file and try again!")
						   .arg(output_name);
		QMessageBox::critical(nullptr, title, message, QMessageBox::Ok, QMessageBox::NoButton);
	}

	m_renderManager.reset(new RenderManager(os, m_ft, output_name));

	Engine::getSong()->setExportLoop( exportLoopCB->isChecked() );
	Engine::getSong()->setRenderBetweenMarkers( renderMarkersCB->isChecked() );
	Engine::getSong()->setLoopRenderCount(loopCountSB->value());

	connect( m_renderManager.get(), SIGNAL(progressChanged(int)),
			progressBar, SLOT(setValue(int)));
	connect( m_renderManager.get(), SIGNAL(progressChanged(int)),
			this, SLOT(updateTitleBar(int)));
	connect( m_renderManager.get(), SIGNAL(finished()),
			this, SLOT(accept())) ;
	connect( m_renderManager.get(), SIGNAL(finished()),
			getGUI()->mainWindow(), SLOT(resetWindowTitle()));

	if ( m_multiExport )
	{
		m_renderManager->renderTracks();
	}
	else
	{
		m_renderManager->renderProject();
	}
}


void ExportProjectDialog::onFileFormatChanged(int index)
{
	// Extract the format tag from the currently selected item,
	// and adjust the UI properly.
	QVariant format_tag = fileFormatCB->itemData(index);
	bool successful_conversion = false;
	auto exportFormat = static_cast<AudioFileFormat>(
		format_tag.toInt(&successful_conversion)
	);
	Q_ASSERT(successful_conversion);

	bool stereoModeVisible = (exportFormat == AudioFileFormat::MP3);

	bool sampleRateControlsVisible = (exportFormat != AudioFileFormat::MP3);

	bool bitRateControlsEnabled =
			(exportFormat == AudioFileFormat::OGG ||
			 exportFormat == AudioFileFormat::MP3);

	bool bitDepthControlEnabled =
			(exportFormat == AudioFileFormat::WAV ||
			 exportFormat == AudioFileFormat::FLAC);

#ifdef LMMS_HAVE_SF_COMPLEVEL
	bool compressionLevelVisible = (exportFormat == AudioFileFormat::FLAC);
	compressionWidget->setVisible(compressionLevelVisible);
#endif

	stereoModeWidget->setVisible(stereoModeVisible);
	sampleRateWidget->setVisible(sampleRateControlsVisible);

	bitrateWidget->setVisible(bitRateControlsEnabled);

	depthWidget->setVisible(bitDepthControlEnabled);
}

void ExportProjectDialog::startBtnClicked()
{
	// Get file format from current menu selection.
	bool successful_conversion = false;
	QVariant tag = fileFormatCB->itemData(fileFormatCB->currentIndex());
	m_ft = static_cast<AudioFileFormat>(
			tag.toInt(&successful_conversion)
	);

	if( !successful_conversion )
	{
		QMessageBox::information( this, tr( "Error" ),
								  tr( "Error while determining file-encoder device. "
									  "Please try to choose a different output "
									  "format." ) );
		reject();
		return;
	}

	// Find proper file extension.
	for (const auto& format : AudioFileFormats)
	{
		if (m_ft == format.format)
		{
			m_fileExtension = QString(QLatin1String(format.extension.data()));
			break;
		}
	}

	startButton->setEnabled( false );
	progressBar->setEnabled( true );

	updateTitleBar( 0 );

	startExport();
}




void ExportProjectDialog::updateTitleBar( int _prog )
{
	getGUI()->mainWindow()->setWindowTitle(
					tr( "Rendering: %1%" ).arg( _prog ) );
}

} // namespace lmms::gui
