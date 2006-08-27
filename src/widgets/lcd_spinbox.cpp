#ifndef SINGLE_SOURCE_COMPILE

/*
 * lcd_spinbox.cpp - class lcdSpinBox, an improved QLCDNumber
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>

#else

#include <qapplication.h>
#include <qcursor.h>
#include <qlabel.h>
#include <qpopupmenu.h>

#endif


#include "lcd_spinbox.h"
#include "embed.h"
#include "gui_templates.h"
#include "templates.h"


lcdSpinBox::lcdSpinBox( int _min, int _max, int _num_digits, QWidget * _parent,
					const QString & _name,
					engine * _engine, track * _track ) :
	QWidget( _parent
#ifndef QT4
			, _name.ascii()
#endif
		),
	autoObj( _engine, _track, 0, _min, _max ),
	m_label( NULL ),
	m_origMousePos()
{
	m_number = new QLCDNumber( _num_digits, this );
	m_number->setFrameShape( QFrame::Panel );
	m_number->setFrameShadow( QFrame::Sunken );
	m_number->setSegmentStyle( QLCDNumber::Flat );
#ifndef QT3
	QPalette pal;
	pal.setColor( QPalette::Light, Qt::gray );
	pal.setColor( QPalette::Mid, Qt::darkGray );
	pal.setColor( QPalette::Dark, Qt::black );
	pal.setColor( m_number->backgroundRole(), Qt::black );
	m_number->setPalette( pal );
	m_number->setAutoFillBackground( TRUE );
#endif
	setEnabled( TRUE );

	if( _track != NULL )
	{
		getAutomationPattern();
	}

	// value is automatically limited to given range
	setInitValue( 0 );
#ifdef QT4
	setAccessibleName( _name );
#endif

	m_number->setFixedSize( m_number->sizeHint() * 0.9 );
	setFixedSize( m_number->size() );
}




lcdSpinBox::~lcdSpinBox()
{
}




void lcdSpinBox::setStep( const int _step )
{
	autoObj::setStep( tMax( _step, 1 ) );
}




void lcdSpinBox::setValue( const int _value )
{
	const int prev_value = value();
	autoObj::setValue( _value );
	QString s = m_textForValue[value()];
	if( s == "" )
	{
		s = QString::number( value() );
		while( (int) s.length() < m_number->numDigits() )
		{
			s = "0" + s;
		}
	}
	m_number->display( s );

	if( prev_value != value() )
	{
		emit valueChanged( value() );
	}
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




void lcdSpinBox::setEnabled( bool _on )
{
	QColor fg( 255, 180, 0 );
	if( _on == FALSE )
	{
		fg = QColor( 160, 160, 160 );
	}
#ifdef QT4
	QPalette pal = m_number->palette();
	pal.setColor( QPalette::Background, QColor( 32, 32, 32 ) );
	pal.setColor( QPalette::Foreground, fg );
	m_number->setPalette( pal );
#else
	m_number->setPaletteBackgroundColor( QColor( 32, 32, 32 ) );
	m_number->setPaletteForegroundColor( fg );
#endif
	QWidget::setEnabled( _on );
}




void lcdSpinBox::contextMenuEvent( QContextMenuEvent * _me )
{
	m_origMousePos = _me->globalPos();

	if( nullTrack() )
	{
		QWidget::contextMenuEvent( _me );
		return;
	}

	// for the case, the user clicked right while pressing left mouse-
	// button, the context-menu appears while mouse-cursor is still hidden
	// and it isn't shown again until user does something which causes
	// an QApplication::restoreOverrideCursor()-call...
	mouseReleaseEvent( NULL );

	QMenu contextMenu( this );
#ifdef QT4
	contextMenu.setTitle( accessibleName() );
#else
	QLabel * caption = new QLabel( "<font color=white><b>" +
			QString( accessibleName() ) + "</b></font>", this );
	caption->setPaletteBackgroundColor( QColor( 0, 0, 192 ) );
	caption->setAlignment( Qt::AlignCenter );
	contextMenu.addAction( caption );
#endif
	contextMenu.addAction( embed::getIconPixmap( "automation" ),
					tr( "&Open in automation editor" ),
					getAutomationPattern(),
					SLOT( openInAutomationEditor() ) );
	contextMenu.exec( QCursor::pos() );
}




void lcdSpinBox::mousePressEvent( QMouseEvent * _me )
{
	if( _me->button() == Qt::LeftButton && _me->y() < m_number->height()  )
	{
		m_origMousePos = _me->globalPos();
		QApplication::setOverrideCursor( Qt::BlankCursor );
		m_oldValue = value();
	}
}




void lcdSpinBox::mouseMoveEvent( QMouseEvent * _me )
{
#ifdef QT4
	if( _me->buttons() & Qt::LeftButton )
#else
	if( _me->modifiers() == Qt::LeftButton )
#endif
	{
		int dy = _me->globalY() - m_origMousePos.y();
		if( dy > 1 || dy < -1 )
		{
			setInitValue( value() - dy / 2 * step() );
			emit manualChange();
			QCursor::setPos( m_origMousePos );
		}
	}
}




void lcdSpinBox::mouseReleaseEvent( QMouseEvent * _me )
{
	addJournalEntryFromOldToCurVal();

	QCursor::setPos( m_origMousePos );
	QApplication::restoreOverrideCursor();
}




void lcdSpinBox::wheelEvent( QWheelEvent * _we )
{
	_we->accept();
	setInitValue( value() + ( ( _we->delta() > 0 ) ? 1 : -1 ) * step() );
	emit manualChange();
}




#include "lcd_spinbox.moc"


#endif
