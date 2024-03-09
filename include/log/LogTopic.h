/*
 * LogTopic.h - a class which describes a category of log messages.

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

#ifndef LOGTOPIC_H
#define LOGTOPIC_H

#include <string>

namespace lmms {

// A helper macro which allows for rapid caching of LogTopics after the first time they are constructed.
// Suitable for use in realtime functions.
#define LT(topic) \
	([]() -> const lmms::LogTopic& { \
		static lmms::LogTopic lt = lmms::LogTopic(topic); \
		return lt; \
	})()

class LogTopic
{
public:
	LogTopic(std::string name);
	std::string name() const;

	// The default log topic, used when one is not specifically provided.
	static const LogTopic& Default() { return LT(""); }

	bool operator==(const LogTopic& other) const { return this->m_name == other.m_name; }
	bool operator!=(const LogTopic& other) const { return this->m_name != other.m_name; }

private:
	//! The name of the topic, which entirely defines it.
	std::string m_name;
};

} // namespace lmms

#endif