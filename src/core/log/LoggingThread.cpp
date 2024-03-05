/*
 * LoggingThread.cpp - implementation of the logging thread
 *
 * Copyright (c) 2020 Artur Twardowski <artur.twardowski/at/gmail/com>
 * Copyright (c) 2024 Jonah Janzen
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
#include "log/LoggingThread.h"

#include "log/LogManager.h"

namespace lmms {

static const int DEFAULT_FLUSH_INTERVAL = 1000;

LoggingThread::LoggingThread()
	: m_flushInterval(DEFAULT_FLUSH_INTERVAL)
	, m_active(false)
{
}

LoggingThread::~LoggingThread()
{
	m_active = false;
	m_thread->join();
}

void LoggingThread::initialize()
{
	m_active = true;
	m_thread = new std::thread([this]() { run(); });
}

LoggingThread& LoggingThread::inst()
{
	static LoggingThread instance;
	return instance;
}

void LoggingThread::run()
{
	while (m_active)
	{
		LogManager::inst().flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(m_flushInterval));
	}
}

} // namespace lmms