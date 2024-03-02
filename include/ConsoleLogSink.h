#ifndef CONSOLE_LOG_SINK_H
#define CONSOLE_LOG_SINK_H

#include "LogSink.h"

class ConsoleLogSink: public LogSink
{
public:
	ConsoleLogSink();
	void onLogLine(const LogLine& line) override;
	~ConsoleLogSink() = default;
};

#endif // CONSOLE_LOG_SINK_H
