/*
 * SampleLoader.cpp - Static functions that open audio files
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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
#include "GuiApplication.h"
#include "PathUtil.h"
#include "SampleDecoder.h"
#include "Song.h"

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
	auto fileTypes = QStringList{};
	auto allFileTypes = QString{"All Audio-Files ("};
	const auto& supportedAudioTypes = SampleDecoder::s_supportedAudioTypes;

	for (int i = 0; i < supportedAudioTypes.size(); ++i)
	{
		const auto& audioType = supportedAudioTypes[i];
		const auto name = QString::fromStdString(audioType.name);
		const auto extension = QString::fromStdString(audioType.extension);

		fileTypes.append(QObject::tr(qPrintable(QString{"%1-Files (*.%2)"}.arg(name, extension))));

		allFileTypes.append(QObject::tr(qPrintable(QString{"*.%2"}.arg(extension))));
		if (i < supportedAudioTypes.size() - 1) { allFileTypes.append(" "); }
		else { allFileTypes.append(")"); }
	}

	fileTypes.append(QObject::tr("Other files (*)"));
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

std::unique_ptr<SampleBuffer> SampleLoader::createBufferFromFile(const QString& filePath, bool collectErrorWhenNotFound)
{
	if (filePath.isEmpty()) { return std::make_unique<SampleBuffer>(); }

	if (collectErrorWhenNotFound)
	{
		auto absolutePath = PathUtil::toAbsolute(filePath);
		if (!QFileInfo(absolutePath).exists())
		{
			QString message = QObject::tr("Sample not found: %1").arg(filePath);
			Engine::getSong()->collectError(message);
			return std::make_unique<SampleBuffer>();
		}
	}

	try
	{
		return std::make_unique<SampleBuffer>(filePath);
	}
	catch (const std::runtime_error& error)
	{
		if (getGUI()) { displayError(QString::fromStdString(error.what())); }
		return std::make_unique<SampleBuffer>();
	}
}

std::unique_ptr<SampleBuffer> SampleLoader::createBufferFromBase64(const QString& base64, int sampleRate)
{
	if (base64.isEmpty()) { return std::make_unique<SampleBuffer>(); }

	try
	{
		return std::make_unique<SampleBuffer>(base64, sampleRate);
	}
	catch (const std::runtime_error& error)
	{
		if (getGUI()) { displayError(QString::fromStdString(error.what())); }
		return std::make_unique<SampleBuffer>();
	}
}

void SampleLoader::displayError(const QString& message)
{
	QMessageBox::critical(nullptr, QObject::tr("Error loading sample"), message);
}

} // namespace lmms::gui
