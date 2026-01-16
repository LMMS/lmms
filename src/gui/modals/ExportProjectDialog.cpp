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

#include "ExportProjectDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "Engine.h"
#include "GuiApplication.h"
#include "MainWindow.h"
#include "ProjectRenderer.h"
#include "Song.h"

namespace lmms::gui {

namespace {
constexpr auto maxCompressionLevel = 8;
constexpr auto maxLoopRepeat = 64;
} // namespace

ExportProjectDialog::ExportProjectDialog(const QString& path, Mode mode, QWidget* parent)
	: QDialog(parent)
	, m_fileFormatSetting(new FileFormatSetting(tr("File format:")))
	, m_sampleRateSetting(new FileFormatSetting(tr("Sampling rate:")))
	, m_bitRateSetting(new FileFormatSetting(tr("Bit rate:")))
	, m_bitDepthSetting(new FileFormatSetting(tr("Bit depth:")))
	, m_stereoModeSetting(new FileFormatSetting(("Stereo mode:")))
	, m_compressionLevelSetting(new FileFormatSetting(tr("Compression level:")))
	, m_exportAsLoopBox(new QCheckBox(tr("Export as loop (remove extra bar)")))
	, m_exportBetweenLoopMarkersBox(new QCheckBox(tr("Export between loop markers")))
	, m_loopRepeatLabel(new QLabel(tr("Render looped section:")))
	, m_loopRepeatBox(new QSpinBox())
	, m_startButton(new QPushButton(tr("Start")))
	, m_cancelButton(new QPushButton(tr("Cancel")))
	, m_progressBar(new QProgressBar())
	, m_path(path)
	, m_mode(mode)
{
	setWindowTitle(tr("Export project"));

	auto mainLayout = new QVBoxLayout(this);

	auto loopRepeatLayout = new QHBoxLayout{};
	loopRepeatLayout->addWidget(m_loopRepeatLabel);
	loopRepeatLayout->addWidget(m_loopRepeatBox);

	auto exportSettingsGroupBox = new QGroupBox(tr("Export settings"));
	auto exportSettingsLayout = new QVBoxLayout{exportSettingsGroupBox};
	exportSettingsLayout->addWidget(m_exportAsLoopBox);
	exportSettingsLayout->addWidget(m_exportBetweenLoopMarkersBox);
	exportSettingsLayout->addLayout(loopRepeatLayout);

	auto fileFormatSettingsGroupBox = new QGroupBox(tr("File format settings"));
	auto fileFormatSettingsLayout = new QVBoxLayout(fileFormatSettingsGroupBox);
	fileFormatSettingsLayout->addWidget(m_fileFormatSetting);
	fileFormatSettingsLayout->addWidget(m_sampleRateSetting);
	fileFormatSettingsLayout->addWidget(m_bitRateSetting);
	fileFormatSettingsLayout->addWidget(m_bitDepthSetting);
	fileFormatSettingsLayout->addWidget(m_stereoModeSetting);
	fileFormatSettingsLayout->addWidget(m_compressionLevelSetting);

	auto startCancelButtonsLayout = new QHBoxLayout{};
	startCancelButtonsLayout->addStretch();
	startCancelButtonsLayout->addWidget(m_startButton);
	startCancelButtonsLayout->addWidget(m_cancelButton);

	mainLayout->addWidget(exportSettingsGroupBox);
	mainLayout->addWidget(fileFormatSettingsGroupBox);
	mainLayout->addStretch();
	mainLayout->addLayout(startCancelButtonsLayout);
	mainLayout->addWidget(m_progressBar);

	connect(m_fileFormatSetting->comboBox(), qOverload<int>(&QComboBox::currentIndexChanged), this,
		&ExportProjectDialog::onFileFormatChanged);
	connect(m_startButton, &QPushButton::clicked, this, &ExportProjectDialog::onStartButtonClicked);
	connect(m_cancelButton, &QPushButton::clicked, this, &ExportProjectDialog::reject);

	auto index = 0;
	for (const auto& device : ProjectRenderer::fileEncodeDevices)
	{
		if (!device.isAvailable()) { continue; }

		m_fileFormatSetting->comboBox()->addItem(tr(device.m_description), static_cast<int>(device.m_fileFormat));

		if (QString::compare(QFileInfo{path}.suffix(), &device.m_extension[1], Qt::CaseInsensitive) == 0)
		{
			m_fileFormatSetting->comboBox()->setCurrentIndex(index);
		}

		++index;
	}

	for (const auto& sampleRate : SUPPORTED_SAMPLERATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(sampleRate), "Hz");
		m_sampleRateSetting->comboBox()->addItem(str, sampleRate);
	}

	for (const auto& bitRate : SUPPORTED_BITRATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(bitRate), "KBit/s");
		m_bitRateSetting->comboBox()->addItem(str, bitRate);
	}

	for (auto i = 0; i < static_cast<int>(OutputSettings::BitDepth::Count); ++i)
	{
		switch (static_cast<OutputSettings::BitDepth>(i))
		{
		case OutputSettings::BitDepth::Depth16Bit:
			m_bitDepthSetting->comboBox()->addItem(tr("16 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth24Bit:
			m_bitDepthSetting->comboBox()->addItem(tr("24 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth32Bit:
			m_bitDepthSetting->comboBox()->addItem(tr("32 Bit float"), i);
			break;
		default:
			assert(false && "invalid or unsupported bit depth");
			break;
		}
	}

	for (auto i = 0; i < static_cast<int>(OutputSettings::StereoMode::Count); ++i)
	{
		switch (static_cast<OutputSettings::StereoMode>(i))
		{
		case OutputSettings::StereoMode::Mono:
			m_stereoModeSetting->comboBox()->addItem(tr("Mono"), i);
			break;
		case OutputSettings::StereoMode::Stereo:
			m_stereoModeSetting->comboBox()->addItem(tr("Stereo"), i);
			break;
		case OutputSettings::StereoMode::JointStereo:
			m_stereoModeSetting->comboBox()->addItem(tr("Joint stereo"), i);
			break;
		default:
			assert(false && "invalid or unsupported stereo mode");
			break;
		}
	}

	for (auto i = 0; i <= maxCompressionLevel; ++i)
	{
		const auto compressionValue = static_cast<float>(i) / maxCompressionLevel;
		switch (i)
		{
		case 0:
			m_compressionLevelSetting->comboBox()->addItem(tr("%1 (Fastest, biggest)").arg(i), compressionValue);
			break;
		case maxCompressionLevel:
			m_compressionLevelSetting->comboBox()->addItem(tr("%1 (Slowest, smallest)").arg(i), compressionValue);
			break;
		default:
			m_compressionLevelSetting->comboBox()->addItem(QString::number(i), compressionValue);
			break;
		}
	}

	m_progressBar->setValue(0);
	m_loopRepeatBox->setMinimum(1);
	m_loopRepeatBox->setMaximum(maxLoopRepeat);
	m_loopRepeatBox->setValue(1);
	m_loopRepeatBox->setSuffix(tr(" time(s)"));
}

void ExportProjectDialog::onFileFormatChanged(int index)
{
	m_sampleRateSetting->setVisible(false);
	m_bitDepthSetting->setVisible(false);
	m_bitRateSetting->setVisible(false);
	m_stereoModeSetting->setVisible(false);
	m_compressionLevelSetting->setVisible(false);

	switch (static_cast<ProjectRenderer::ExportFileFormat>(index))
	{
	case ProjectRenderer::ExportFileFormat::Wave:
		m_sampleRateSetting->setVisible(true);
		m_bitDepthSetting->setVisible(true);
		break;
	case ProjectRenderer::ExportFileFormat::Flac:
		m_sampleRateSetting->setVisible(true);
		m_bitDepthSetting->setVisible(true);
		m_compressionLevelSetting->setVisible(true);
		break;
	case ProjectRenderer::ExportFileFormat::Ogg:
		m_sampleRateSetting->setVisible(true);
		m_bitRateSetting->setVisible(true);
		break;
	case ProjectRenderer::ExportFileFormat::MP3:
		m_stereoModeSetting->setVisible(true);
		m_bitRateSetting->setVisible(true);
	default:
		break;
	}
}

void ExportProjectDialog::onStartButtonClicked()
{
	const auto sampleRate = static_cast<sample_rate_t>(m_sampleRateSetting->comboBox()->currentData().toInt());
	const auto bitRate = static_cast<bitrate_t>(m_bitRateSetting->comboBox()->currentData().toInt());
	const auto bitDepth = static_cast<OutputSettings::BitDepth>(m_bitDepthSetting->comboBox()->currentData().toInt());
	const auto stereoMode
		= static_cast<OutputSettings::StereoMode>(m_stereoModeSetting->comboBox()->currentData().toInt());
	auto outputSettings = OutputSettings{sampleRate, bitRate, bitDepth, stereoMode};

	const auto compressionLevel = m_compressionLevelSetting->comboBox()->currentData().toDouble();
	outputSettings.setCompressionLevel(compressionLevel);

	const auto format
		= static_cast<ProjectRenderer::ExportFileFormat>(m_fileFormatSetting->comboBox()->currentData().toInt());
	m_renderManager = std::make_unique<RenderManager>(outputSettings, format, m_path);
	m_startButton->setEnabled(false);

	Engine::getSong()->setExportLoop(m_exportAsLoopBox->isChecked());
	Engine::getSong()->setRenderBetweenMarkers(m_exportBetweenLoopMarkersBox->isChecked());
	Engine::getSong()->setLoopRenderCount(m_loopRepeatBox->value());

	connect(m_renderManager.get(), &RenderManager::progressChanged, m_progressBar, &QProgressBar::setValue);
	connect(m_renderManager.get(), &RenderManager::progressChanged, this, &ExportProjectDialog::updateTitleBar);
	connect(m_renderManager.get(), &RenderManager::finished, this, &QDialog::accept);
	connect(m_renderManager.get(), &RenderManager::finished, getGUI()->mainWindow(), &MainWindow::resetWindowTitle);

	switch (m_mode)
	{
	case Mode::ExportProject:
		m_renderManager->renderProject();
		break;
	case Mode::ExportTracks:
		m_renderManager->renderTracks();
		break;
	}
}

void ExportProjectDialog::accept()
{
	m_renderManager.reset(nullptr);
	QDialog::accept();
	getGUI()->mainWindow()->resetWindowTitle();

}

void ExportProjectDialog::reject()
{
	if (m_renderManager) { m_renderManager->abortProcessing(); }
	m_renderManager.reset(nullptr);
	QDialog::reject();
}

ExportProjectDialog::FileFormatSetting::FileFormatSetting(const QString& header)
	: m_label(new QLabel(header))
	, m_comboBox(new QComboBox())
{
	m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	auto layout = new QVBoxLayout{this};
	layout->addWidget(m_label);
	layout->addWidget(m_comboBox);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);
}

void ExportProjectDialog::updateTitleBar(int prog)
{
	getGUI()->mainWindow()->setWindowTitle(tr("Rendering: %1%").arg(prog));
}

} // namespace lmms::gui
