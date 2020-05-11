#ifndef CONSOLE_LOG_SINK_H
#define CONSOLE_LOG_SINK_H

#include "Logging.h"

class ConsoleLogSink: public LogSink
{
public:
	ConsoleLogSink(LogVerbosity maxVerbosity, LogManager& logManager);
	void onLogLine(const LogLine& line) override;
	~ConsoleLogSink() = default;
};

#endif // CONSOLE_LOG_SINK_H
