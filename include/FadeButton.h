/*
 * FadeButton.h - declaration of class fadeButton
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef FADE_BUTTON_H
#define FADE_BUTTON_H

#include <QtCore/QTime>
#include <QAbstractButton>
#include <QColor>


class FadeButton : public QAbstractButton
{
	Q_OBJECT
public:
	FadeButton( const QColor & _normal_color,
		const QColor & _activated_color,
		const QColor & _hold_color,
		QWidget * _parent );

	virtual ~FadeButton();
	void setActiveColor( const QColor & activated_color );


public slots:
	void activate();
	void noteEnd();


protected:
	void paintEvent( QPaintEvent * _pe ) override;


private:
	QTime m_stateTimer;
	QTime m_releaseTimer;

	// the default color of the widget
	QColor m_normalColor;
	// the color on note play
	QColor m_activatedColor;
	// the color after the "play" fade is done but a note is still playing
	QColor m_holdColor;
	int activeNotes;

	QColor fadeToColor(QColor, QColor, QTime, float);

} ;


#endif
