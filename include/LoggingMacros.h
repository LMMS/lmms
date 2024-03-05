/*
 * LoggingMacros.h - macros related to LMMS' logging API.
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

#define LOG_TOPIC(verbosity, topic, format, ...) \
	lmms::LogManager::inst().push(verbosity, __FILE__, __LINE__, topic, format, ##__VA_ARGS__)

#define LOG_FATAL_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Fatal, topic, format, ##__VA_ARGS__)

#define LOG_ERR_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Error, topic, format, ##__VA_ARGS__)

#define LOG_WARN_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Warning, topic, format, ##__VA_ARGS__)

#define LOG_INFO_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Info, topic, format, ##__VA_ARGS__)

#ifdef LMMS_DEBUG
#define LOG_TRACE_TOPIC(topic, format, ...) LOG_TOPIC(lmms::LogVerbosity::Trace, topic, format, ##__VA_ARGS__);
#else
#define LOG_TRACE_TOPIC(topic, format, ...)
#endif

#define LOG_FATAL(format, ...) LOG_FATAL_TOPIC(lmms::LogTopic::Default(), format, ##__VA_ARGS__)

#define LOG_ERR(format, ...) LOG_ERR_TOPIC(lmms::LogTopic::Default(), format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) LOG_WARN_TOPIC(lmms::LogTopic::Default(), format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) LOG_INFO_TOPIC(lmms::LogTopic::Default(), format, ##__VA_ARGS__)

#define LOG_TRACE(format, ...) LOG_TRACE_TOPIC(lmms::LogTopic::Default(), format, ##__VA_ARGS__);

#endif // LOGGING_MACROS_H