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

#include <QDebug>
#include <QMessageBox>
#include <chrono>
#include <iostream>

#include "AudioEngine.h"
#include "GuiApplication.h"

using namespace std::chrono_literals;

namespace lmms {

auto SampleLoader::createBufferFromFile(const QString& filePath) -> std::shared_ptr<const SampleBuffer>
{
	if (filePath.isEmpty()) { return nullptr; }

	qDebug() << "SampleLoader::createBufferFromFile";

	try
	{
		return SampleBuffer::create(filePath);
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

	qDebug() << "SampleLoader::createBufferFromBase64";

	try
	{
		return SampleBuffer::create(base64, sampleRate);
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
