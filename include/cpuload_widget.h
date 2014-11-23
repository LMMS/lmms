/*
 * cpuload_widget.h - widget for displaying CPU-load (partly based on
 *                    Hydrogen's CPU-load-widget)
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _CPULOAD_WIDGET_H
#define _CPULOAD_WIDGET_H

#include <QtCore/QTimer>
#include <QtGui/QPixmap>
#include <QtGui/QWidget>

#include "lmms_basics.h"


class cpuloadWidget : public QWidget
{
	Q_OBJECT
public:
	cpuloadWidget( QWidget * _parent );
	virtual ~cpuloadWidget();


protected:
	virtual void paintEvent( QPaintEvent * _ev );


protected slots:
	void updateCpuLoad();


private:
	int m_currentLoad;

	QPixmap m_temp;
	QPixmap m_background;
	QPixmap m_leds;

	bool m_changed;

	QTimer m_updateTimer;

} ;


#endif
