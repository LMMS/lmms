/*
 * AudioFileDeviceSample.cpp - exporting files (currently only audio files), TODO rename class when things change
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

#include <QFile>

#include "LmmsExporterSample.h"

#include "SampleBuffer.h"

namespace lmms
{

LmmsExporterSample::LmmsExporterSample() :
	m_fileDescriptor(NULL)
{

}

LmmsExporterSample::~LmmsExporterSample()
{
	stopExorting();
}


void LmmsExporterSample::startExporting(const QString& outputLocationAndName, std::shared_ptr<SampleBuffer> buffer)
{
	m_readMutex.lock();
	m_buffers.push_back(std::make_pair(outputLocationAndName, buffer));
	m_readMutex.unlock();

	if (m_isThreadRunning == false)
	{
		stopExorting();
		m_isThreadRunning = true;
		m_thread = std::make_unique<std::thread>(&LmmsExporterSample::threadedExportFunction, this, &m_abortExport);
	}
}

void LmmsExporterSample::stopExporting()
{
	if (m_thread.get() != nullptr)
	{
		if (m_isThreadRunning == true)
		{
			m_abortExport = true;
			m_thread->join();
		}
		m_thread = nullptr;
		m_isThreadRunning = false;
		m_abortExport = false;
	}
}


void LmmsExporterSample::threadedExportFunction(LmmsExporterSample* thisExporter, bool* abortExport)
{
	thisExporter->m_isThreadRunning = true;

	while (abortExport == false)
	{
		std::pari<QString, std::shared_ptr<SampleBuffer>> curBuffer = nullptr;
		thisExporter->m_readMutex.lock();
		bool shouldExit = thisExporter->m_buffers.size() <= 0;
		if (shouldExit == false)
		{
			curBuffer = thisExporter->m_buffers[0];
		}
		thisExporter->m_readMutex.unlock();
		if (shouldExit) { break; }

		thisExporter->openFile(curBuffer.first, curBuffer.second);
		thisExporter->exportBuffer(curBuffer.second);
		thisExporter->closeFile();
	}

	thisExporter->m_isThreadRunning = false;
}

void LmmsExporterSample::openFile(const QString& outputLocationAndName, std::shared_ptr<SampleBuffer> buffer)
{
	SF_INFO exportInfo;

	memset(&exportInfo, 0, sizeof(exportInfo));
	exportInfo.frames = buffer->size();
	exportInfo.samplerate = buffer->sampleRate();
	exportInfo.channels = 2;
	exportInfo.format = SF_FORMAT_FLAC;

#ifdef LMMS_BUILD_WIN32 || LMMS_BUILD_WIN64
	std::wstring characters = outputLocationAndName.toWString();
	// wstring::c_str should guarantee null termination character at end
	// this should be in big endian byte order
	m_fileDescriptor = sf_open(characters.c_str(), SFM_WRITE, &exportInfo);
#else
	QByteArray characters = outputLocationAndName.toUtf8();
	m_fileDescriptor = sf_open(&characters, SFM_WRITE, &exportInfo);
#endif
}

void LmmsExporterSample::exportBuffer(std::shared_ptr<SampleBuffer> buffer)
{
	if (m_fileDescriptor == NULL) { return; }
	sf_writef_float(m_fileDescrtiptor, static_cast<float*>(buffer->data()), buffer->size());
}

void LmmsExporterSample::closeFile()
{
	if (m_fileDescriptor == NULL) { return; }
	int success = sf_close(m_fileDescriptor);
	m_fileDescriptor = NULL;
}

} // namespace lmms

