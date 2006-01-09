/*
 * fade_button.h - declaration of class fadeButton 
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _FADE_BUTTON_H
#define _FADE_BUTTON_H 

#include "qt3support.h"

#ifdef QT4

#include <QAbstractButton>
#include <QColor>

#else

#include <qbutton.h>
#include <qcolor.h>

#endif



class fadeButton : public QAbstractButton
{
	Q_OBJECT
public:
	fadeButton( const QColor & _normal_color, const QColor &
					_activated_color, QWidget * _parent );

	virtual ~fadeButton();


public slots:
	void activate( void );
	void reset( void );


protected:
	virtual void paintEvent( QPaintEvent * _pe );


private slots:
	void nextState( void );


private:
	float m_state;
	QColor m_normalColor;
	QColor m_activatedColor;

} ;


#endif

