/*
 * ConsoleLogSink.h - the LogSink implementation that writes output to console.
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

#ifndef CONSOLE_LOG_SINK_H
#define CONSOLE_LOG_SINK_H

#include <iostream>

#include "log/LogLine.h"
#include "log/LogSink.h"

namespace lmms {

class ConsoleLogSink : public LogSink
{
public:
	~ConsoleLogSink() override{};
	void log(const LogLine& line) override { std::cout << line.toString() << std::endl; }
};

} // namespace lmms

#endif // CONSOLE_LOG_SINK_H
