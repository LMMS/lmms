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

class QVBoxLayout;
class QHBoxLayout;
class QPushButton;
class QLabel;
class QCloseEvent;
class QKeyEvent;
class QCheckBox;

namespace lmms {
class TapTempo;
}

namespace lmms::gui {

class TapTempoView : public ToolPluginView
{
	Q_OBJECT
public:
	TapTempoView(ToolPlugin*);
	void closeEvent(QCloseEvent*) override;
	void keyPressEvent(QKeyEvent*) override;
	void updateLabels();

private:
	QVBoxLayout* m_windowLayout;
	QVBoxLayout* m_mainLayout;
	QPushButton* m_tapButton;
	QHBoxLayout* m_sidebarLayout;
	QLabel* m_msLabel;
	QLabel* m_hzLabel;
	QVBoxLayout* m_verticalLayout;
	QCheckBox* m_precisionCheckBox;
	QCheckBox* m_muteCheckBox;
	TapTempo* m_plugin;
	friend class TapTempo;
};
} // namespace lmms::gui

#endif