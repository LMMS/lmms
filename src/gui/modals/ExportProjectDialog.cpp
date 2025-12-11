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

ExportProjectDialog::ExportProjectDialog(const QString& path, RenderManager::Mode mode, QWidget* parent)
	: QDialog(parent)
	, Ui::ExportProjectDialog()
	, m_path(path)
	, m_mode(mode)
	, m_renderManager(nullptr)
{
	setupUi( this );
	setWindowTitle(tr("Export project to %1").arg(QFileInfo(path).fileName()));

	for (const auto& fileEncodeDevice : ProjectRenderer::fileEncodeDevices)
	{
		if (!fileEncodeDevice.isAvailable()) { continue; }
		fileFormatCB->addItem(
			fileEncodeDevice.m_description, QVariant(static_cast<int>(fileEncodeDevice.m_fileFormat)));
	}

	// When exporting either the project or a single track, a file name with an extension is expected, so we can fetch
	// it to update the file format box automatically
	if (mode == RenderManager::Mode::ExportProject || mode == RenderManager::Mode::ExportTrack)
	{
		auto extension = QFileInfo{path}.completeSuffix();
		if (extension.isEmpty()) { extension = ProjectRenderer::fileEncodeDevices[0].m_extension; }

		for (auto i = 0; i < fileFormatCB->count(); ++i)
		{
			const auto comboBoxExtension = ProjectRenderer::getFileExtensionFromFormat(
				static_cast<ProjectRenderer::ExportFileFormat>(fileFormatCB->itemData(i).toInt())).remove(0, 1);
			if (QString::compare(extension, comboBoxExtension) != 0) { continue; }
			fileFormatCB->setCurrentIndex(i);
		}
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

	m_renderManager.reset(new RenderManager(os, m_ft, m_path));

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

	switch (m_mode)
	{
	case RenderManager::Mode::ExportProject:
		m_renderManager->renderProject();
		break;
	case RenderManager::Mode::ExportTracks:
		m_renderManager->renderTracks();
		break;
	case RenderManager::Mode::ExportTrack:
		// TODO: Implement
		break;
	}
}


void ExportProjectDialog::onFileFormatChanged(int index)
{
	// Extract the format tag from the currently selected item,
	// and adjust the UI properly.
	QVariant format_tag = fileFormatCB->itemData(index);
	bool successful_conversion = false;
	auto exportFormat = static_cast<ProjectRenderer::ExportFileFormat>(
		format_tag.toInt(&successful_conversion)
	);
	Q_ASSERT(successful_conversion);

	bool stereoModeVisible = (exportFormat == ProjectRenderer::ExportFileFormat::MP3);

	bool sampleRateControlsVisible = (exportFormat != ProjectRenderer::ExportFileFormat::MP3);

	bool bitRateControlsEnabled =
			(exportFormat == ProjectRenderer::ExportFileFormat::Ogg ||
			 exportFormat == ProjectRenderer::ExportFileFormat::MP3);

	bool bitDepthControlEnabled =
			(exportFormat == ProjectRenderer::ExportFileFormat::Wave ||
			 exportFormat == ProjectRenderer::ExportFileFormat::Flac);

#ifdef LMMS_HAVE_SF_COMPLEVEL
	bool compressionLevelVisible = (exportFormat == ProjectRenderer::ExportFileFormat::Flac);
	compressionWidget->setVisible(compressionLevelVisible);
#endif

	stereoModeWidget->setVisible(stereoModeVisible);
	sampleRateWidget->setVisible(sampleRateControlsVisible);

	bitrateWidget->setVisible(bitRateControlsEnabled);

	depthWidget->setVisible(bitDepthControlEnabled);
}

void ExportProjectDialog::startBtnClicked()
{
	m_ft = ProjectRenderer::ExportFileFormat::Count;

	// Get file format from current menu selection.
	bool successful_conversion = false;
	QVariant tag = fileFormatCB->itemData(fileFormatCB->currentIndex());
	m_ft = static_cast<ProjectRenderer::ExportFileFormat>(
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
