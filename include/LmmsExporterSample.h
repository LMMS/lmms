/*
 * LmmsExporterSample.h - exporting files (currently only audio files), TODO rename class when things change
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

#ifndef LMMS_LMMS_EXPORTER_H
#define LMMS_LMMS_EXPORTER_H

#include <memory>
#include <sndfile.h>
#include <thread>
#include <utility>
#include <vector>
#include <mutex>

#include <QString>

namespace lmms
{

class SampleBuffer;

class LmmsExporterSample
{
public:
	LmmsExporterSample();
	~LmmsExporterSample();

	void startExporting(const QString& outputLocationAndName, std::shared_ptr<SampleBuffer> buffer);
	void stopExporting();
	
	//void writeBuffer(const SampleFrame* _ab, fpp_t const frames);

private:
	static void threadedExportFunction(LmmsExporterSample* thisExporter, bool* abortExport);

	void stopThread();
	void openFile(const QString& outputLocationAndName);
	void exportBuffer(std::shared_ptr<SampleBuffer> buffer);
	void closeFile();

	QFile m_outputFile;
	std::vector<std::pair<QString, std::shared_ptr<SampleBuffer>>> m_buffers;

	volatile bool m_abortExport;
	bool m_isThreadRunning;
	std::mutex m_readMutex;
	std::unique_ptr<std::thread> m_thread;
	
	SNDFILE* m_fileDescriptor;
};

} // namespace lmms

#endif // LMMS_LMMS_EXPORTER_H
