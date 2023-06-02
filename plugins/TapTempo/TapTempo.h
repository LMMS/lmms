/*
 * TapTempo.h - Plugin to count beats per minute
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

#ifndef LMMS_TAP_TEMPO_H
#define LMMS_TAP_TEMPO_H

#include <chrono>

#include "TapTempoUi.h"
#include "ToolPlugin.h"
#include "ToolPluginView.h"

class QLabel;
class QPushButton;
class LedCheckBox;

namespace lmms {

namespace gui {

class TapTempoView : public ToolPluginView
{
	Q_OBJECT
public:
	using clock = std::chrono::steady_clock;

	TapTempoView(ToolPlugin*);

	void onBpmClick();

	void keyPressEvent(QKeyEvent*) override;
	void closeEvent(QCloseEvent*) override;

private:
	void updateLabels();

private:
	std::chrono::time_point<clock> m_startTime;
	std::chrono::time_point<clock> m_prevTime;
	TapTempoUi m_ui;
	int m_numTaps = 0;
	static constexpr int s_numRecentTaps = 3;
	static constexpr int s_bpmDifferenceThreshold = 30;
	double m_bpm = 0.0;
	bool m_showDecimal = false;
};
} // namespace gui

class TapTempo : public ToolPlugin
{
	Q_OBJECT
public:
	TapTempo();

	virtual gui::PluginView* instantiateView(QWidget*) { return new gui::TapTempoView(this); }

	virtual QString nodeName() const;

	void saveSettings(QDomDocument&, QDomElement&) override {}
	void loadSettings(const QDomElement&) override {}
};
} // namespace lmms

#endif // LMMS_TAP_TEMPO_H
