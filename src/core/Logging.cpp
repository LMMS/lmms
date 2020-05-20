/*
 * Logging.cpp - implementation of member functions of classes related
 *               to logging
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
#include "Logging.h"
#include "LoggingThread.h"
#include <sstream>
#include <iomanip>
#include <map>
#include <sys/time.h>
#include <QThread>
#include "LogSink.h"

const int LOG_BUFFER_SIZE = 1024;
const unsigned int USEC_PER_SEC = 1000000;

std::map<int, std::string>  LogTopic::ms_topicIds;
std::map<Qt::HANDLE, std::string> LogManager::ms_threadIds;

LogTopic LT_Default("default");

#if defined(WIN32) || defined(_WIN32)
	#define PATH_SEPARATOR "\\"
#else
	#define PATH_SEPARATOR "/"
#endif

LogTopic::LogTopic(std::string name)
{
	static int nextId = 1;

	/* Check if a topic has not been already registered. If yes, substitute
	 * the identifier rather than generating a new one */
	for (const std::pair<int, std::string>& idName: ms_topicIds)
	{
		if (idName.second == name)
		{
			m_id = idName.first;
			return;
		}
	}

	m_id = nextId++;
	ms_topicIds[m_id] = name;
}

std::string LogTopic::name() const
{
	return ms_topicIds[m_id];
}

LogLine::LogLine(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content,
		LogTopic topic)
	: verbosity(verbosity)
	, fileLineNo(fileLineNo)
	, content(content)
	, topic(topic)
{
	static unsigned int logLineNo = 0;
	static unsigned long int initialTimestamp = 0;
	struct timeval tv;

	this->fileName = fileName;
	this->logLineNo = logLineNo++;

	/* Fill the timestamp information */
	gettimeofday(&tv, nullptr);
	this->timestamp = tv.tv_sec * USEC_PER_SEC + tv.tv_usec;

	/* If the timestamp was not set yet, initialize it with the value
	 * we have just obtained - this way the very first log line will
	 * always have the timestamp of 0 */
	if (initialTimestamp == 0)
	{
		initialTimestamp = this->timestamp;
	}

	this->timestamp -= initialTimestamp;

	/* Check if the file name is a basename. If not, drop the directory
	 * information */
	if (this->fileName.find(PATH_SEPARATOR) != std::string::npos)
	{
		this->fileName = this->fileName.substr(
					this->fileName.rfind(PATH_SEPARATOR)+1);
	}

}

std::string LogLine::toString() const
{
	const char VERBOSITY_LETTERS[] = {'F', 'E', 'W', 'I', 'L', 'H'};
	static_assert(sizeof(VERBOSITY_LETTERS) == (int)LogVerbosity::Last,
		      "The set of verbosity letters does not match the enum");

	std::ostringstream os;
	if (verbosity < LogVerbosity::Last)
	{
		os << VERBOSITY_LETTERS[(int)verbosity];
	}
	else
	{
		os << "X";
	}

	os << "-" << logLineNo;
	os << "-" << (timestamp / USEC_PER_SEC);
	os << "." << std::setw(6) << std::setfill('0') << (timestamp % USEC_PER_SEC);

	if (fileLineNo != 0)
	{
		os << "-" << fileName << ":" << fileLineNo;
	}
	else
	{
		os << "-?";
	}

	os << "-[" << LogManager::inst().threadName(thread) << "]";

	os << "-#" << topic.name();

	os << ": " << content;

	return os.str();
}

LogManager& LogManager::inst()
{
	static LogManager instance;
	return instance;
}

LogManager::LogManager()
	: m_pendingLogLines(LOG_BUFFER_SIZE)
	, m_pendingLogLinesReader(m_pendingLogLines)
{
	m_flushPaused.store(false);
}

LogManager::~LogManager()
{
	for (LogSink* pSink: m_sinks)
	{
		delete pSink;
	}
}

void LogManager::addSink(LogSink* sink)
{
	m_sinks.push_back(sink);
}

void LogManager::pauseFlush()
{
	m_flushPaused.store(true);
}

void LogManager::resumeFlush()
{
	m_flushPaused.store(false);
	if (LoggingThread::inst().isRunning())
	{
		flush();
	}
}

bool LogManager::isFlushPaused() const
{
	return m_flushPaused.load();
}


void LogManager::push(LogLine* logLine)
{
	logLine->thread = QThread::currentThreadId();
	if (ms_threadIds.find(logLine->thread) == ms_threadIds.end())
	{
		/* Generate default thread name */
		registerCurrentThread("");
	}

	if (m_pendingLogLines.write_space() > 1)
	{
		m_pendingLogLines.write(&logLine, 1);
	}
	else if (m_pendingLogLines.write_space() == 1)
	{
		Log_Wrn(LT_Default,
			"Log queue overflow, some lines might be dropped");
	}

	/* If the logging thread is not used, make the log message appear
	 * immediately unless we are in a performance-critical section */
	if (!isFlushPaused() && !LoggingThread::inst().isRunning())
	{
		flush();
	}

	/* In case of fatal error flush the messages immediately,
	 * then terminate the application */
	if (logLine->verbosity == LogVerbosity::Fatal)
	{
		flush();
		abort();
	}
}

void LogManager::push(LogVerbosity verbosity,
			std::string fileName,
			unsigned int fileLineNo,
			std::string content,
			LogTopic topic)
{
	LogLine* logLine = new LogLine(verbosity,fileName, fileLineNo,
					content, topic);
	push(logLine);
}

void LogManager::flush()
{	
	auto seq = m_pendingLogLinesReader.read_max(LOG_BUFFER_SIZE);

	for (size_t ix = 0; ix < seq.size(); ix++)
	{
		LogLine* logLine = seq[ix];
		for (LogSink* pSink: m_sinks)
		{
			if (pSink->canAcceptLogLine(logLine->topic,
							logLine->verbosity))
			{
				pSink->onLogLine(*logLine);
			}
		}
		delete logLine;
	}
}

void LogManager::registerCurrentThread(std::string name)
{
	static int unnamedThreadNextId = 1;
	if (name.empty())
	{
		std::ostringstream os;
		os << "Thread" << unnamedThreadNextId++;
		name = os.str();
	}
	ms_threadIds[QThread::currentThreadId()] = name;
}

std::string LogManager::threadName(Qt::HANDLE threadId)
{
	return ms_threadIds.at(threadId);
}


LogIostreamWrapper::LogIostreamWrapper(LogVerbosity verbosity,
					std::string fileName,
					unsigned int fileLineNo,
					LogTopic topic)
	: m_pLine(new LogLine(verbosity, fileName, fileLineNo, "", topic))
{
}

LogIostreamWrapper::~LogIostreamWrapper()
{
	m_pLine->content = this->str();
	LogManager::inst().push(m_pLine);
	/* m_pLine will be freed by the LogManager */
}

LogVerbosity stringToLogVerbosity(std::string s)
{
	static const std::map<std::string, LogVerbosity> mapping = {
		{"fatal", LogVerbosity::Fatal},
		{"error", LogVerbosity::Error},
		{"warning", LogVerbosity::Warning},
		{"info", LogVerbosity::Info},
		{"debug_lo", LogVerbosity::Debug_Lo},
		{"debug_hi", LogVerbosity::Debug_Hi}
	};
	return mapping.at(s);
}
