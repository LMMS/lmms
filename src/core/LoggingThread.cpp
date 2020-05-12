#include "LoggingThread.h"
#include "Logging.h"

static const int DEFAULT_FLUSH_INTERVAL = 1000;

LoggingThread::LoggingThread()
	: m_flushInterval(DEFAULT_FLUSH_INTERVAL)
	, m_active(true)
{
}

LoggingThread::~LoggingThread()
{
	m_active = false;
	if (!wait(m_flushInterval)) {
		terminate();
	}

	/* Forcifully flush all the messages that still are present
	 * in the buffer - don't care about the performance, as probably
	 * we are shutting down right now.*/
	LogManager::inst().flush();
}

LoggingThread& LoggingThread::inst()
{
	static LoggingThread instance;
	return instance;
}

void LoggingThread::run()
{
	while (m_active)
	{
		if (!LogManager::inst().isFlushPaused())
		{
			LogManager::inst().flush();
		}
		msleep(DEFAULT_FLUSH_INTERVAL);
	}
	this->exit();
}
