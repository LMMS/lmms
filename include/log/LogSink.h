/*
 * LogSink.h - declaration of the logging sink base class
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

#ifndef LOGSINK_H
#define LOGSINK_H

#include "log/LogManager.h"

namespace lmms {

//! A class which accepts and handles messages logged by LMMS.
//! Example uses include logging to the console or a file.
class LogSink
{
public:
	virtual ~LogSink(){};

	virtual void log(const LogLine& line) = 0;
};

} // namespace lmms

#endif // LOGSINK_H
