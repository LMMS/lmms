/*
 * lcd_spinbox.cpp - class lcdSpinBox, an improved QLCDNumber
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


#include "qt3support.h"

#ifdef QT4

#include <QApplication>
#include <QMouseEvent>
#include <QCursor>
#include <QLabel>

#else

#include <qapplication.h>
#include <qcursor.h>
#include <qlabel.h>

#endif


#include "lcd_spinbox.h"
#include "gui_templates.h"
#include "templates.h"


lcdSpinBox::lcdSpinBox( int _min, int _max, int _num_digits,
							QWidget * _parent ) :
	QWidget( _parent ),
	m_minValue( _min ),
	m_maxValue( _max ),
	m_step( 1 ),
	m_label( NULL ),
	m_origMousePos()
{
	m_number = new QLCDNumber( _num_digits, this );
	m_number->setFrameShape( QFrame::Panel );
	m_number->setFrameShadow( QFrame::Sunken );
	m_number->setSegmentStyle( QLCDNumber::Flat );
#ifdef QT4
	QPalette pal = m_number->palette();
	pal.setColor( QPalette::Background, QColor( 32, 32, 32 ) );
	pal.setColor( QPalette::Foreground, QColor( 255, 180, 0 ) );
	m_number->setPalette( pal );
#else
	m_number->setPaletteBackgroundColor( QColor( 32, 32, 32 ) );
	m_number->setPaletteForegroundColor( QColor( 255, 180, 0 ) );
#endif
	// value is automatically limited to given range
	setValue( 0 );

	m_number->setFixedSize( m_number->sizeHint() * 0.9 );
	setFixedSize( m_number->size() );
}




lcdSpinBox::~lcdSpinBox()
{
}



void lcdSpinBox::setStep( int _step )
{
	m_step = tMax( _step, 1 );
}




void lcdSpinBox::setValue( int _value )
{
	_value = ( ( tLimit( _value, m_minValue, m_maxValue ) - m_minValue ) /
						m_step ) * m_step + m_minValue;
	QString s = QString::number( _value );
	while( (int) s.length() < m_number->numDigits() )
	{
		s = "0" + s;
	}

	m_number->display( s );
}




void lcdSpinBox::setLabel( const QString & _txt )
{
	if( m_label == NULL )
	{
		m_label = new QLabel( _txt, this );
		m_label->setFont( pointSize<6>( m_label->font() ) );
		m_label->setGeometry( 0, y() + height(),
			QFontMetrics( m_label->font() ).width( _txt ), 7 );
		setFixedSize( tMax( width(), m_label->width() ),
						height() + m_label->height() );
	}
	else
	{
		m_label->setText( _txt );
	}
}




void lcdSpinBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && _me->y() < m_number->height()  )
	{
		m_origMousePos = _me->globalPos();
		QApplication::setOverrideCursor( Qt::BlankCursor );
	}
}



void lcdSpinBox::mouseMoveEvent( QMouseEvent * _me )
{
	if( _me->modifiers() == Qt::LeftButton )
	{
		int dy = _me->globalY() - m_origMousePos.y();
		if( dy > 1 || dy < -1 )
		{
			setValue( value() - dy / 2 * m_step );
			emit valueChanged( value() );
			QCursor::setPos( m_origMousePos );
		}
	}
}




void lcdSpinBox::mouseReleaseEvent( QMouseEvent * _me )
{
	QCursor::setPos( m_origMousePos );
	QApplication::restoreOverrideCursor();
}




void lcdSpinBox::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	setValue( value() + _we->delta() / 120 * m_step );
	emit valueChanged( value() );
}




#include "lcd_spinbox.moc"

