#ifndef DEBUG_TRACE_H
#define DEBUG_TRACE_H

#include <string>
#include <sstream>
#include <vector>
#include <QMutex>
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

struct LogLine
{
	LogVerbosity verbosity;
	unsigned int logLineNo;
	unsigned long int timestamp;
	std::string fileName;
	unsigned int fileLineNo;
	std::string content;

	LogLine(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content);

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

	void push(const LogLine logLine);
	void push(LogVerbosity verbosity,
		std::string fileName,
		unsigned int fileLineNo,
		std::string content);

	void flush();

private:
	LogManager();

	bool m_flushPaused;
	LogVerbosity m_maxVerbosity;
	std::vector<LogSink*> m_sinks;
	std::vector<LogLine> m_pendingLogLines;

	mutable QMutex m_flushPausedMutex;
	QMutex m_pendingLogLinesMutex;
};

class LogIostreamWrapper: public std::ostringstream
{
public:
	LogIostreamWrapper(LogVerbosity verbosity,
			std::string fileName,
			unsigned int fileLineNo);

	~LogIostreamWrapper() override;

private:
	LogLine m_line;
};

#define Log_Gen(verb,format,...)                                               \
	do {                                                                   \
		char _content[1024];                                           \
		snprintf(_content, sizeof(_content), format, ##__VA_ARGS__);   \
		LogManager::inst().push(verb, __FILE__, __LINE__, _content);   \
	} while(0)

#define Log_Fatal(format,...) Log_Gen(LogVerbosity::Fatal, format, ##__VA_ARGS__);
#define Log_Err(format,...) Log_Gen(LogVerbosity::Error, format, ##__VA_ARGS__);
#define Log_Wrn(format,...) Log_Gen(LogVerbosity::Warning, format, ##__VA_ARGS__);
#define Log_Inf(format,...) Log_Gen(LogVerbosity::Info, format, ##__VA_ARGS__);
#define Log_Dbg_Lo(format, ...) Log_Gen(LogVerbosity::Debug_Lo, format, ##__VA_ARGS__);
#define Log_Dbg_Hi(format, ...) Log_Gen(LogVerbosity::Debug_Hi, format, ##__VA_ARGS__);

#define Log_Str_Gen(verb) LogIostreamWrapper(verb, __FILE__, __LINE__)
#define Log_Str_Fatal Log_Str_Gen(LogVerbosity::Fatal)
#define Log_Str_Err Log_Str_Gen(LogVerbosity::Error)
#define Log_Str_Wrn Log_Str_Gen(LogVerbosity::Warning)
#define Log_Str_Inf Log_Str_Gen(LogVerbosity::Info)
#define Log_Str_Dbg_Lo Log_Str_Gen(LogVerbosity::Debug_Lo)
#define Log_Str_Dbg_Hi Log_Str_Gen(LogVerbosity::Debug_Hi)

#endif // DEBUG_TRACE_H
