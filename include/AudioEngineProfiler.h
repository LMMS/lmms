/*
 * AudioEngineProfiler.h - class for profiling performance of AudioEngine
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

#ifndef LMMS_AUDIO_ENGINE_PROFILER_H
#define LMMS_AUDIO_ENGINE_PROFILER_H

#include <array>
#include <atomic>
#include <QFile>

#include "LmmsTypes.h"
#include "MicroTimer.h"

namespace lmms
{

class AudioEngineProfiler
{
public:
	AudioEngineProfiler();
	~AudioEngineProfiler() = default;

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

	enum class DetailType {
		NoteSetup,
		Instruments,
		Effects,
		Mixing,
		Count
	};

	constexpr static auto DetailCount = static_cast<std::size_t>(DetailType::Count);

	int detailLoad(const DetailType type) const
	{
		return m_detailLoad[static_cast<std::size_t>(type)].load(std::memory_order_relaxed);
	}

	class Probe
	{
	public:
		Probe(AudioEngineProfiler& profiler, AudioEngineProfiler::DetailType type)
			: m_profiler(profiler)
			, m_type(type)
		{
			profiler.startDetail(type);
		}
		~Probe() { m_profiler.finishDetail(m_type); }
		Probe& operator=(const Probe&) = delete;
		Probe(const Probe&) = delete;
		Probe(Probe&&) = delete;

	private:
		AudioEngineProfiler &m_profiler;
		const AudioEngineProfiler::DetailType m_type;
	};

private:
	void startDetail(const DetailType type) { m_detailTimer[static_cast<std::size_t>(type)].reset(); }
	void finishDetail(const DetailType type)
	{
		m_detailTime[static_cast<std::size_t>(type)] = m_detailTimer[static_cast<std::size_t>(type)].elapsed();
	}

	MicroTimer m_periodTimer;
	std::atomic<float> m_cpuLoad;
	QFile m_outputFile;

	// Use arrays to avoid dynamic allocations in realtime code
	std::array<MicroTimer, DetailCount> m_detailTimer;
	std::array<int, DetailCount> m_detailTime{0};
	std::array<std::atomic<float>, DetailCount> m_detailLoad{0};
};

} // namespace lmms

#endif // LMMS_AUDIO_ENGINE_PROFILER_H
