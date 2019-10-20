/*
 * ProgressBar.h - widget for displaying CPU-load or volume in the top bar
 *                    (partly based on Hydrogen's CPU-load-widget)
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


#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <QtCore/QTimer>
#include <QPixmap>
#include <QWidget>

#include "lmms_basics.h"


class ProgressBar : public QWidget
{
	Q_OBJECT
public:
	ProgressBar( QWidget * _parent, const QPixmap & background, const QPixmap & leds );
	virtual ~ProgressBar();
	
	void setValue( float value );
	float getValue() const;

protected:
	virtual void paintEvent( QPaintEvent * _ev );

private:
	float m_value;

	QPixmap m_temp;
	QPixmap m_background;
	QPixmap m_leds;

	bool m_changed;

} ;


#endif
