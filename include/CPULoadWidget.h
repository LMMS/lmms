/*
 * CPULoadWidget.h - widget for displaying CPU-load (partly based on
 *                    Hydrogen's CPU-load-widget)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_CPU_LOAD_WIDGET_H
#define LMMS_GUI_CPU_LOAD_WIDGET_H

#include <algorithm>
#include <QTimer>
#include <QPixmap>
#include <QWidget>

#include "LmmsTypes.h"


namespace lmms::gui
{


class CPULoadWidget : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(int stepSize MEMBER m_stepSize)
public:
	CPULoadWidget( QWidget * _parent );
	~CPULoadWidget() override = default;


protected:
	void paintEvent( QPaintEvent * _ev ) override;


protected slots:
	void updateCpuLoad();


private:
	int stepSize() const { return std::max(1, m_stepSize); }

	int m_currentLoad;

	QPixmap m_temp;
	QPixmap m_background;
	QPixmap m_leds;

	bool m_changed;

	QTimer m_updateTimer;

	int m_stepSize = 1;

} ;


} // namespace lmms::gui

#endif // LMMS_GUI_CPU_LOAD_WIDGET_H
