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

//#ifdef LMMS_DEBUG_PERFLOG

#include "PerfLog.h"

QHash< QString,PerfLog::Entry> PerfLog::s_running;

PerfLog::Entry::Entry()
{
	c=times(&t);
	if(c==-1) qFatal("PerfLogEntry: init failed");
}

void PerfLog::begin(const QString& what)
{
	if(s_running.contains(what))
		qWarning("PerfLog::begin already %s",qPrintable(what));

	s_running.insert(what,Entry());
}

void PerfLog::end(const QString& what)
{
	static long clktck = 0;
	if (!clktck)
		if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
			qFatal("PerfLog::end sysconf()");

	PerfLog::Entry e;
	PerfLog::Entry b=s_running.take(what);
	//                | task | real  | user  | syst 
	qWarning("PERFLOG | %20s | %7.2f | %7.2f | %7.2f",
		 qPrintable(what),
		 (e.c-b.c)/(double)clktck,
		 (e.t.tms_utime - b.t.tms_utime)/(double)clktck,
		 (e.t.tms_stime - b.t.tms_stime)/(double)clktck);
}

//#endif
