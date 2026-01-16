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

#include <QComboBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

#include "ProjectRenderer.h"

namespace lmms::gui {

ExportProjectDialog::ExportProjectDialog(const QString& path, QWidget* parent, bool multiExport)
	: QDialog(parent)
{
	setWindowTitle(tr("Export project to %1").arg(QFileInfo(path).fileName()));

	auto mainLayout = new QVBoxLayout(this);

	auto fileFormatSettingsGroupBox = new QGroupBox(tr("File format settings"));
	auto fileFormatSettingsLayout = new QVBoxLayout(fileFormatSettingsGroupBox);

	auto fileFormatLabel = new QLabel(tr("File format:"));
	auto fileFormatComboBox = new QComboBox();

	auto sampleRateLabel = new QLabel(tr("Sampling rate:"));
	auto sampleRateComboBox = new QComboBox();

	auto bitRateLabel = new QLabel(tr("Bit rate:"));
	auto bitRateComboBox = new QComboBox();

	auto bitDepthLabel = new QLabel(tr("Bit depth:"));
	auto bitDepthComboBox = new QComboBox();

	auto stereoModeLabel = new QLabel(tr("Stereo mode:"));
	auto stereoModeComboBox = new QComboBox();

	auto compressionLevelLabel = new QLabel(tr("Compression level:"));
	auto compressionLevelComboBox = new QComboBox();

	auto startCancelButtonsLayout = new QHBoxLayout{};
	auto startButton = new QPushButton(tr("Start"));
	auto cancelButton = new QPushButton(tr("Cancel"));

	auto progressBar = new QProgressBar{};
	progressBar->setValue(0);

	fileFormatSettingsLayout->addWidget(fileFormatLabel);
	fileFormatSettingsLayout->addWidget(fileFormatComboBox);

	fileFormatSettingsLayout->addWidget(sampleRateLabel);
	fileFormatSettingsLayout->addWidget(sampleRateComboBox);

	fileFormatSettingsLayout->addWidget(bitRateLabel);
	fileFormatSettingsLayout->addWidget(bitRateComboBox);

	fileFormatSettingsLayout->addWidget(bitDepthLabel);
	fileFormatSettingsLayout->addWidget(bitDepthComboBox);

	fileFormatSettingsLayout->addWidget(stereoModeLabel);
	fileFormatSettingsLayout->addWidget(stereoModeComboBox);

	fileFormatSettingsLayout->addWidget(compressionLevelLabel);
	fileFormatSettingsLayout->addWidget(compressionLevelComboBox);

	startCancelButtonsLayout->addStretch();
	startCancelButtonsLayout->addWidget(startButton);
	startCancelButtonsLayout->addWidget(cancelButton);

	mainLayout->addWidget(fileFormatSettingsGroupBox);
	mainLayout->addLayout(startCancelButtonsLayout);
	mainLayout->addWidget(progressBar);

	for (const auto& device : ProjectRenderer::fileEncodeDevices)
	{
		if (!device.isAvailable()) { continue; }	
		fileFormatComboBox->addItem(tr(device.m_description), static_cast<int>(device.m_fileFormat));
	}

	for (const auto& sampleRate : SUPPORTED_SAMPLERATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(sampleRate), "Hz");
		sampleRateComboBox->addItem(str, sampleRate);
	}

	for (const auto& bitRate : SUPPORTED_BITRATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(bitRate), "KBit/s");
		bitRateComboBox->addItem(str, bitRate);
	}

	for (auto i = 0; i < static_cast<int>(OutputSettings::BitDepth::Count); ++i)
	{
		switch (static_cast<OutputSettings::BitDepth>(i))
		{
		case OutputSettings::BitDepth::Depth16Bit:
			bitDepthComboBox->addItem(tr("16 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth24Bit:
			bitDepthComboBox->addItem(tr("24 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth32Bit:
			bitDepthComboBox->addItem(tr("32 Bit float"), i);
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
			stereoModeComboBox->addItem(tr("Stereo"), i);
			break;
		case OutputSettings::StereoMode::JointStereo:
			stereoModeComboBox->addItem(tr("Joint stereo"), i);
			break;
		case OutputSettings::StereoMode::Mono:
			stereoModeComboBox->addItem(tr("Mono"), i);
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
			compressionLevelComboBox->addItem(tr("%1 (Fastest, biggest)").arg(i), compressionValue);
			break;
		case maxCompressionLevel:
			compressionLevelComboBox->addItem(tr("%1 (Slowest, smallest)").arg(i), compressionValue);
			break;
		default:
			compressionLevelComboBox->addItem(QString::number(i), compressionValue);
			break;
		}
	}
}

} // namespace lmms::gui
