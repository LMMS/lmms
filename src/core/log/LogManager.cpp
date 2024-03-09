/*
 * Logging.cpp - implementation of member functions of classes related
 *               to logging
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
#include "log/LogManager.h"

#include <chrono>
#include <iomanip>
#include <map>
#include <sstream>

#include "LoggingMacros.h"
#include "log/LogLine.h"
#include "log/LogSink.h"
#include "log/LoggingThread.h"

namespace lmms {

// The number of messages that can be stored internally before a flush is forced.
const int LOG_BUFFER_SIZE = 1024;

LogTopic::LogTopic(std::string name)
	: m_name(name)
{
}

std::string LogTopic::name() const
{
	return m_name;
}

LogManager& LogManager::inst()
{
	static LogManager instance;
	return instance;
}

LogManager::LogManager()
	: m_pendingLogLines(LOG_BUFFER_SIZE)
	, m_pendingLogLinesReader(m_pendingLogLines)
{
	// Initialize the ringbuffer and start the logging thread.
	m_pendingLogLines.mlock();
	m_pendingLogLines.touch();
	LoggingThread::inst().initialize();
}

LogManager::~LogManager()
{
	/* Forcibly flush all the messages that still are present
	 * in the buffer - don't care about the performance, as probably
	 * we are shutting down right now.*/
	flush();

	// Clean up all log sinks.
	for (LogSink* pSink : m_sinks)
	{
		delete pSink;
	}
}

void LogManager::addSink(LogSink* sink)
{
	m_sinks.push_back(sink);
}

void LogManager::push(LogLine* logLine)
{
	// Only log messages at or below the max verbosity.
	if (m_maxVerbosity < logLine->verbosity) return;

	// Add it to the ringbuffer queue if possible; otherwise force a log flush so no messages are dropped.
	if (m_pendingLogLines.write_space() > 1) { m_pendingLogLines.write(&logLine, 1); }
	else if (m_pendingLogLines.write_space() == 1)
	{
		// This may cause performance stutters in realtime functions.
		flush();
		LOG_WARN("A log queue overflow has resulted in a forced flush.");
		m_pendingLogLines.write(&logLine, 1);
	}

	/* In case of fatal error flush the messages immediately,
	 * then terminate the application */
	if (logLine->verbosity == LogVerbosity::Fatal)
	{
		flush();
		abort();
	}
}

void LogManager::flush()
{
	// Go through all pending log messages and pass them to each log sink.
	auto seq = m_pendingLogLinesReader.read_max(LOG_BUFFER_SIZE);

	for (size_t ix = 0; ix < seq.size(); ix++)
	{
		LogLine* logLine = seq[ix];
		for (LogSink* sink : m_sinks)
		{
			sink->log(*logLine);
		}
		delete logLine;
	}
}

void LogManager::setMaxVerbosity(LogVerbosity verbosity)
{
	m_maxVerbosity = verbosity;
}

} // namespace lmms
