/*
 * SampleLoader.h
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

#ifndef LMMS_SAMPLE_LOADER_H
#define LMMS_SAMPLE_LOADER_H

#include <QString>
#include <memory>

#include "SampleBuffer.h"
#include "lmms_export.h"

namespace lmms {
class LMMS_EXPORT SampleLoader
{
public:
	static std::shared_ptr<const SampleBuffer> loadBufferFromFile(const QString& filePath);
	static std::shared_ptr<const SampleBuffer> loadBufferFromBase64(
		const QString& base64, int sampleRate = Engine::audioEngine()->outputSampleRate());
};
} // namespace lmms

#endif // LMMS_SAMPLE_LOADER_H
