/*
 * TapTempo.cpp - Plugin to count beats per minute
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
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

#include "TapTempo.h"

#include <string>

#include "SamplePlayHandle.h"
#include "Song.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms {

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT taptempo_plugin_descriptor
	= {LMMS_STRINGIFY(PLUGIN_NAME), "Tap Tempo", QT_TRANSLATE_NOOP("PluginBrowser", "Tap to the beat"),
		"saker <sakertooth@gmail.com>", 0x0100, Plugin::Type::Tool, new PluginPixmapLoader("logo"), nullptr, nullptr};

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*)
{
	return new TapTempo;
}
}

TapTempo::TapTempo()
	: ToolPlugin(&taptempo_plugin_descriptor, nullptr)
{
}

void TapTempo::tap(bool play)
{
	using namespace std::literals;

	if (play)
	{
		const auto metronomeFile = m_beat == 0 ? "misc/metronome02.ogg" : "misc/metronome01.ogg";
		Engine::audioEngine()->addPlayHandle(new SamplePlayHandle(metronomeFile));
	}

	if (m_lastTap.time_since_epoch() != 0ms)
	{
		const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_lastTap).count();
		m_intervals.emplace_back(delta);
		if (m_intervals.size() > MaxIntervals) { m_intervals.pop_front(); }

		const auto total = std::accumulate(m_intervals.begin(), m_intervals.end(), 0.0);
		const auto avg = total / m_intervals.size();
		m_bpm = 60000. / avg;
	}

	const auto timeSigNumerator = Engine::getSong()->getTimeSigModel().getNumerator();
	m_beat = (m_beat + 1) % timeSigNumerator;
	m_lastTap = clock::now();
}

void TapTempo::sync()
{
	Engine::getSong()->setTempo(std::round(m_bpm));
}

void TapTempo::reset()
{
	m_bpm = 0;
	m_beat = 0;
	m_intervals.clear();
	m_lastTap = std::chrono::time_point<clock>{};
}

QString TapTempo::nodeName() const
{
	return taptempo_plugin_descriptor.name;
}

double TapTempo::bpm() const
{
	return m_bpm;
}

} // namespace lmms