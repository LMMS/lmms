/*
 * Logging.h - classes and macros related to LMMS' logging API
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

#ifndef LOGGING_H
#define LOGGING_H

#include <Qt>
#include <atomic>
#include <chrono>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "log/LogTopic.h"
#include "ringbuffer/ringbuffer.h"

namespace lmms {

class LogManager;

enum class LogVerbosity
{
	Fatal,
	Error,
	Warning,
	Info,
	Trace
};

LogVerbosity stringToLogVerbosity(std::string s);
std::string logVerbosityToString(LogVerbosity verbosity);

class LogSink;
class LogLine;

class LogManager
{
public:
	static LogManager& inst();
	~LogManager();
	void addSink(LogSink* sink);

	void push(LogLine* logLine);
	template <typename... Args>
	void push(LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, LogTopic topic,
		std::string content, Args... format_args);

	void flush();

	void setMaxVerbosity(LogVerbosity verbosity);

	std::map<LogVerbosity, std::string> LogVerbosityNames;

private:
	LogManager();
	LogManager(LogManager const&) = delete;
	void operator=(LogManager const&) = delete;

	LogVerbosity m_maxVerbosity = LogVerbosity::Info;
	std::vector<LogSink*> m_sinks;
	ringbuffer_t<LogLine*> m_pendingLogLines;
	ringbuffer_reader_t<LogLine*> m_pendingLogLinesReader;
};

} // namespace lmms

#endif // LOGGING_H
