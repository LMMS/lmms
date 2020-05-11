#ifndef DEBUG_TRACE_H
#define DEBUG_TRACE_H

#include <string>
#include <vector>

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
		const char* fileName,
		unsigned int fileLineNo,
		const char* content);

	std::string toString() const;
};

class LogSink
{
public:
	LogSink(LogVerbosity maxVerbosity, LogManager& logManager);
	virtual ~LogSink();

	virtual void onLogLine(const LogLine& line) = 0;

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

	void push(const LogLine& logLine);
	void flush();

private:
	LogManager();

	bool m_flushPaused;
	LogVerbosity m_maxVerbosity;
	std::vector<LogSink*> m_sinks;
	std::vector<LogLine> m_pendingLogLines;
};

#define Log_Gen(verb,format,...)                                               \
	do {                                                                   \
		char _content[1024];                                           \
		snprintf(_content, sizeof(_content), format, ##__VA_ARGS__);   \
		LogLine _logLine(verb, __FILE__, __LINE__, _content);          \
		LogManager::inst().push(std::move(_logLine));                  \
	} while(0)

#define Log_Fatal(format,...) Log_Gen(LogVerbosity::Fatal, format, ##__VA_ARGS__);
#define Log_Err(format,...) Log_Gen(LogVerbosity::Error, format, ##__VA_ARGS__);
#define Log_Wrn(format,...) Log_Gen(LogVerbosity::Warning, format, ##__VA_ARGS__);
#define Log_Inf(format,...) Log_Gen(LogVerbosity::Info, format, ##__VA_ARGS__);
#define Log_Dbg_Lo(format, ...) Log_Gen(LogVerbosity::Debug_Lo, format, ##__VA_ARGS__);
#define Log_Dbg_Hi(format, ...) Log_Gen(LogVerbosity::Debug_Hi, format, ##__VA_ARGS__);

#endif // DEBUG_TRACE_H
