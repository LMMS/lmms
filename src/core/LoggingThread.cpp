/*
 * LoggingThread.cpp - implementation of the logging thread
 *
 * Copyright (c) 2020 Artur Twardowski <artur.twardowski/at/gmail/com>
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
#include "LoggingThread.h"
#include "Logging.h"

static const int DEFAULT_FLUSH_INTERVAL = 1000;

LoggingThread::LoggingThread()
	: m_flushInterval(DEFAULT_FLUSH_INTERVAL)
	, m_active(true)
{
}

LoggingThread::~LoggingThread()
{
	m_active = false;
	if (!wait(m_flushInterval)) {
		terminate();
	}

	/* Forcifully flush all the messages that still are present
	 * in the buffer - don't care about the performance, as probably
	 * we are shutting down right now.*/
	LogManager::inst().flush();
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
		if (!LogManager::inst().isFlushPaused())
		{
			LogManager::inst().flush();
		}
		msleep(DEFAULT_FLUSH_INTERVAL);
	}
	this->exit();
}
