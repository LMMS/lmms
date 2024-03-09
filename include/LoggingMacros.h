/*
 * LoggingMacros.h - a collection of macros to provide straightforward access to LMMS' logging API.
 *
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

#ifndef LOGGING_MACROS_H
#define LOGGING_MACROS_H

#include "log/LogManager.h"

// This first set of macros logs messages that are assigned a specific LogTopic.

#define LOG_TOPIC(verbosity, topic, format, ...) \
	lmms::LogManager::inst().push(verbosity, __FILE__, __LINE__, LT(#topic), format, ##__VA_ARGS__)

#define LOG_FATAL_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Fatal, topic, format, ##__VA_ARGS__)

#define LOG_ERR_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Error, topic, format, ##__VA_ARGS__)

#define LOG_WARN_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Warning, topic, format, ##__VA_ARGS__)

#define LOG_INFO_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Info, topic, format, ##__VA_ARGS__)

// For performance reasons, trace messages are only logged in debug builds.
#ifdef LMMS_DEBUG
#define LOG_TRACE_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Trace, topic, format, ##__VA_ARGS__)
#else
#define LOG_TRACE_TOPIC(topic, format, ...)
#endif

// This group of macros logs general messages using the default topic, so that no specific topic will be written to the
// log output.

#define LOG_FATAL(format, ...) LOG_FATAL_TOPIC(, format, ##__VA_ARGS__)

#define LOG_ERR(format, ...) LOG_ERR_TOPIC(, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) LOG_WARN_TOPIC(, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) LOG_INFO_TOPIC(, format, ##__VA_ARGS__)

#define LOG_TRACE(format, ...) LOG_TRACE_TOPIC(, format, ##__VA_ARGS__)

#endif // LOGGING_MACROS_H