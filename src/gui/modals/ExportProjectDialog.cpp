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
#include <QPushButton>
#include <QVBoxLayout>

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
}

} // namespace lmms::gui
