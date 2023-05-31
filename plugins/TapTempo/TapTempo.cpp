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
		"sakertooth <sakertooth@gmail.com>", 0x0100, Plugin::Tool, new PluginPixmapLoader("logo"), nullptr, nullptr};

// neccessary for getting instance out of shared lib
PLUGIN_EXPORT Plugin* lmms_plugin_main(Model*, void*)
{
	return new TapTempo;
}
}

TapTempo::TapTempo()
	: ToolPlugin(&taptempo_plugin_descriptor, nullptr)
{
}

QString TapTempo::nodeName() const
{
	return taptempo_plugin_descriptor.name;
}

namespace gui {
TapTempoView::TapTempoView(ToolPlugin* _tool)
	: ToolPluginView(_tool)
	, m_numTaps(0)
{
	m_ui.setupUi(this);
	connect(m_ui.precisionCheckBox, &LedCheckBox::toggled, [this](bool checked) {
		m_showDecimal = checked;
		updateLabels();
	});

	connect(m_ui.tapButton, &QPushButton::pressed, this, &TapTempoView::onBpmClick);
	updateLabels();

	if (parentWidget())
	{
		parentWidget()->hide();
		parentWidget()->layout()->setSizeConstraint(QLayout::SetFixedSize);
	}
}

void TapTempoView::onBpmClick()
{
	if (!m_ui.muteCheckBox->isChecked())
	{
		const auto timeSigNumerator = Engine::getSong()->getTimeSigModel().getNumerator();
		m_numTaps % timeSigNumerator == 0
			? Engine::audioEngine()->addPlayHandle(new SamplePlayHandle("misc/metronome02.ogg"))
			: Engine::audioEngine()->addPlayHandle(new SamplePlayHandle("misc/metronome01.ogg"));
	}

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
		updateLabels();
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
				updateLabels();
				return;
			}
		}

		m_prevTime = currentTime;
	}

	++m_numTaps;
}

void TapTempoView::updateLabels()
{
	const double bpm = m_showDecimal ? m_bpm : std::round(m_bpm);
	const double hz = bpm / 60;
	const double ms = bpm > 0 ? 1 / hz * 1000 : 0;

	m_ui.tapButton->setText(QString::number(bpm, 'f', m_showDecimal ? 1 : 0));
	m_ui.msLabel->setText(tr("%1 ms").arg(ms, 0, 'f', m_showDecimal ? 1 : 0));
	m_ui.hzLabel->setText(tr("%1 Hz").arg(hz, 0, 'f', 4));
}

void TapTempoView::keyPressEvent(QKeyEvent* event)
{
	QWidget::keyPressEvent(event);
	if (!event->isAutoRepeat()) { onBpmClick(); }
}

void TapTempoView::closeEvent(QCloseEvent* event)
{
	m_numTaps = 0;
	m_bpm = 0;
	updateLabels();
}
} // namespace gui
} // namespace lmms
