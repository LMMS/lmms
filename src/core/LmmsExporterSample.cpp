/*
 * LmmsExporterSample.cpp - exports .flac files outside of AudioEngine
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
#include <QFileInfo>

#include "LmmsExporterSample.h"

#include "SampleBuffer.h"

namespace lmms
{

LmmsExporterSample::LmmsExporterSample() :
	m_abortExport(false),
	m_isThreadRunning(false),
	m_readMutex(),
	m_thread(nullptr),
	m_fileDescriptor(NULL)
{

}

LmmsExporterSample::~LmmsExporterSample()
{
	stopExporting();
}


void LmmsExporterSample::startExporting(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer)
{
	QString filteredName(QFileInfo(outputLocationAndName).absolutePath() + "/" + QFileInfo(outputLocationAndName).baseName()+ ".flac");
	qDebug("LmmsExporterSample::startExporting");
	m_readMutex.lock();
	m_buffers.push_back(std::make_pair(filteredName, buffer));
	m_readMutex.unlock();

	if (m_isThreadRunning == false)
	{
		qDebug("LmmsExporterSample::startExporting export");
		stopExporting();
		m_isThreadRunning = true;
		m_thread = new std::thread(&LmmsExporterSample::threadedExportFunction, this, &m_abortExport);
	}
}

void LmmsExporterSample::stopExporting()
{
	qDebug("LmmsExporterSample::stopExporting");
	if (m_thread != nullptr)
	{
		qDebug("LmmsExporterSample::stopExporting not nullptr");
		if (m_isThreadRunning == true)
		{
			m_abortExport = true;
		}
		m_thread->join();
		qDebug("LmmsExporterSample::stopExporting deleting");
		delete m_thread;
		m_thread = nullptr;
		m_isThreadRunning = false;
		m_abortExport = false;
		qDebug("LmmsExporterSample::stopExporting end");
	}
}


void LmmsExporterSample::threadedExportFunction(LmmsExporterSample* thisExporter, volatile bool* abortExport)
{
	thisExporter->m_isThreadRunning = true;

	qDebug("LmmsExporterSample::threadedExportFunction");
	while (*abortExport == false)
	{
		qDebug("LmmsExporterSample::threadedExportFunction getting buffer");
		std::pair<QString, std::shared_ptr<const SampleBuffer>> curBuffer = std::make_pair(QString(""), nullptr);
		thisExporter->m_readMutex.lock();
		bool shouldExit = thisExporter->m_buffers.size() <= 0;
		if (shouldExit == false)
		{
			curBuffer = thisExporter->m_buffers[thisExporter->m_buffers.size() - 1];
			qDebug("LmmsExporterSample::threadedExportFunction read buffer");
		}
		thisExporter->m_readMutex.unlock();
		if (shouldExit) { break; }

		qDebug("LmmsExporterSample::threadedExportFunction libsndfile code:");
		bool success = thisExporter->openFile(curBuffer.first, curBuffer.second);
		if (success)
		{
			thisExporter->exportBuffer(curBuffer.second);
			thisExporter->closeFile();
		}

		thisExporter->m_buffers.pop_back();
	}

	qDebug("LmmsExporterSample::threadedExportFunction closing thread");
	thisExporter->m_isThreadRunning = false;
}

bool LmmsExporterSample::openFile(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer)
{
	bool success = true;
	qDebug("LmmsExporterSample::openFile: %s", outputLocationAndName.toStdString().c_str());
	QFile targetFile(outputLocationAndName);
	if (targetFile.exists() == false)
	{
		// creating new file
		success = targetFile.open(QIODevice::WriteOnly);
		if (success == false) {qDebug("returning because of input"); return false; }
		targetFile.close();
		qDebug("new file created");
	}

	constexpr int channelCount = 2;
	SF_INFO exportInfo;

	memset(&exportInfo, 0, sizeof(exportInfo));
	exportInfo.frames = static_cast<sf_count_t>(buffer->size() * channelCount);
	//exportInfo.frames = 400;
	exportInfo.samplerate = buffer->sampleRate();
	exportInfo.channels = channelCount;
	exportInfo.format = (SF_FORMAT_FLAC | SF_FORMAT_PCM_24);

	qDebug("LmmsExporterSample::openFile 2, buffer size: %ld", buffer->size());
#if defined(LMMS_BUILD_WIN32) || defined(LMMS_BUILD_WIN64)
	qDebug("LmmsExporterSample::openFile 3");
	std::wstring characters = outputLocationAndName.toWString();
	// wstring::c_str should guarantee null termination character at end
	// this should be in big endian byte order
	m_fileDescriptor = sf_open(characters.c_str(), SFM_WRITE, &exportInfo);
#else
	qDebug("LmmsExporterSample::openFile 4");
	QByteArray characters = outputLocationAndName.toUtf8();
	m_fileDescriptor = sf_open(characters.data(), SFM_WRITE, &exportInfo);
	//m_fileDescriptor = sf_open(tempPath, SFM_WRITE, &exportInfo);
#endif
	qDebug("LmmsExporterSample::openFile error: %d", sf_error(m_fileDescriptor));
	qDebug("LmmsExporterSample::openFile error: %s", sf_strerror(m_fileDescriptor));
	success = m_fileDescriptor != NULL;
	return success;
}

void LmmsExporterSample::exportBuffer(std::shared_ptr<const SampleBuffer> buffer)
{
	if (m_fileDescriptor == NULL) { return; }
	constexpr size_t channelCount = 2;
	// multiply by 2 since there is 2 channels
	std::vector<float> outputBuffer(buffer->size() * channelCount);
	size_t i = 0;
	for (auto it = buffer->begin(); it != buffer->end(); ++it)
	{
		outputBuffer[i] = static_cast<float>(it->left());
		outputBuffer[i + 1] = static_cast<float>(it->right());
		outputBuffer[i] = outputBuffer[i] > 1.0f ? 1.0f : outputBuffer[i] < -1.0f ? -1.0f : outputBuffer[i];
		outputBuffer[i + 1] = outputBuffer[i + 1] > 1.0f ? 1.0f : outputBuffer[i + 1] < -1.0f ? -1.0f : outputBuffer[i + 1];
		i = i + channelCount;
	}
	qDebug("LmmsExporterSample::exportBuffer, wrote buffer size: %ld, buffer size: %ld", i, outputBuffer.size());
	size_t count = sf_writef_float(m_fileDescriptor, outputBuffer.data(), static_cast<sf_count_t>(buffer->size()));
	if (count != outputBuffer.size())
	{
		qDebug("LmmsExporterSample::exportBuffer error: %s", sf_strerror(m_fileDescriptor));
	}
	qDebug("LmmsExporterSample::exportBuffer, wrote buffer size 2: %ld   %ld", count, sizeof(float));
}

void LmmsExporterSample::closeFile()
{
	if (m_fileDescriptor == NULL) { return; }
	qDebug("LmmsExporterSample::closeFile");
	int success = sf_close(m_fileDescriptor);
	if (success != 0)
	{
		printf("LmmsExporterSample export error");
	}
	m_fileDescriptor = NULL;
}

} // namespace lmms

