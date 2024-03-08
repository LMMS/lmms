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

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

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
	std::chrono::system_clock::time_point timestamp;
	std::string fileName;
	unsigned int fileLineNumber;
	std::string content;
	LogTopic topic;

	LogLine(
		LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, std::string content, LogTopic topic);

	std::string toString() const;

	static std::string logVerbosityToString(LogVerbosity verbosity);
	static LogVerbosity stringToLogVerbosity(std::string s);
};

} // namespace lmms

#endif