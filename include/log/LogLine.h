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

#include <iomanip>

#include "log/LogManager.h"
#include "log/LogTopic.h"

namespace lmms {

#if defined(WIN32) || defined(_WIN32)
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

class LogLine
{
public:
	LogVerbosity verbosity;
	time_t timestamp;
	std::string fileName;
	unsigned int fileLineNumber;
	std::string content;
	LogTopic topic;

	LogLine(
		LogVerbosity verbosity, std::string fileName, unsigned int fileLineNumber, std::string content, LogTopic topic)
		: verbosity(verbosity)
		, fileName(fileName)
		, fileLineNumber(fileLineNumber)
		, content(content)
		, topic(topic)
	{
		/* Fill the timestamp information */
		timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		/* Check if the file name is a basename. If not, drop the directory
		 * information */
		if (this->fileName.find(PATH_SEPARATOR) != std::string::npos)
		{
			this->fileName = this->fileName.substr(this->fileName.rfind(PATH_SEPARATOR) + 1);
		}
	}

	std::string toString() const
	{
		std::ostringstream os;

		if (topic != LogTopic::Default()) { os << "(" << topic.name() << ") "; }

		os << logVerbosityToString(verbosity) << ": ";

		os << std::put_time(std::localtime(&timestamp), "%T");

		if (fileLineNumber != 0) { os << ": " << fileName << "#" << fileLineNumber; }

		os << ": " << content;

		return os.str();
	}
};

} // namespace lmms

#endif