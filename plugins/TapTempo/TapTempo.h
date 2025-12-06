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

#include "TapTempoView.h"
#include "ToolPlugin.h"

namespace lmms {

class TapTempo : public ToolPlugin
{
	Q_OBJECT
public:
	using clock = std::chrono::steady_clock;

	TapTempo();
	void onBpmClick();

	QString nodeName() const override;
	void saveSettings(QDomDocument&, QDomElement&) override {}
	void loadSettings(const QDomElement&) override {}

	gui::PluginView* instantiateView(QWidget*) override { return new gui::TapTempoView(this); }

private:
	std::chrono::time_point<clock> m_startTime;
	int m_numTaps = 0;
	int m_tapsNeededToDisplay = 2;
	double m_bpm = 0.0;
	bool m_showDecimal = false;

	friend class gui::TapTempoView;
};
} // namespace lmms

#endif // LMMS_TAP_TEMPO_H
