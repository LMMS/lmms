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
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "log/LogLine.h"
#include "log/LogTopic.h"
#include "ringbuffer/ringbuffer.h"

namespace lmms {

class LogSink;

class LogManager
{
public:
	static LogManager& inst();
	~LogManager();
	void addSink(LogSink* sink);

	void push(LogLine* logLine);
	template <typename... Args>
	void push(LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, LogTopic topic,
		std::string content, Args... format_args)
	{
		if (m_maxVerbosity < verbosity) return;

		// Format the log string with the provided arguments.
		int size_s = std::snprintf(nullptr, 0, content.c_str(), format_args...) + 1; // Extra space for '\0'
		if (size_s <= 0)
		{
			push(new LogLine(
				lmms::LogVerbosity::Error, "myfile", 3, "Error formatting log message.", lmms::LogTopic::Default()));
			return;
		}
		auto size = static_cast<size_t>(size_s);
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, content.c_str(), format_args...);
		std::string message(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside

		LogLine* logLine = new LogLine(verbosity, fileName, fileLineNumber, message, topic);
		push(logLine);
	}

	void flush();

	void setMaxVerbosity(LogVerbosity verbosity);

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
