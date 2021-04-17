/*
 * MixerProfiler.h - class for profiling performance of Mixer
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MIXER_PROFILER_H
#define MIXER_PROFILER_H

#include <array>
#include <QFile>

#include "lmms_basics.h"
#include "MicroTimer.h"

class MixerProfiler
{
public:
	MixerProfiler();
	~MixerProfiler();

	void startPeriod()
	{
		m_periodTimer.reset();
	}

	void finishPeriod(sample_rate_t sampleRate, fpp_t framesPerPeriod);

	int cpuLoad() const
	{
		return m_cpuLoad;
	}

	void setOutputFile(const QString& outputFile);

	enum DetailType {
		NoteSetup,
		Instruments,
		Effects,
		Mixing,
		DetailCount
	};

	void startDetail(const DetailType type) { m_detailTimer[type].reset(); }
	void finishDetail(const DetailType type) { m_detailTime[type] = m_detailTimer[type].elapsed(); }

	int detailLoad(const DetailType type) const { return m_detailLoad[type]; }

private:
	MicroTimer m_periodTimer;
	float m_cpuLoad;
	QFile m_outputFile;

	std::array<MicroTimer, DetailCount> m_detailTimer;   // use arrays to avoid dynamic allocations in realtime code
#ifdef __GNUC__
	std::array<int, DetailCount> m_detailTime{{0}};      // Workaround for https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65815
	std::array<float, DetailCount> m_detailLoad{{0}};    // (Remove when LMMS CI updates to GCC 6 or higher.)
#else
	std::array<int, DetailCount> m_detailTime = {0};
	std::array<float, DetailCount> m_detailLoad = {0};
#endif
};

#endif
