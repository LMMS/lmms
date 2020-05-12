#include "Logging.h"
#include <sstream>
#include <iomanip>
#include <sys/time.h>

const int LOG_QUEUE_SIZE = 256;
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
	os << "-" << fileName << ":" << fileLineNo;
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


LogManager& LogManager::inst()
{
	static LogManager instance;
	return instance;
}

LogManager::LogManager()
{
	m_pendingLogLines.reserve(LOG_QUEUE_SIZE);
	m_flushPaused = false;
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
	m_flushPaused = true;
}

void LogManager::resumeFlush()
{
	m_flushPaused = false;
}

void LogManager::push(const LogLine& logLine)
{
	if (m_maxVerbosity > logLine.verbosity) return;

	if (m_pendingLogLines.size() < LOG_QUEUE_SIZE -1)
	{
		m_pendingLogLines.push_back(logLine);
	}
	else if (m_pendingLogLines.size() == LOG_QUEUE_SIZE - 1)
	{
		Log_Wrn("Log queue overflow, some lines might be dropped");
	}

	if (!m_flushPaused) flush();
}

void LogManager::flush()
{
	for (LogSink* pSink: m_sinks)
	{
		for (LogLine& logLine: m_pendingLogLines)
		{
			if (logLine.verbosity <= pSink->getMaxVerbosity())
			{
				pSink->onLogLine(logLine);
			}
		}
	}
	m_pendingLogLines.clear();
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

