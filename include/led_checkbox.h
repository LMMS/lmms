/*
 * led_checkbox.h - class ledCheckBox, an improved QCheckBox
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


#ifndef _LED_CHECKBOX_H
#define _LED_CHECKBOX_H

#include "qt3support.h"

#ifdef QT4

#include <QWidget>

#else

#include <qwidget.h>

#endif


class QPixmap;


class ledCheckBox : public QWidget
{
	Q_OBJECT
public:
	enum ledColors
	{
		YELLOW, GREEN, TOTAL_COLORS
	} ;

	ledCheckBox( const QString & _txt, QWidget * _parent,
						ledColors _color = YELLOW );
	virtual ~ledCheckBox();


	inline bool isChecked( void ) const
	{
		return( m_checked );
	}

	inline const QString & text( void )
	{
		return( m_text );
	}


public slots:
	void toggle( void );
	void setChecked( bool _on );


protected:
	virtual void paintEvent( QPaintEvent * _pe );
	virtual void mousePressEvent( QMouseEvent * _me );


private:
	QPixmap * m_ledOnPixmap;
	QPixmap * m_ledOffPixmap;
	
	bool m_checked;
	QString m_text;
	
signals:
	void toggled( bool );

} ;

#endif
