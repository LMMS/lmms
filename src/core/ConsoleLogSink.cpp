#include <iostream>
#include "ConsoleLogSink.h"

ConsoleLogSink::ConsoleLogSink(LogVerbosity maxVerbosity, LogManager& logManager)
	: LogSink(maxVerbosity, logManager)
{
}

void ConsoleLogSink::onLogLine(const LogLine& line)
{
	std::cout << line.toString() << std::endl;
}
