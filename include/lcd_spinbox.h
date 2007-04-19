/*
 * lcd_spinbox.h - class lcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _LCD_SPINBOX_H
#define _LCD_SPINBOX_H

#include "qt3support.h"

#ifdef QT4

#include <QtCore/QMap>
#include <QtGui/QLCDNumber>

#else

#include <qlcdnumber.h>
#include <qmap.h>

#endif


#include "automatable_object.h"


class QLabel;


class lcdSpinBox : public QWidget, public automatableObject<int>
{
	Q_OBJECT
public:
	lcdSpinBox( int _min, int _max, int _num_digits, QWidget * _parent,
					const QString & _name, track * _track );
	virtual ~lcdSpinBox();

	virtual void setStep( const int _step );

	void setLabel( const QString & _txt );

	inline void addTextForValue( int _val, const QString & _text )
	{
		m_textForValue[_val] = _text;
	}


public slots:
	virtual void setValue( const int _value );
	virtual void setEnabled( bool _on );


protected:
	virtual void contextMenuEvent( QContextMenuEvent * _me );
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	QMap<int, QString> m_textForValue;

	QLCDNumber * m_number;
	QLabel * m_label;

	QPoint m_origMousePos;


signals:
	void valueChanged( int );
	void manualChange( void );

} ;

#endif
