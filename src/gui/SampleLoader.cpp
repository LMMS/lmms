/*
 * SampleLoader.cpp - Static functions that open audio files
 *
 * Copyright (c) 2025 Sotonye Atemie <sakertooth@gmail.com>
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

#include "GuiApplication.h"

namespace lmms {
std::shared_ptr<const SampleBuffer> SampleLoader::createBufferFromFile(const std::filesystem::path& audioFile)
{
	if (audioFile.empty()) { return SampleBuffer::emptyBuffer(); }

	try
	{
		// TODO: Remove use of Qt
		return SampleBuffer::loadFromCache(audioFile);
	}
	catch (const std::runtime_error& error)
	{
		if (gui::getGUI()) { displayError(QString::fromStdString(error.what())); }
		return SampleBuffer::emptyBuffer();
	}
}

std::shared_ptr<const SampleBuffer> SampleLoader::createBufferFromBase64(const QString& base64, int sampleRate)
{
	if (base64.isEmpty()) { return SampleBuffer::emptyBuffer(); }

	try
	{
		return std::make_shared<SampleBuffer>(base64, sampleRate);
	}
	catch (const std::runtime_error& error)
	{
		if (gui::getGUI()) { displayError(QString::fromStdString(error.what())); }
		return SampleBuffer::emptyBuffer();
	}
}

void SampleLoader::displayError(const QString& message)
{
	QMessageBox::critical(nullptr, QObject::tr("Error loading sample"), message);
}

} // namespace lmms
