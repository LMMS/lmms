/*
 * LogLine.h - a class which holds information about a log entry

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

#ifndef LOGLINE_H
#define LOGLINE_H

#include <chrono>
#include <iomanip>

#include "log/LogTopic.h"

namespace lmms {

// The character used to split the __FILE__ macro to take only the file name.
#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

//! The level of detail of a log message. "Trace" produces the most detailed logs, while "Fatal" logs only fatal errors.
//! For performance reasons, "Trace" messages are only logged in debug builds.
enum class LogVerbosity
{
	Fatal,
	Error,
	Warning,
	Info,
	Trace
};

class LogLine
{
public:
	LogVerbosity verbosity;
	//! The time at which the log line was recorded.
	std::chrono::system_clock::time_point timestamp;
	//! The name of the file from which the logging call originated. Usually populated by the __FILE__ macro. Useful for
	//! debugging.
	std::string fileName;
	//! The line number of the file from which the logging called originated. Usually populated by the __LINE__ macro.
	//! Useful for debugging.
	unsigned int fileLineNumber;
	//! The message being logged. Often formatted with positional arguments in LogManager.
	std::string content;
	//! The category of the log message. Can be checked in a log sink to filter log messages to only a specific
	//! category.
	LogTopic topic;

	LogLine(
		LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, std::string content, LogTopic topic);

	//! Generates the string representation of this log line. Presumably used in sinks.
	std::string toString() const;

	// Helper functions to convert between log verbosities and strings.

	static std::string logVerbosityToString(LogVerbosity verbosity);
	static LogVerbosity stringToLogVerbosity(std::string s);
};

} // namespace lmms

#endif