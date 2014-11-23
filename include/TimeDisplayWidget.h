/*
 * TimeDisplayWidget.h - widget for displaying current playback time
 *
 * Copyright (c) 2014 Ruben Ibarra
 * Copyright (c) 2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#ifndef _TIME_DISPLAY_WIDGET
#define _TIME_DISPLAY_WIDGET

#include <QtGui/QWidget>
#include <QtGui/QHBoxLayout>

#include "LcdWidget.h"


class TimeDisplayWidget : public QWidget
{
	Q_OBJECT
public:
	TimeDisplayWidget();
	virtual ~TimeDisplayWidget();


protected:
	virtual void mousePressEvent( QMouseEvent* mouseEvent );


private slots:
	void updateTime();


private:
	enum DisplayModes
	{
		MinutesSeconds,
		BarsTicks,
		DisplayModeCount
	};
	typedef DisplayModes DisplayMode;

	void setDisplayMode( DisplayMode displayMode );

	DisplayMode m_displayMode;
	QHBoxLayout m_spinBoxesLayout;
	LcdWidget m_majorLCD;
	LcdWidget m_minorLCD;
	LcdWidget m_milliSecondsLCD;

} ;

#endif
