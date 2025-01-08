/*
 * FlacExporter.cpp - exports .flac files outside of AudioEngine
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

#include <vector>

#include <sndfile.h>

#include "FlacExporter.h"

#include "SampleFrame.h"

namespace lmms
{

FlacExporter::FlacExporter(int sampleRate, int bitDepth, const QString& outputLocationAndName) :
	m_isSuccesful(false),
	m_fileDescriptor(NULL)
{
	constexpr int channelCount = 2;
	SF_INFO exportInfo;

	memset(&exportInfo, 0, sizeof(exportInfo));
	exportInfo.samplerate = sampleRate;
	exportInfo.channels = channelCount;
	if (bitDepth == 16)
	{
		exportInfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_16);
	}
	else if (bitDepth == 24)
	{
		exportInfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_24);
	}
	else
	{
		// there is no 32 bit support
		exportInfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_24);
	}

	QByteArray characters = outputLocationAndName.toUtf8();
	m_fileDescriptor = sf_open(characters.data(), SFM_WRITE, &exportInfo);

	m_isSuccesful = m_fileDescriptor != NULL;

	if (m_isSuccesful == false)
	{
		printf("FlacExporter sf_open error\n");
	}
}

FlacExporter::~FlacExporter()
{
	if (m_fileDescriptor == NULL) { return; }
	sf_write_sync(m_fileDescriptor);
	m_isSuccesful = sf_close(m_fileDescriptor) == 0;
	if (m_isSuccesful == false)
	{
		printf("FlacExporter sf_close error\n");
	}
}

void FlacExporter::writeThisBuffer(const SampleFrame* samples, size_t sampleCount)
{
	if (m_fileDescriptor == NULL) { m_isSuccesful = false; return; }
	constexpr size_t channelCount = 2;
	// multiply by 2 since there is 2 channels
	std::vector<float> outputBuffer(sampleCount * channelCount);
	size_t i = 0;
	for (size_t j = 0; j < sampleCount; j++)
	{
		outputBuffer[i] = static_cast<float>((samples + j)->left());
		outputBuffer[i + 1] = static_cast<float>((samples + j)->right());
		outputBuffer[i] = outputBuffer[i] > 1.0f ? 1.0f : outputBuffer[i] < -1.0f ? -1.0f : outputBuffer[i];
		outputBuffer[i + 1] = outputBuffer[i + 1] > 1.0f ? 1.0f : outputBuffer[i + 1] < -1.0f ? -1.0f : outputBuffer[i + 1];
		i = i + channelCount;
	}
	size_t count = sf_writef_float(m_fileDescriptor, outputBuffer.data(), static_cast<sf_count_t>(sampleCount));
	if (count != sampleCount)
	{
		m_isSuccesful = false;
		printf("FlacExporter sf_writef_float error\n");
	}
}

bool FlacExporter::getIsSuccesful() const
{
	return m_isSuccesful;
}

} // namespace lmms
