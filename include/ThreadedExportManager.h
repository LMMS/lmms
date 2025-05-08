/*
 * ThreadedExportManager.h - exports files in .flac format on an other thread
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

#ifndef LMMS_THREADED_EXPORT_MANAGER_H
#define LMMS_THREADED_EXPORT_MANAGER_H

#include <atomic>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>
#include <mutex>

#include <sndfile.h>

#include <QString>

namespace lmms
{

class SampleBuffer;

typedef void (*callbackFn)(void*);

class ThreadedExportManager
{
public:
	ThreadedExportManager();
	~ThreadedExportManager();

	//! @param outputLocationAndName: should include path and name, could include ".flac"
	void startExporting(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer, callbackFn callbackFunction = nullptr, void* callbackObject = nullptr);
	void stopExporting();

private:
	static void threadedExportMethod(ThreadedExportManager* thisExporter, std::atomic<bool>* abortExport);

	std::vector<std::tuple<QString, std::shared_ptr<const SampleBuffer>, callbackFn, void*>> m_buffers;

	std::atomic<bool> m_isThreadRunning = false;
	std::mutex m_readMutex;
	std::thread* m_thread = nullptr;
};

} // namespace lmms

#endif // LMMS_THREADED_EXPORT_MANAGER_H
