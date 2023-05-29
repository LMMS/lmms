/*
 * TapTempo.h - plugin to count beats per minute
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

#ifndef TAPTEMPO_H
#define TAPTEMPO_H

#include <chrono>

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
	TapTempoView(ToolPlugin*);

	void onBpmClick();

	void keyPressEvent(QKeyEvent*) override;
	void closeEvent(QCloseEvent*) override;

private:
	void reset();
	void updateLabels();

private:
	std::chrono::time_point<std::chrono::steady_clock> m_firstTime;
	std::chrono::time_point<std::chrono::steady_clock> m_previousTime;
	int m_numTaps;
	QPushButton* m_bpmButton;
	QLabel* m_msLabel;
	QLabel* m_hzLabel;
	double m_bpm = 0;
	bool m_showDecimal = false;
};
} // namespace gui

class TapTempo : public ToolPlugin
{
	Q_OBJECT;

public:
	TapTempo();

	virtual gui::PluginView* instantiateView(QWidget*) { return new gui::TapTempoView(this); }

	virtual QString nodeName() const;

	virtual void saveSettings(QDomDocument& doc, QDomElement& element)
	{
		Q_UNUSED(doc)
		Q_UNUSED(element)
	}

	virtual void loadSettings(const QDomElement& element) { Q_UNUSED(element) }
};
} // namespace lmms

#endif
