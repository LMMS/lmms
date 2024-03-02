#include <iostream>
#include "ConsoleLogSink.h"

ConsoleLogSink::ConsoleLogSink()
	: LogSink(LogSinkType::Console)
{
}

void ConsoleLogSink::onLogLine(const LogLine& line)
{
	std::cout << line.toString() << std::endl;
}
