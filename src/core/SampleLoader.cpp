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

#include <QMessageBox>
#include <QTimer>
#include <QObject>
#include <chrono>
#include <memory>
#include <iostream>

#include "AudioEngine.h"
#include "GuiApplication.h"
#include "NoCopyNoMove.h"
#include "SampleCache.h"

using namespace std::chrono_literals;

namespace lmms {

auto SampleLoader::createBufferFromFile(const QString& filePath) -> std::shared_ptr<const SampleBuffer>
{
	if (filePath.isEmpty()) { return nullptr; }

	if (auto buffer = SampleCache::get(filePath, SampleBuffer::Source::AudioFile))
	{
		return buffer;
	}

	try
	{
		auto buffer = SampleCache::createBufferFromFile(filePath);
		SampleCache::add(*buffer);
		return buffer;
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

auto SampleLoader::createBufferFromBase64(const QString& base64, int sampleRate)
	-> std::shared_ptr<const SampleBuffer>
{
	if (base64.isEmpty()) { return nullptr; }

	if (auto buffer = SampleCache::get(base64, SampleBuffer::Source::Base64))
	{
		return buffer;
	}

	try
	{
		auto buffer = SampleCache::createBufferFromBase64(base64, sampleRate);
		SampleCache::add(*buffer);
		return buffer;
	}
	catch (const std::runtime_error& error)
	{
		displayError(QString::fromStdString(error.what()));
		return nullptr;
	}
}

void SampleLoader::displayError(const QString& message)
{
	if (gui::getGUI())
	{
		QMessageBox::critical(nullptr, QObject::tr("Error loading sample"), message);
	}
	else
	{
		std::cerr << QObject::tr("Error loading sample: %1").arg(message).toStdString() << "\n";
	}
}

} // namespace lmms
