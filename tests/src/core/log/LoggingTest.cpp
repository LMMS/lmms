/*
 * LoggingTest.cpp - automated tests for LMMS' realtime-safe logging system
 *
 * Copyright (c) 2024 Jonah Janzen
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include <QtTest/QtTest>

#include "LoggingMacros.h"
#include "log/LogLine.h"
#include "log/LogSink.h"
#include "log/LoggingThread.h"

// Because the logs are processed through the logging thread, there is no guarantee that they will be written
// synchronously. This macro is called before all logging assertions in tests to ensure that the most recent messages
// have actually been made available to sinks.
#define FLUSH_LOGS lmms::LogManager::inst().flush()

// The temporary buffer from which messages written by test cases can be accessed.
lmms::LogLine lastLogLine(lmms::LogVerbosity::Trace, "", 0, "", lmms::LogTopic::Default());

class LoggingTest : public QObject
{
	Q_OBJECT

	// A log sink that just assigns each incoming log message to "lastLogLine" so the result of the log call can be
	// analyzed from within the test case.
	class TestLogSink : public lmms::LogSink
	{
	public:
		void log(const lmms::LogLine& line) override { lastLogLine = lmms::LogLine(line); }
	};

private slots:
	void initTestCase()
	{
		// Set up the log manager.
		// The logging thread is automatically initialized by the LogManager constructor.
		lmms::LogManager::inst().addSink(new TestLogSink());
	}

	// Each log level should be properly defined and logged.
	void testLogLevels()
	{
		// Fatal logs are not tested because they abort the application.

		// The max verbosity is explicitly reset at the beginning of each test case to ensure deterministic behaviour.
		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Trace);

		LOG_ERR("Err Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Err Log", "Error logs should be written.");
		QVERIFY2(lastLogLine.verbosity == lmms::LogVerbosity::Error,
			"The verbosity level of an error log should be \"error\".");

		LOG_WARN("Warn Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Warn Log", "Warn logs should be written.");
		QVERIFY2(lastLogLine.verbosity == lmms::LogVerbosity::Warning,
			"The verbosity level of a warn log should be \"warn\".");

		LOG_INFO("Info Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Info Log", "Info logs should be written.");
		QVERIFY2(lastLogLine.verbosity == lmms::LogVerbosity::Info,
			"The verbosity level of an info log should be \"info\".");

#ifdef LMMS_DEBUG
		LOG_TRACE("Trace Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Trace Log", "Trace logs should be written when using a debug build.");
		QVERIFY2(lastLogLine.verbosity == lmms::LogVerbosity::Trace,
			"The verbosity level of a trace log should be \"trace\".");
#else
		LOG_INFO("Pre Trace Log");
		LOG_TRACE("Trace Log");
		FLUSH_LOGS;
		QVERIFY2(
			lastLogLine.content == "Pre Trace Log", "Trace logs should not be written when not using a debug build.");
#endif
	}

	// Log messages should be properly filtered based on the max verbosity of the log manager.
	void testMaxVerbosity()
	{
		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Trace);
		LOG_INFO("Visible Info Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Visible Info Log",
			"Info logs should be written when the maximum log level is \"trace\".");

		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Warning);
		LOG_INFO("Suppressed Info Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content != "Suppressed Info Log",
			"Info logs should not be written when the maximum log level is \"warning\".");

		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Error);
		LOG_ERR("Error Log");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Error Log",
			"Error logs should be written when the maximum log level is \"error\".");
	}

	// General messages should be assigned the default topic, and custom log topics should be properly handled.
	void testLogTopic()
	{
		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Trace);

		LOG_INFO("Info log without topic");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.topic == lmms::LogTopic::Default(),
			"A message logged without a topic should be assigned the default topic.");

		LOG_ERR_TOPIC(Test Topic, "Error log with a topic");
		FLUSH_LOGS;
		QVERIFY2(
			lastLogLine.topic.name() == "Test Topic", "A message logged with a topic should be assigned that topic.");
	}

	// The log macros should output the basename of the file they are used from.
	void testLogFileName()
	{
		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Trace);

		LOG_INFO("Log from LoggingTest.cpp");
		FLUSH_LOGS;
		QVERIFY2(
			lastLogLine.fileName == "LoggingTest.cpp", "Log messages should include the file they originated from.");
	}

	// Logs should be properly formatted with printf-style positional arguments.
	void testLogFormatting()
	{
		lmms::LogManager::inst().setMaxVerbosity(lmms::LogVerbosity::Trace);

		LOG_INFO("Test log with %s", "a string format parameter");
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Test log with a string format parameter",
			"Log messages should be formatted with string parameters.");

		LOG_INFO("Test log with the number %.2f", 3.654);
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Test log with the number 3.65",
			"Log messages should be formatted with properly truncated float parameters.");

		// LOG_INFO("Test log with 4 parameters: %s, %i, %c, %ld", "1st", 2, '3', 100000000000LL);
		lmms::LogManager::inst().push(lmms::LogVerbosity::Info,
			"/home/jonah/Documents/lmms/lmms-source/tests/src/core/log/LoggingTest.cpp", 143, lmms::LogTopic::Default(),
			"Test log with 4 parameters: %s, %i, %c, %ld", "1st", 2, '3', 100000000000LL);
		FLUSH_LOGS;
		QVERIFY2(lastLogLine.content == "Test log with 4 parameters: 1st, 2, 3, 100000000000",
			"Log messages should be formatted with multiple properly formatted parameters.");
	}
};

QTEST_GUILESS_MAIN(LoggingTest)
#include "LoggingTest.moc"