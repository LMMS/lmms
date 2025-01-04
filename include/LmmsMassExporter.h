/*
 * LmmsMassExporter.h - exports .flac files outside of AudioEngine
 *
 * Copyright (c) 2024 - 2025 szeli1 <TODO/at/gmail/dot.com>
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

#ifndef LMMS_LMMS_MASS_EXPORTER_H
#define LMMS_LMMS_MASS_EXPORTER_H

#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>

#include <sndfile.h>

#include <QString>

namespace lmms
{

class SampleBuffer;
class SampleFrame;

class FlacExporter
{
public:
	FlacExporter(int sampleRate, int bitDepth, const QString& outputLocationAndName);
	~FlacExporter();

	void writeThisBuffer(const SampleFrame* samples, size_t sampleCount);
	bool getIsSuccesful() const;
	
private:
	bool m_isSuccesful;
	SNDFILE* m_fileDescriptor;
};

class LmmsMassExporter
{
public:
	LmmsMassExporter();
	~LmmsMassExporter();

	//! outputLocationAndName: should include path and name, could include ".flac"
	void startExporting(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer);
	void stopExporting();
	
	//void writeBuffer(const SampleFrame* _ab, fpp_t const frames);

private:
	static void threadedExportFunction(LmmsMassExporter* thisExporter, volatile std::atomic<bool>* abortExport);

	void stopThread();
	bool openFile(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer);
	void exportBuffer(std::shared_ptr<const SampleBuffer> buffer);
	void closeFile();

	std::vector<std::pair<QString, std::shared_ptr<const SampleBuffer>>> m_buffers;

	volatile std::atomic<bool> m_abortExport;
	bool m_isThreadRunning;
	std::mutex m_readMutex;
	std::thread* m_thread;
	
	
};

} // namespace lmms

#endif // LMMS_LMMS_MASS_EXPORTER_H
