/*
 * MixerProfiler.h - class for profiling performance of Mixer
 *
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QFile>

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

	void finishPeriod( sample_rate_t sampleRate, fpp_t framesPerPeriod );

	int cpuLoad() const
	{
		return m_cpuLoad;
	}

	void setOutputFile( const QString& outputFile );


private:
	MicroTimer m_periodTimer;
	int m_cpuLoad;
	QFile m_outputFile;

};

#endif
