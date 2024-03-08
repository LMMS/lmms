/*
 * LogLine.cpp - a class which holds information about a log entry

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

#include "log/LogLine.h"

#include "LoggingMacros.h"

namespace lmms {
LogLine::LogLine(
	LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, std::string content, LogTopic topic)
	: verbosity(verbosity)
	, fileName(fileName)
	, fileLineNumber(fileLineNumber)
	, content(content)
	, topic(topic)
{
	/* Fill the timestamp information */
	timestamp = std::chrono::system_clock::now();

	/* Check if the file name is a basename. If not, drop the directory
	 * information */
	if (this->fileName.find(PATH_SEPARATOR) != std::string::npos)
	{
		this->fileName = this->fileName.substr(this->fileName.rfind(PATH_SEPARATOR) + 1);
	}
}
std::string LogLine::toString() const
{
	std::ostringstream os;

	if (topic != LogTopic::Default()) { os << "(" << topic.name() << ") "; }

	os << logVerbosityToString(verbosity) << ": ";

	time_t timestampT = std::chrono::system_clock::to_time_t(timestamp);
	std::chrono::milliseconds timestampMs
		= std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch());
	os << std::put_time(std::localtime(&timestampT), "%T") << "." << (timestampMs % std::chrono::seconds(1)).count();

	if (fileLineNumber != 0) { os << ": " << fileName << "#" << fileLineNumber; }

	os << ": " << content;

	return os.str();
}

const std::vector<std::pair<std::string, LogVerbosity>> verbosityMapping
	= {{"Fatal", LogVerbosity::Fatal}, {"Error", LogVerbosity::Error}, {"Warning", LogVerbosity::Warning},
		{"Info", LogVerbosity::Info}, {"Trace", LogVerbosity::Trace}};

std::string LogLine::logVerbosityToString(LogVerbosity verbosity)
{
	auto it = std::find_if(
		verbosityMapping.begin(), verbosityMapping.end(), [verbosity](const auto& p) { return p.second == verbosity; });
	if (it != verbosityMapping.end()) return it->first;
	LOG_WARN("Log verbosity %i not found.", (int)verbosity);
	return "Info";
}

LogVerbosity LogLine::stringToLogVerbosity(std::string s)
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

} // namespace lmms