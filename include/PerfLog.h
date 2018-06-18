/*
 * PerfLog.h - Small performance logger
 *
 * Copyright (c) 2017-2018 LMMS Developers
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

#include <ctime>
#include <QtCore/QString>

/// \brief CPU time point
///
/// Represents a point in CPU time (not wall-clock time) intended for measuring
/// performance.
class PerfTime
{
public:
	PerfTime();
	bool valid() const;

	clock_t real() const;
	clock_t user() const;
	clock_t system() const;

	static PerfTime now();
	static clock_t ticksPerSecond();

	friend PerfTime operator-(const PerfTime& lhs, const PerfTime& rhs);
private:
	clock_t m_real;
	clock_t m_user;
	clock_t m_system;
};

/// \brief The PerfLog class
///
/// Measures time between construction and destruction and prints the result to
/// stderr, along with \p name. Alternatively, call begin() and end() explicitly.
class PerfLogTimer
{
 public:
	PerfLogTimer(const QString& name);
	~PerfLogTimer();

	void begin();
	void end();

 private:
	QString name;
	PerfTime begin_time;
};

#endif
