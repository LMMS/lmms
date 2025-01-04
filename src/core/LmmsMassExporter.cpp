/*
 * LmmsMassExporter.cpp - exports .flac files outside of AudioEngine
 *
 * Copyright (c) 2024 szeli1 <TODO/at/gmail/dot.com>
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

#include <sndfile.h>

#include <QFileInfo>

#include "LmmsMassExporter.h"

#include "SampleBuffer.h"

namespace lmms
{

FlacExporter::FlacExporter(int sampleRate, int bitDepth, const QString& outputLocationAndName) :
	m_isSuccesful(false),
	m_fileDescriptor(NULL)
{
	constexpr int channelCount = 2;
	SF_INFO exportInfo;

	memset(&exportInfo, 0, sizeof(exportInfo));
	exportInfo.frames = 1;
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
		printf("LmmsMassExporter sf_open error\n");
	}
}

FlacExporter::~FlacExporter()
{
	if (m_fileDescriptor == NULL) { return; }
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
		printf("LmmsMassExporter sf_writef_float error\n");
	}
}

bool FlacExporter::getIsSuccesful() const
{
	return m_isSuccesful;
}

LmmsMassExporter::LmmsMassExporter() :
	m_abortExport(false),
	m_isThreadRunning(false),
	m_readMutex(),
	m_thread(nullptr)
{}

LmmsMassExporter::~LmmsMassExporter()
{
	stopExporting();
}


void LmmsMassExporter::startExporting(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer)
{
	QString filteredName(QFileInfo(outputLocationAndName).absolutePath() + "/" + QFileInfo(outputLocationAndName).baseName()+ ".flac");
	m_readMutex.lock();
	m_buffers.push_back(std::make_pair(filteredName, buffer));
	m_readMutex.unlock();

	if (m_isThreadRunning == false)
	{
		stopExporting();
		m_isThreadRunning = true;
		m_thread = new std::thread(&LmmsMassExporter::threadedExportFunction, this, &m_abortExport);
	}
}

void LmmsMassExporter::stopExporting()
{
	if (m_thread != nullptr)
	{
		if (m_isThreadRunning == true)
		{
			m_abortExport = true;
		}
		m_thread->join();
		delete m_thread;
		m_thread = nullptr;
		m_isThreadRunning = false;
		m_abortExport = false;
	}
}


void LmmsMassExporter::threadedExportFunction(LmmsMassExporter* thisExporter, volatile std::atomic<bool>* abortExport)
{
	thisExporter->m_isThreadRunning = true;

	while (*abortExport == false)
	{
		std::pair<QString, std::shared_ptr<const SampleBuffer>> curBuffer = std::make_pair(QString(""), nullptr);
		thisExporter->m_readMutex.lock();
		bool shouldExit = thisExporter->m_buffers.size() <= 0;
		if (shouldExit == false)
		{
			curBuffer = thisExporter->m_buffers[thisExporter->m_buffers.size() - 1];
		}
		thisExporter->m_readMutex.unlock();
		if (shouldExit) { break; }

		FlacExporter exporter(curBuffer.second->sampleRate(), 24, curBuffer.first);
		if (exporter.getIsSuccesful())
		{
			exporter.writeThisBuffer(curBuffer.second->data(), curBuffer.second->size());
		}

		thisExporter->m_buffers.pop_back();
	}

	thisExporter->m_isThreadRunning = false;
}

} // namespace lmms
