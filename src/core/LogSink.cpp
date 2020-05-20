#include "LogSink.h"

LogSink::LogSink(LogSinkType sinkType)
	: m_sinkType(sinkType)
	, m_defaultMaxVerbosity(LogVerbosity::Info)
{
}

LogSink::~LogSink()
{
}


void LogSink::setMaxVerbosity(std::string topicName, LogVerbosity verbosity)
{
	LogTopic topic(topicName);
	m_topicToMaxVerbosity[topic] = verbosity;
}

void LogSink::setDefaultMaxVerbosity(LogVerbosity verbosity)
{
	m_defaultMaxVerbosity = verbosity;
}

bool LogSink::canAcceptLogLine(LogTopic topic, LogVerbosity verbosity)
{
	std::map<LogTopic, LogVerbosity>::const_iterator topicVerbosity =
			m_topicToMaxVerbosity.find(topic);

	if (topicVerbosity != m_topicToMaxVerbosity.end())
	{
		return topicVerbosity->second >= verbosity;
	}
	else
	{
		return m_defaultMaxVerbosity >= verbosity;
	}
}
