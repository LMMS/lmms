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

#include <cstdint>

#include "TracyProfiling.h"

namespace lmms {
namespace {

[[maybe_unused]] constexpr const char* PeriodName = "AudioEngine period";

} // namespace

void AudioEngineProfiler::startPeriod()
{
	m_periodTimer.reset();
	FrameMarkStart(PeriodName);
}

void AudioEngineProfiler::finishPeriod( sample_rate_t sampleRate, f_cnt_t framesPerPeriod )
{
	FrameMarkEnd(PeriodName);

	// Time taken to process all data and fill the audio buffer.
	const unsigned int periodElapsed = m_periodTimer.elapsed();
	// Maximum time the processing can take before causing buffer underflow. Convert to us.
	const uint64_t timeLimit = static_cast<uint64_t>(1000000) * framesPerPeriod / sampleRate;

	// Compute new overall CPU load and apply exponential averaging.
	// The result is used for overload detection in AudioEngine::criticalXRuns()
	// → the weight of a new sample must be high enough to allow relatively fast changes!
	const auto newCpuLoad = 100.f * periodElapsed / timeLimit;
	m_cpuLoad = newCpuLoad * 0.1f + m_cpuLoad * 0.9f;

	// Compute detailed load analysis. Can use stronger averaging to get more stable readout.
	for (std::size_t i = 0; i < DetailCount; i++)
	{
		const auto newLoad = 100.f * m_detailTime[i] / timeLimit;
		const auto oldLoad = m_detailLoad[i].load(std::memory_order_relaxed);
		m_detailLoad[i].store(newLoad * 0.05f + oldLoad * 0.95f, std::memory_order_relaxed);
	}

	if( m_outputFile.isOpen() )
	{
		m_outputFile.write( QString( "%1\n" ).arg( periodElapsed ).toLatin1() );
	}
}

void AudioEngineProfiler::setOutputFile( const QString& outputFile )
{
	m_outputFile.close();
	m_outputFile.setFileName( outputFile );
	m_outputFile.open( QFile::WriteOnly | QFile::Truncate );
}

AudioEngineProfiler::Probe::Probe(AudioEngineProfiler& profiler, AudioEngineProfiler::DetailType type)
	: m_profiler(profiler)
	, m_type(type)
{
	profiler.startDetail(type);

#ifdef LMMS_DEBUG_TRACY
	TracyCZone(context, true);

	switch (type)
	{
		case DetailType::NoteSetup:
			TracyCZoneName(context, "Note Setup", sizeof("Note Setup"));
			TracyCZoneColor(context, tracy::Color::SpringGreen); // #00ff7f
			break;
		case DetailType::Instruments:
			TracyCZoneName(context, "Instruments", sizeof("Instruments"));
			TracyCZoneColor(context, tracy::Color::Tomato); // #ff6347
			break;
		case DetailType::Effects:
			TracyCZoneName(context, "Effects", sizeof("Effects"));
			TracyCZoneColor(context, tracy::Color::DarkTurquoise); // #00ced1
			break;
		case DetailType::Mixing:
			TracyCZoneName(context, "Mixing", sizeof("Mixing"));
			TracyCZoneColor(context, tracy::Color::Maroon); // #b03060
			break;
		default: break;
	}

	// Store the Tracy context
	m_context.id     = context.id;
	m_context.active = context.active;
#endif
}

AudioEngineProfiler::Probe::~Probe()
{
#ifdef LMMS_DEBUG_TRACY
	// Restore the Tracy context
	auto context = TracyCZoneCtx {
		.id     = m_context.id,
		.active = m_context.active
	};

	TracyCZoneEnd(context);
#endif

	m_profiler.finishDetail(m_type);
}

} // namespace lmms
