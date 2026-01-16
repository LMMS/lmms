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
	, m_fileFormatLabel(new QLabel(tr("File format:")))
	, m_fileFormatComboBox(new QComboBox())
	, m_sampleRateLabel(new QLabel(tr("Sampling rate:")))
	, m_sampleRateComboBox(new QComboBox())
	, m_bitRateLabel(new QLabel(tr("Bit rate:")))
	, m_bitRateComboBox(new QComboBox())
	, m_bitDepthLabel(new QLabel(tr("Bit depth:")))
	, m_bitDepthComboBox(new QComboBox())
	, m_stereoModeLabel(new QLabel(tr("Stereo mode:")))
	, m_stereoModeComboBox(new QComboBox())
	, m_compressionLevelLabel(new QLabel(tr("Compression level:")))
	, m_compressionLevelComboBox(new QComboBox())
	, m_startButton(new QPushButton(tr("Start")))
	, m_cancelButton(new QPushButton(tr("Cancel")))
	, m_progressBar(new QProgressBar())
{
	setWindowTitle(tr("Export project to %1").arg(QFileInfo(path).fileName()));

	auto mainLayout = new QVBoxLayout(this);
	auto startCancelButtonsLayout = new QHBoxLayout{};

	auto fileFormatSettingsGroupBox = new QGroupBox(tr("File format settings"));
	auto fileFormatSettingsLayout = new QVBoxLayout(fileFormatSettingsGroupBox);

	fileFormatSettingsLayout->addWidget(m_fileFormatLabel);
	fileFormatSettingsLayout->addWidget(m_fileFormatComboBox);

	fileFormatSettingsLayout->addWidget(m_sampleRateLabel);
	fileFormatSettingsLayout->addWidget(m_sampleRateComboBox);

	fileFormatSettingsLayout->addWidget(m_bitRateLabel);
	fileFormatSettingsLayout->addWidget(m_bitRateComboBox);

	fileFormatSettingsLayout->addWidget(m_bitDepthLabel);
	fileFormatSettingsLayout->addWidget(m_bitDepthComboBox);

	fileFormatSettingsLayout->addWidget(m_stereoModeLabel);
	fileFormatSettingsLayout->addWidget(m_stereoModeComboBox);

	fileFormatSettingsLayout->addWidget(m_compressionLevelLabel);
	fileFormatSettingsLayout->addWidget(m_compressionLevelComboBox);

	startCancelButtonsLayout->addStretch();
	startCancelButtonsLayout->addWidget(m_startButton);
	startCancelButtonsLayout->addWidget(m_cancelButton);

	mainLayout->addWidget(fileFormatSettingsGroupBox);
	mainLayout->addLayout(startCancelButtonsLayout);
	mainLayout->addWidget(m_progressBar);

	for (const auto& device : ProjectRenderer::fileEncodeDevices)
	{
		if (!device.isAvailable()) { continue; }
		m_fileFormatComboBox->addItem(tr(device.m_description), static_cast<int>(device.m_fileFormat));
	}

	for (const auto& sampleRate : SUPPORTED_SAMPLERATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(sampleRate), "Hz");
		m_sampleRateComboBox->addItem(str, sampleRate);
	}

	for (const auto& bitRate : SUPPORTED_BITRATES)
	{
		const auto str = tr("%1 %2").arg(QString::number(bitRate), "KBit/s");
		m_bitRateComboBox->addItem(str, bitRate);
	}

	for (auto i = 0; i < static_cast<int>(OutputSettings::BitDepth::Count); ++i)
	{
		switch (static_cast<OutputSettings::BitDepth>(i))
		{
		case OutputSettings::BitDepth::Depth16Bit:
			m_bitDepthComboBox->addItem(tr("16 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth24Bit:
			m_bitDepthComboBox->addItem(tr("24 Bit integer"), i);
			break;
		case OutputSettings::BitDepth::Depth32Bit:
			m_bitDepthComboBox->addItem(tr("32 Bit float"), i);
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
			m_stereoModeComboBox->addItem(tr("Stereo"), i);
			break;
		case OutputSettings::StereoMode::JointStereo:
			m_stereoModeComboBox->addItem(tr("Joint stereo"), i);
			break;
		case OutputSettings::StereoMode::Mono:
			m_stereoModeComboBox->addItem(tr("Mono"), i);
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
			m_compressionLevelComboBox->addItem(tr("%1 (Fastest, biggest)").arg(i), compressionValue);
			break;
		case maxCompressionLevel:
			m_compressionLevelComboBox->addItem(tr("%1 (Slowest, smallest)").arg(i), compressionValue);
			break;
		default:
			m_compressionLevelComboBox->addItem(QString::number(i), compressionValue);
			break;
		}
	}

	m_progressBar->setValue(0);
}

} // namespace lmms::gui
