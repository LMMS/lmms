/*
 * SampleLoader.h - Load audio and waveform files
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

#ifndef LMMS_GUI_SAMPLE_LOADER_H
#define LMMS_GUI_SAMPLE_LOADER_H

#include <QString>
#include <memory>

#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms::gui {
class LMMS_EXPORT SampleLoader
{
public:
	static QString openAudioFile(const QString& previousFile = "");
	static QString openWaveformFile(const QString& previousFile = "");
	static std::shared_ptr<const SampleBuffer> createBufferFromFile(const QString& filePath);
	static std::shared_ptr<const SampleBuffer> createBufferFromBase64(
		const QString& base64, int sampleRate = Engine::audioEngine()->processingSampleRate());
private:
	static void displayError(const QString& message);
};
} // namespace lmms::gui

#endif // LMMS_GUI_SAMPLE_LOADER_H
