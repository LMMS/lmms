/*
 * TimeDisplayWidget.h - widget for displaying current playback time
 *
 * Copyright (c) 2014 Ruben Ibarra
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef LMMS_GUI_TIME_DISPLAY_WIDGET_H
#define LMMS_GUI_TIME_DISPLAY_WIDGET_H

#include <QWidget>
#include <QHBoxLayout>

#include "LcdWidget.h"

namespace lmms::gui
{

class TimeDisplayWidget : public QWidget
{
	Q_OBJECT
public:
	TimeDisplayWidget();
	~TimeDisplayWidget() override = default;


protected:
	void mousePressEvent( QMouseEvent* mouseEvent ) override;


private slots:
	void updateTime();


private:
	enum class DisplayMode
	{
		MinutesSeconds,
		BarsTicks
	};

	void setDisplayMode( DisplayMode displayMode );

	DisplayMode m_displayMode;
	QHBoxLayout m_spinBoxesLayout;
	LcdWidget m_majorLCD;
	LcdWidget m_minorLCD;
	LcdWidget m_milliSecondsLCD;

} ;

} // namespace lmms::gui

#endif // LMMS_GUI_TIME_DISPLAY_WIDGET_H
