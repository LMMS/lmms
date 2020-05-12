#include "Logging.h"
#include "LoggingThread.h"
#include <sstream>
#include <iomanip>
#include <sys/time.h>

const int LOG_BUFFER_SIZE = 256;
const unsigned int USEC_PER_SEC = 1000000;

#if defined(WIN32) || defined(_WIN32)
	#define PATH_SEPARATOR "\\"
#else
	#define PATH_SEPARATOR "/"
#endif


LogLine::LogLine(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content)
{
	static unsigned int logLineNo = 0;
	static unsigned long int initialTimestamp = 0;
	struct timeval tv;

	this->verbosity = verbosity;
	this->fileLineNo = fileLineNo;
	this->fileName = fileName;
	this->content = content;
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
	static_assert(sizeof(VERBOSITY_LETTERS) == (int)LogVerbosity::Last);

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

	os << ": " << content;

	return os.str();
}

LogSink::LogSink(LogVerbosity maxVerbosity, LogManager& logManager)
	: m_maxVerbosity(maxVerbosity), m_logManager(logManager)
{
}

LogSink::~LogSink()
{
}

void LogSink::onTermination()
{
}


LogManager& LogManager::inst()
{
	static LogManager instance;
	return instance;
}

LogManager::LogManager()
{
	m_pendingLogLines.reserve(LOG_BUFFER_SIZE);
	m_flushPaused = false;
}

LogManager::~LogManager()
{
	for (LogSink* pSink: m_sinks)
	{
		pSink->onTermination();
		delete pSink;
	}
}

void LogManager::addSink(LogSink* sink)
{
	m_sinks.push_back(sink);
}

void LogManager::pauseFlush()
{
	m_flushPausedMutex.lock();
	m_flushPaused = true;
	m_flushPausedMutex.unlock();
}

void LogManager::resumeFlush()
{
	m_flushPausedMutex.lock();
	m_flushPaused = false;
	m_flushPausedMutex.unlock();

	/* If the logging thread is not used, flush immediately the log lines
	 * that were generated in performance-critical section. */
	if (!LoggingThread::inst().isRunning())
	{
		flush();
	}
}

bool LogManager::isFlushPaused() const
{
	bool result;

	m_flushPausedMutex.lock();
	result = m_flushPaused;
	m_flushPausedMutex.unlock();

	return result;
}


void LogManager::push(const LogLine logLine)
{
	if (m_maxVerbosity > logLine.verbosity) return;

	/* The logger thread might be now caching the log lines with the intent
	 * of flushing them. It is better not to interfere */
	m_pendingLogLinesMutex.lock();

	if (m_pendingLogLines.size() < LOG_BUFFER_SIZE -1)
	{
		m_pendingLogLines.push_back(logLine);
	}
	else if (m_pendingLogLines.size() == LOG_BUFFER_SIZE - 1)
	{
		Log_Wrn("Log queue overflow, some lines might be dropped");
	}

	m_pendingLogLinesMutex.unlock();

	/* If the logging thread is not used, make the log message appear
	 * immediately unless we are in a performance-critical section */
	if (!isFlushPaused() && !LoggingThread::inst().isRunning())
	{
		flush();
	}
}

void LogManager::push(LogVerbosity verbosity,
			std::string fileName,
			unsigned int fileLineNo,
			std::string content)
{
	LogLine logLine(verbosity, fileName, fileLineNo, content);
	push(std::move(logLine));
}

void LogManager::flush()
{
	m_pendingLogLinesMutex.lock();
	std::vector<LogLine> pendingLogLines(m_pendingLogLines);
	m_pendingLogLines.clear();
	m_pendingLogLinesMutex.unlock();

	/* We have saved all the log lines and cleared the main buffer.
	 * Other threads may now fill up the main buffer again, but they
	 * will not be blocked by us writing down (potentially slowly)
	 * the contents of the buffer. */

	for (LogLine& logLine: pendingLogLines)
	{
		for (LogSink* pSink: m_sinks)
		{
			if (logLine.verbosity <= pSink->getMaxVerbosity())
			{
				pSink->onLogLine(logLine);
			}
		}
	}
}

LogIostreamWrapper::LogIostreamWrapper(LogVerbosity verbosity,
					std::string fileName,
					unsigned int fileLineNo)
	: m_line(verbosity, fileName, fileLineNo, "")
{
}

LogIostreamWrapper::~LogIostreamWrapper()
{
	m_line.content = this->str();
	LogManager::inst().push(m_line);
}


