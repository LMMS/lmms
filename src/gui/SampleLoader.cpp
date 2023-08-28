/*
 * SampleLoader.cpp - Static functions that open audio files
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "SampleLoader.h"

#include <QFileInfo>
#include <QMessageBox>

#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"

namespace lmms::gui {
QString SampleLoader::openAudioFile(const QString& previousFile)
{
	auto openFileDialog = gui::FileDialog(nullptr, QObject::tr("Open audio file"));
	auto dir = QString{};

	if (!previousFile.isEmpty())
	{
		auto file = QString{previousFile};
		if (QFileInfo{file}.isRelative())
		{
			file = ConfigManager::inst()->userSamplesDir() + file;
			if (!QFileInfo{file}.exists()) { file = ConfigManager::inst()->factorySamplesDir() + previousFile; }
		}

		dir = QFileInfo{file}.absolutePath();
	}
	else { dir = ConfigManager::inst()->userSamplesDir(); }

	// change dir to position of previously opened file
	openFileDialog.setDirectory(dir);
	openFileDialog.setFileMode(gui::FileDialog::ExistingFiles);

	// set filters
	// TODO: Since libsndfile 1.1.0, MP3 is supported
	const auto fileTypes = QStringList{QObject::tr("All Audio-Files (*.wav *.ogg *.ds *.flac *.spx *.voc "
												   "*.aif *.aiff *.au *.raw)"),
		QObject::tr("Wave-Files (*.wav)"), QObject::tr("OGG-Files (*.ogg)"), QObject::tr("DrumSynth-Files (*.ds)"),
		QObject::tr("FLAC-Files (*.flac)"), QObject::tr("SPEEX-Files (*.spx)"), QObject::tr("VOC-Files (*.voc)"),
		QObject::tr("AIFF-Files (*.aif *.aiff)"), QObject::tr("AU-Files (*.au)"), QObject::tr("RAW-Files (*.raw)")};

	openFileDialog.setNameFilters(fileTypes);

	if (!previousFile.isEmpty())
	{
		// select previously opened file
		openFileDialog.selectFile(QFileInfo{previousFile}.fileName());
	}

	if (openFileDialog.exec() == QDialog::Accepted)
	{
		if (openFileDialog.selectedFiles().isEmpty()) { return ""; }

		return PathUtil::toShortestRelative(openFileDialog.selectedFiles()[0]);
	}

	return "";
}

QString SampleLoader::openWaveformFile(const QString& previousFile)
{
	return openAudioFile(
		previousFile.isEmpty() ? ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac" : previousFile);
}

std::unique_ptr<SampleBuffer> SampleLoader::createBufferFromFile(const QString& filePath)
{
	try
	{
		return std::make_unique<SampleBuffer>(filePath);
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return std::make_unique<SampleBuffer>();
	}
}

std::unique_ptr<SampleBuffer> SampleLoader::createBufferFromBase64(const QString& base64, int sampleRate)
{
	try
	{
		return std::make_unique<SampleBuffer>(base64.toUtf8().toBase64(), sampleRate);
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return std::make_unique<SampleBuffer>();
	}
}

void SampleLoader::displayError(const QString& message)
{
	QMessageBox::critical(nullptr, QObject::tr("Error loading sample"), message);
}

} // namespace lmms::gui