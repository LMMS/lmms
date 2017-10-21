/*
 * PerfLog.h - Small performance logger
 *
 * Copyright (c) 2017 gi0e5b06
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

#ifndef PERFLOG_H
#define PERFLOG_H

#include <unistd.h>
#include <sys/times.h>

#include <QHash>
#include <QString>

class PerfLog
{
 public:
	static void begin(const QString& what);
	static void end(const QString& what);

 private:
	class Entry
	{
	public:
		clock_t c;
		tms     t;
		Entry();
	};

	static QHash< QString,PerfLog::Entry> s_running;
};

#ifndef LMMS_DEBUG_PERFLOG

#define PL_BEGIN(w)
#define PL_END(w)

#else

#define PL_BEGIN(w) PerfLog::begin(w);
#define PL_END(w) PerfLog::end(w);

#endif

#endif
