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

#include <QFont>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <cfloat>
#include <cmath>
#include <string>

#include "AudioEngine.h"
#include "Engine.h"
#include "LedCheckBox.h"
#include "SamplePlayHandle.h"
#include "Song.h"
#include "embed.h"
#include "plugin_export.h"

namespace lmms {

extern "C" {
Plugin::Descriptor PLUGIN_EXPORT taptempo_plugin_descriptor
	= {LMMS_STRINGIFY(PLUGIN_NAME), "Tap Tempo", QT_TRANSLATE_NOOP("PluginBrowser", "Tap to the beat"),
		"saker <sakertooth@gmail.com>", 0x0100, Plugin::Tool, new PluginPixmapLoader("logo"), nullptr, nullptr};

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
		m_prevTime = currentTime;
	}
	else
	{
		const std::chrono::duration<double> fullInterval = currentTime - m_startTime;
		m_bpm = m_numTaps / std::max(DBL_MIN, fullInterval.count()) * 60;
	}

	if (m_numTaps > 0 && m_numTaps % s_numRecentTaps == 0)
	{
		if (m_prevTime != m_startTime)
		{
			const std::chrono::duration<double> recentInveral = currentTime - m_prevTime;
			const auto recentBpm = s_numRecentTaps / std::max(DBL_MIN, recentInveral.count()) * 60;

			if (std::abs(m_bpm - recentBpm) > s_bpmDifferenceThreshold)
			{
				m_numTaps = 0;
				m_bpm = 0;
				m_startTime = currentTime;
				m_prevTime = currentTime;
				return;
			}
		}

		m_prevTime = currentTime;
	}
	++m_numTaps;
}

PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*)
{
	return new TapTempo;
}
}

QString TapTempo::nodeName() const
{
	return taptempo_plugin_descriptor.name;
}
} // namespace lmms