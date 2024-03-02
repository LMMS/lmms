/*
 * LoggingThread.h - declaration of the logging thread
 *
 * Copyright (c) 2020 Artur Twardowski <artur.twardowski/at/gmail/com>
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

#include <QThread>

class LoggingThread: public QThread
{
public:
	static LoggingThread& inst();
	~LoggingThread();
	void run() override;
	void setFlushInterval(unsigned int interval)
	{
		m_flushInterval = interval;
	}

	unsigned int flushInterval()
	{
		return m_flushInterval;
	}

private:
	LoggingThread();
	unsigned int m_flushInterval;
	bool m_active;
};

#endif // LOGGINGTHREAD_H
