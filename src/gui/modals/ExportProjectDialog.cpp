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

#include <QFileInfo>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "ProjectRenderer.h"

namespace lmms::gui {

ExportProjectDialog::ExportProjectDialog(const QString& path, QWidget* parent, bool multiExport)
	: QDialog(parent)
	, m_fileFormatSetting(new FileFormatSetting(tr("File format:")))
	, m_sampleRateSetting(new FileFormatSetting(tr("Sampling rate:")))
	, m_bitRateSetting(new FileFormatSetting(tr("Bit rate:")))
	, m_bitDepthSetting(new FileFormatSetting(tr("Bit depth:")))
	, m_stereoModeSetting(new FileFormatSetting(("Stereo mode:")))
	, m_compressionLevelSetting(new FileFormatSetting(tr("Compression level:")))
	, m_startButton(new QPushButton(tr("Start")))
	, m_cancelButton(new QPushButton(tr("Cancel")))
	, m_progressBar(new QProgressBar())
{
	setWindowTitle(tr("Export project to %1").arg(QFileInfo(path).fileName()));

	auto mainLayout = new QVBoxLayout(this);
	auto startCancelButtonsLayout = new QHBoxLayout{};

	auto fileFormatSettingsGroupBox = new QGroupBox(tr("File format settings"));
	auto fileFormatSettingsLayout = new QVBoxLayout(fileFormatSettingsGroupBox);

	fileFormatSettingsLayout->addWidget(m_fileFormatSetting);
	fileFormatSettingsLayout->addWidget(m_sampleRateSetting);
	fileFormatSettingsLayout->addWidget(m_bitRateSetting);
	fileFormatSettingsLayout->addWidget(m_bitDepthSetting);
	fileFormatSettingsLayout->addWidget(m_stereoModeSetting);
	fileFormatSettingsLayout->addWidget(m_compressionLevelSetting);

	startCancelButtonsLayout->addStretch();
	startCancelButtonsLayout->addWidget(m_startButton);
	startCancelButtonsLayout->addWidget(m_cancelButton);

	mainLayout->addWidget(fileFormatSettingsGroupBox);
	mainLayout->addStretch();
	mainLayout->addLayout(startCancelButtonsLayout);
	mainLayout->addWidget(m_progressBar);

	connect(m_fileFormatSetting->comboBox(), qOverload<int>(&QComboBox::currentIndexChanged), this,
		&ExportProjectDialog::onFileFormatChanged);

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
		case OutputSettings::StereoMode::Stereo:
			m_stereoModeSetting->comboBox()->addItem(tr("Stereo"), i);
			break;
		case OutputSettings::StereoMode::JointStereo:
			m_stereoModeSetting->comboBox()->addItem(tr("Joint stereo"), i);
			break;
		case OutputSettings::StereoMode::Mono:
			m_stereoModeSetting->comboBox()->addItem(tr("Mono"), i);
			break;
		default:
			assert(false && "invalid or unsupported stereo mode");
			break;
		}
	}

	constexpr auto maxCompressionLevel = 8;
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

} // namespace lmms::gui
