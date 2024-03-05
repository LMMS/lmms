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

class LogTopic
{
public:
	LogTopic(std::string name);
	std::string name() const;

	static LogTopic& Default()
	{
		static LogTopic instance("");
		return instance;
	}

	bool operator==(LogTopic& other) const { return this->m_name == other.m_name; }

private:
	std::string m_name;
};

} // namespace lmms

#endif