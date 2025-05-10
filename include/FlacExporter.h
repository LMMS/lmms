/*
 * FlacExporter.h - exports .flac files outside of AudioEngine
 *
 * Copyright (c) 2024 - 2025 szeli1
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

#ifndef LMMS_FLAC_EXPORTER_H
#define LMMS_FLAC_EXPORTER_H

#include <sndfile.h>

#include <QString>

namespace lmms
{

class SampleFrame;

class FlacExporter
{
public:
	FlacExporter(int sampleRate, int bitDepth, const QString& outputLocationAndName);
	~FlacExporter();

	void writeThisBuffer(const SampleFrame* samples, size_t sampleCount);
	bool getIsSuccesful() const;

private:
	bool m_isSuccesful = false;
	SNDFILE* m_fileDescriptor = nullptr;
};

} // namespace lmms

#endif // LMMS_FLAC_EXPORTER_H
