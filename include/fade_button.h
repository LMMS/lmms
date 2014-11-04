/*
 * fade_button.h - declaration of class fadeButton 
 *
 * Copyright (c) 2005-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _FADE_BUTTON_H
#define _FADE_BUTTON_H 

#include <QtCore/QTime>
#include <QtGui/QAbstractButton>
#include <QtGui/QColor>


class fadeButton : public QAbstractButton
{
	Q_OBJECT
public:
	fadeButton( const QColor & _normal_color, const QColor &
					_activated_color, QWidget * _parent );

	virtual ~fadeButton();


public slots:
	void activate();


protected:
	virtual void customEvent( QEvent * );
	virtual void paintEvent( QPaintEvent * _pe );


private:
	QTime m_stateTimer;
	QColor m_normalColor;
	QColor m_activatedColor;

	void signalUpdate();

} ;


#endif

