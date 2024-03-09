/*
 * LoggingThread.h - declaration of the logging thread
 *
 * Copyright (c) 2020 Artur Twardowski <artur.twardowski/at/gmail/com>
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
#ifndef LOGGINGTHREAD_H
#define LOGGINGTHREAD_H

#ifdef __MINGW32__
#include <mingw.thread.h>
#else
#include <thread>
#endif

namespace lmms {

class LoggingThread
{
public:
	//! Returns the singleton instance of LoggingThread.
	static LoggingThread& inst();
	~LoggingThread();

	void setFlushInterval(unsigned int interval) { m_flushInterval = interval; }

	unsigned int flushInterval() { return m_flushInterval; }

	//! Must be called explicitly to start the logging thread.
	void initialize();

private:
	LoggingThread();
	// These are specifically un-defined so that only one instance of the singleton can exist.
	LoggingThread(const LoggingThread& t) = delete;
	void operator=(LoggingThread const&) = delete;

	//! The function which will run asynchronously on the thread.
	void run();

	unsigned int m_flushInterval;
	bool m_active;
	//! The worker thread itself.
	std::thread m_thread;
};

} // namespace lmms

#endif // LOGGINGTHREAD_H
