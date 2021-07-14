/*
 * LogSink.h - declaration of the logging sink base class and enumeration
 * of supported log sinks
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

#ifndef LOGSINK_H
#define LOGSINK_H

#include "Logging.h"

enum class LogSinkType
{
	Console = 1, // relevant for ConsoleLogSink
	LogFile,     // reserved for future LogFileSink
	LogWindow    // reserved for future LogWindowSink
};

class LogSink
{
public:
	explicit LogSink(LogSinkType sinkType);
	virtual ~LogSink();

	virtual void onFlushBegin() {}
	virtual void onFlushEnd() {}
	virtual void onLogLine(const LogLine& line) = 0;

	void setMaxVerbosity(std::string topicName, LogVerbosity verbosity);
	void setDefaultMaxVerbosity(LogVerbosity verbosity);
	bool canAcceptLogLine(LogTopic topic, LogVerbosity verbosity);

private:
	LogSinkType m_sinkType;
	std::map<LogTopic, LogVerbosity> m_topicToMaxVerbosity;
	LogVerbosity m_defaultMaxVerbosity;
};

#endif // LOGSINK_H
