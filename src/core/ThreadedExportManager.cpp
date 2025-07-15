/*
 * ThreadedExportManager.cpp - exports files in .flac format on an other thread
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

#include "ThreadedExportManager.h"

#include "FlacExporter.h"
#include "SampleBuffer.h"

namespace lmms
{

ThreadedExportManager::ThreadedExportManager()
{}

ThreadedExportManager::~ThreadedExportManager()
{
	stopExporting();
}


void ThreadedExportManager::startExporting(const QString& outputLocationAndName, std::shared_ptr<const SampleBuffer> buffer, callbackFn callbackFunction, void* callbackObject)
{
	m_readMutex.lock();
	m_buffers.push_back(std::make_tuple(outputLocationAndName, buffer, callbackFunction, callbackObject));
	m_readMutex.unlock();

	if (m_isThreadRunning == false)
	{
		stopExporting();
		m_isThreadRunning = true;
		m_thread = new std::thread(&ThreadedExportManager::threadedExportMethod, this, &m_isThreadRunning);
	}
}

void ThreadedExportManager::stopExporting()
{
	if (m_thread != nullptr)
	{
		m_isThreadRunning = false;
		m_thread->join();
		delete m_thread;
		m_thread = nullptr;
	}
}


void ThreadedExportManager::threadedExportMethod(ThreadedExportManager* thisExporter, std::atomic<bool>* shouldRun)
{
	while (*shouldRun == true)
	{
		std::tuple<QString, std::shared_ptr<const SampleBuffer>, callbackFn, void*> curBuffer = std::make_tuple(QString(""), nullptr, nullptr, nullptr);
		thisExporter->m_readMutex.lock();
		bool shouldExit = thisExporter->m_buffers.size() <= 0;
		if (shouldExit == false)
		{
			curBuffer = thisExporter->m_buffers[thisExporter->m_buffers.size() - 1];
			thisExporter->m_buffers.pop_back();
		}
		thisExporter->m_readMutex.unlock();
		if (shouldExit) { break; }

		// important new scope
		// can't call back if flacExporter's file is open
		{
			FlacExporter flacExporter(std::get<1>(curBuffer)->sampleRate(), 24, std::get<0>(curBuffer));
			if (flacExporter.getIsSuccesful())
			{
				flacExporter.writeThisBuffer(std::get<1>(curBuffer)->data(), std::get<1>(curBuffer)->size());
			}
		}

		if (std::get<2>(curBuffer))
		{
			// calling callback funcion
			std::get<2>(curBuffer)(std::get<3>(curBuffer));
		}
	}
}

} // namespace lmms
