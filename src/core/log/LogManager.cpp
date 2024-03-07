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
	m_pendingLogLines.mlock();
	m_pendingLogLines.touch();
}

LogManager::~LogManager()
{
	/* Forcibly flush all the messages that still are present
	 * in the buffer - don't care about the performance, as probably
	 * we are shutting down right now.*/
	flush();

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
	if (m_maxVerbosity >= logLine->verbosity)
	{
		if (m_pendingLogLines.write_space() > 1) { m_pendingLogLines.write(&logLine, 1); }
		else if (m_pendingLogLines.write_space() == 1)
		{
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
}

template <typename... Args>
void LogManager::push(LogVerbosity verbosity, std::string fileName, unsigned int fileLineNo, LogTopic topic,
	std::string content, Args... format_args)
{
	// Format the log string with the provided arguments.
	int size_s = std::snprintf(nullptr, 0, content.c_str(), format_args...) + 1; // Extra space for '\0'
	if (size_s <= 0) { LOG_ERR("Error during log formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, content.c_str(), format_args...);
	std::string message(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside

	LogLine* logLine = new LogLine(verbosity, fileName, fileLineNo, message, topic);
	push(logLine);
}

void LogManager::flush()
{
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

static const std::vector<std::pair<std::string, LogVerbosity>> verbosityMapping
	= {{"Fatal", LogVerbosity::Fatal}, {"Error", LogVerbosity::Error}, {"Warning", LogVerbosity::Warning},
		{"Info", LogVerbosity::Info}, {"Trace", LogVerbosity::Trace}};

LogVerbosity stringToLogVerbosity(std::string s)
{
	auto it = std::find_if(
		verbosityMapping.begin(), verbosityMapping.end(), [s](const std::pair<std::string, LogVerbosity>& p) {
			for (int i = 0; i < s.length(); i++)
			{
				if (std::tolower(p.first.at(i)) != std::tolower(s.at(i))) return false;
			}
			return true;
		});
	if (it != verbosityMapping.end()) return it->second;
	LOG_WARN("Log verbosity %s not found.", s.c_str());
	return LogVerbosity::Info;
}

std::string logVerbosityToString(LogVerbosity verbosity)
{
	auto it = std::find_if(
		verbosityMapping.begin(), verbosityMapping.end(), [verbosity](const auto& p) { return p.second == verbosity; });
	if (it != verbosityMapping.end()) return it->first;
	LOG_WARN("Log verbosity %i not found.", (int)verbosity);
	return "Info";
}

} // namespace lmms
