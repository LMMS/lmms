/*
 * lcd_spinbox.h - class lcdSpinBox, an improved QLCDNumber
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _LCD_SPINBOX_H
#define _LCD_SPINBOX_H

#include "qt3support.h"

#ifdef QT4

#include <QLCDNumber>
#include <QMap>

#else

#include <qlcdnumber.h>
#include <qmap.h>

#endif


class QLabel;


class lcdSpinBox : public QWidget
{
	Q_OBJECT
public:
	lcdSpinBox( int _min, int _max, int _num_digits, QWidget * _parent );
	virtual ~lcdSpinBox();

	void setStep( int _step );

	inline int value( void ) const
	{
		return( m_number->intValue() );
	}

	void setValue( int _value );
	void setLabel( const QString & _txt );

	inline void addTextForValue( int _val, const QString & _text )
	{
		m_textForValue[_val] = _text;
	}


public slots:
	virtual void setEnabled( bool _on );


protected:
	virtual void mousePressEvent( QMouseEvent * _me );
	virtual void mouseMoveEvent( QMouseEvent * _me );
	virtual void mouseReleaseEvent( QMouseEvent * _me );
	virtual void wheelEvent( QWheelEvent * _we );


private:
	QMap<int, QString> m_textForValue;

	int m_value;
	int m_minValue;
	int m_maxValue;
	int m_step;

	QLCDNumber * m_number;
	QLabel * m_label;

	QPoint m_origMousePos;


signals:
	void valueChanged( int );

} ;

#endif
