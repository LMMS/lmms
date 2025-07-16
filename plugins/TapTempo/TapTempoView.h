/*
 * TapTempoView.h - Plugin to count beats per minute
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

#ifndef LMMS_GUI_TAP_TEMPO_VIEW_H
#define LMMS_GUI_TAP_TEMPO_VIEW_H

#include "ToolPluginView.h"

class QPushButton;
class QLabel;

namespace lmms {
class TapTempo;
}

namespace lmms::gui {

class TapTempoView : public ToolPluginView
{
	Q_OBJECT
public:
	TapTempoView(TapTempo* plugin);
	void updateLabels();
	void keyPressEvent(QKeyEvent* event) override;
	void closeEvent(QCloseEvent*) override;

private:
	QPushButton* m_tapButton;
	QLabel* m_msLabel;
	QLabel* m_hzLabel;
	TapTempo* m_plugin;
	friend class TapTempo;
};
} // namespace lmms::gui

#endif // LMMS_GUI_TAP_TEMPO_VIEW_H
