/*
 * TapTempo.cpp - Plugin to count beats per minute
 *
 *
 * Copyright (c) 2022 saker <sakertooth@gmail.com>
 *
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

void TapTempo::onBpmClick()
{
	const auto currentTime = clock::now();
	if (m_numTaps == 0)
	{
		m_startTime = currentTime;
	}
	else
	{
		using namespace std::chrono_literals;
		const auto secondsElapsed = (currentTime - m_startTime) / 1.0s;
		if (m_numTaps >= m_tapsNeededToDisplay) { m_bpm = m_numTaps / secondsElapsed * 60; }
	}

	++m_numTaps;
}

QString TapTempo::nodeName() const
{
	return taptempo_plugin_descriptor.name;
}
} // namespace lmms