/*
 * AudioEngineProfiler.cpp - class for profiling performance of AudioEngine
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

#include "AudioEngineProfiler.h"

#include <algorithm>
#include <cstdint>

namespace lmms
{

AudioEngineProfiler::AudioEngineProfiler() :
	m_periodTimer(),
	m_cpuLoad(0),
	m_outputFile()
{
}



void AudioEngineProfiler::finishPeriod(sample_rate_t sampleRate, fpp_t framesPerPeriod)
{
	// Time taken to process all data and fill the audio buffer.
	const unsigned int periodElapsed = m_periodTimer.elapsed();
	// Maximum time the processing can take before causing buffer underflow. Convert to us.
	const uint64_t timeLimit = static_cast<uint64_t>(1000000) * framesPerPeriod / sampleRate;

	// Compute new overall CPU load and apply exponential averaging.
	// The result is used for overload detection in AudioEngine::CriticalXRuns()
	// → the weight of a new sample must be high enough to allow relatively fast changes!
	const int newCpuLoad = 100 * periodElapsed / timeLimit;
	m_cpuLoad = std::min(newCpuLoad * 0.1f + m_cpuLoad * 0.9f, 100.f);

	// Compute detailed load analysis. Can use stronger averaging to get more stable readout.
	for (int i = 0; i < DetailCount; i++)
	{
		const int newLoad = 100 * m_detailTime[i] / timeLimit;
		m_detailLoad[i] = std::min(newLoad * 0.05f + m_detailLoad[i] * 0.95f, 100.f);
	}

	if (m_outputFile.isOpen())
	{
		m_outputFile.write(QString("%1\n").arg(periodElapsed).toLatin1());
	}
}



void AudioEngineProfiler::setOutputFile(const QString &outputFile)
{
	m_outputFile.close();
	m_outputFile.setFileName(outputFile);
	m_outputFile.open(QFile::WriteOnly | QFile::Truncate);
}

} // namespace lmms
