/*
 * Logging.h - classes and macros related to native LMMS' logging API
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

#ifndef DEBUG_TRACE_H
#define DEBUG_TRACE_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include "ringbuffer/ringbuffer.h"
#include <atomic>
#include <Qt>

class LogManager;

enum class LogVerbosity
{
	Fatal = 0,
	Error,
	Warning,
	Info,
	Debug_Lo,
	Debug_Hi,

	Last
};

LogVerbosity stringToLogVerbosity(std::string s);

class LogTopic
{
public:
	LogTopic(std::string name);
	std::string name() const;
	int id() const { return m_id; }
private:
	int m_id;
	static std::map<int, std::string> ms_topicIds;
};

extern LogTopic LT_Default;

struct LogLine
{
	LogVerbosity verbosity;
	unsigned int logLineNo;
	unsigned long int timestamp;
	std::string fileName;
	unsigned int fileLineNo;
	std::string content;
	LogTopic topic;
	Qt::HANDLE thread;

	LogLine(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content,
		LogTopic topic);

	std::string toString() const;
};

class LogSink
{
public:
	LogSink(LogVerbosity maxVerbosity, LogManager& logManager);
	virtual ~LogSink();

	virtual void onLogLine(const LogLine& line) = 0;
	virtual void onTermination();

	LogVerbosity getMaxVerbosity()
	{
		return m_maxVerbosity;
	}

	void setMaxVerbosity(LogVerbosity verbosity);

private:
	LogVerbosity m_maxVerbosity;
	LogManager& m_logManager;
};

class LogManager
{
public:
	static LogManager& inst();
	~LogManager();
	void addSink(LogSink* sink);

	void pauseFlush();
	void resumeFlush();
	bool isFlushPaused() const;

	void push(LogLine *logLine);
	void push(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content,
		LogTopic topic);

	void flush();
	void notifyVerbosityChanged();
	void registerCurrentThread(std::string name);
	std::string threadName(Qt::HANDLE threadId);

private:
	LogManager();

	std::atomic_bool m_flushPaused;
	LogVerbosity m_maxVerbosity;
	std::vector<LogSink*> m_sinks;
	ringbuffer_t<LogLine*> m_pendingLogLines;
	ringbuffer_reader_t<LogLine*> m_pendingLogLinesReader;

	static std::map<Qt::HANDLE, std::string> ms_threadIds;
};

class LogIostreamWrapper: public std::ostringstream
{
public:
	LogIostreamWrapper(LogVerbosity verbosity,
			std::string fileName,
			unsigned int fileLineNo,
			LogTopic topic);

	~LogIostreamWrapper() override;

private:
	LogLine* m_pLine;
};

#define Log_Gen(verb,topic,format,...)                                             \
	do {                                                                       \
		char _content[1024];                                               \
		snprintf(_content, sizeof(_content), format, ##__VA_ARGS__);       \
		LogManager::inst().push(verb, __FILE__, __LINE__,                  \
					_content, topic);                          \
	} while(0)

#define Log_Fatal(topic,format,...) \
	Log_Gen(LogVerbosity::Fatal, topic, format, ##__VA_ARGS__);

#define Log_Err(topic,format,...) \
	Log_Gen(LogVerbosity::Error, topic, format, ##__VA_ARGS__);

#define Log_Wrn(topic,format,...) \
	Log_Gen(LogVerbosity::Warning, topic, format, ##__VA_ARGS__);

#define Log_Inf(topic,format,...) \
	Log_Gen(LogVerbosity::Info, topic, format, ##__VA_ARGS__);

#define Log_Dbg_Lo(topic,format, ...) \
	Log_Gen(LogVerbosity::Debug_Lo, topic, format, ##__VA_ARGS__);

#define Log_Dbg_Hi(topic,format, ...) \
	Log_Gen(LogVerbosity::Debug_Hi, topic, format, ##__VA_ARGS__);

#define Log_Str_Gen(verb, topic) LogIostreamWrapper(verb, __FILE__, __LINE__, topic)
#define Log_Str_Fatal(topic) Log_Str_Gen(LogVerbosity::Fatal, topic)
#define Log_Str_Err(topic) Log_Str_Gen(LogVerbosity::Error, topic)
#define Log_Str_Wrn(topic) Log_Str_Gen(LogVerbosity::Warning, topic)
#define Log_Str_Inf(topic) Log_Str_Gen(LogVerbosity::Info, topic)
#define Log_Str_Dbg_Lo(topic) Log_Str_Gen(LogVerbosity::Debug_Lo, topic)
#define Log_Str_Dbg_Hi(topic) Log_Str_Gen(LogVerbosity::Debug_Hi, topic)

#endif // DEBUG_TRACE_H
