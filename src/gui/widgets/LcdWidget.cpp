/*
 * LcdWidget.cpp - a widget for displaying numbers in LCD style
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
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



#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QFontMetrics>
#include <QtGui/QStyleOptionFrameV2>

#include "LcdWidget.h"
#include "engine.h"
#include "embed.h"
#include "gui_templates.h"
#include "MainWindow.h"




//! @todo: in C++11, we can use delegating ctors
#define DEFAULT_LCDWIDGET_INITIALIZER_LIST \
	QWidget( parent ), \
	m_label()

LcdWidget::LcdWidget( QWidget* parent, const QString& name ) :
	DEFAULT_LCDWIDGET_INITIALIZER_LIST,
	m_numDigits( 1 )
{
	initUi( name );
}




LcdWidget::LcdWidget( int numDigits, QWidget* parent, const QString& name ) :
	DEFAULT_LCDWIDGET_INITIALIZER_LIST,
	m_numDigits( numDigits )
{
	initUi( name );
}




LcdWidget::LcdWidget( int numDigits, const QString& style, QWidget* parent, const QString& name ) :
	DEFAULT_LCDWIDGET_INITIALIZER_LIST,
	m_numDigits( numDigits )
{
	initUi( name, style );
}

#undef DEFAULT_LCDWIDGET_INITIALIZER_LIST




LcdWidget::~LcdWidget()
{
	delete m_lcdPixmap;
}




void LcdWidget::setValue( int value )
{
	QString s = m_textForValue[value];
	if( s.isEmpty() )
	{
		s = QString::number( value );
		// TODO: if pad == true
		/*
		while( (int) s.length() < m_numDigits )
		{
			s = "0" + s;
		}
		*/
	}

	m_display = s;

	update();
}




void LcdWidget::paintEvent( QPaintEvent* )
{
	QPainter p( this );

	QSize cellSize( m_cellWidth, m_cellHeight );

	QRect cellRect( 0, 0, m_cellWidth, m_cellHeight );

	int margin = 1;  // QStyle::PM_DefaultFrameWidth;
	//int lcdWidth = m_cellWidth * m_numDigits + (margin*m_marginWidth)*2;

//	p.translate( width() / 2 - lcdWidth / 2, 0 ); 
	p.save();

	p.translate( margin, margin );

	// Left Margin
	p.drawPixmap( cellRect, *m_lcdPixmap, 
			QRect( QPoint( charsPerPixmap*m_cellWidth, 
				isEnabled()?0:m_cellHeight ), 
			cellSize ) );

	p.translate( m_marginWidth, 0 );

	// Padding
	for( int i=0; i < m_numDigits - m_display.length(); i++ ) 
	{
		p.drawPixmap( cellRect, *m_lcdPixmap, 
			QRect( QPoint( 10 * m_cellWidth, isEnabled()?0:m_cellHeight) , cellSize ) );
		p.translate( m_cellWidth, 0 );
	}

	// Digits
	for( int i=0; i < m_display.length(); i++ ) 
	{
		int val = m_display[i].digitValue();
		if( val < 0 ) 
		{
			if( m_display[i] == '-' )
				val = 11;
			else
				val = 10;
		}
		p.drawPixmap( cellRect, *m_lcdPixmap,
				QRect( QPoint( val*m_cellWidth, 
					isEnabled()?0:m_cellHeight ),
				cellSize ) );
		p.translate( m_cellWidth, 0 );
	}

	// Right Margin
	p.drawPixmap( QRect( 0, 0, m_marginWidth-1, m_cellHeight ), *m_lcdPixmap, 
			QRect( charsPerPixmap*m_cellWidth, isEnabled()?0:m_cellHeight,
				m_cellWidth / 2, m_cellHeight ) );


	p.restore();

	// Border
	QStyleOptionFrame opt;
	opt.initFrom( this );
	opt.state = QStyle::State_Sunken;
	opt.rect = QRect( 0, 0, m_cellWidth * m_numDigits + (margin+m_marginWidth)*2 - 1,
			m_cellHeight + (margin*2) );

	style()->drawPrimitive( QStyle::PE_Frame, &opt, &p, this );

	p.resetTransform();

	// Label
	if( !m_label.isEmpty() )
	{
		p.setFont( pointSizeF( p.font(), 6.5 ) );
		p.setPen( QColor( 64, 64, 64 ) );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2 + 1,
						height(), m_label );
		p.setPen( QColor( 255, 255, 255 ) );
		p.drawText( width() / 2 -
				p.fontMetrics().width( m_label ) / 2,
						height() - 1, m_label );
	}

}




void LcdWidget::setLabel( const QString & _txt )
{
	m_label = _txt;
	updateSize();
}




void LcdWidget::setMarginWidth( int _width )
{
	m_marginWidth = _width;

	updateSize();
}




void LcdWidget::updateSize()
{
	int margin = 1;
	if (m_label.isEmpty()) {
		setFixedSize( m_cellWidth * m_numDigits + 2*(margin+m_marginWidth),
				m_cellHeight + (2*margin) );
	}
	else {
		setFixedSize( qMax<int>(
				m_cellWidth * m_numDigits + 2*(margin+m_marginWidth),
				QFontMetrics( pointSizeF( font(), 6.5 ) ).width( m_label ) ),
				m_cellHeight + (2*margin) + 9 );
	}

	update();
}




void LcdWidget::initUi(const QString& name , const QString& style)
{
	setEnabled( true );

	setWindowTitle( name );

	// We should make a factory for these or something.
	//m_lcdPixmap = new QPixmap( embed::getIconPixmap( QString( "lcd_" + style ).toUtf8().constData() ) );
	//m_lcdPixmap = new QPixmap( embed::getIconPixmap( "lcd_19green" ) ); // TODO!!
	m_lcdPixmap = new QPixmap( embed::getIconPixmap( QString( "lcd_" + style ).toUtf8().constData() ) );

	m_cellWidth = m_lcdPixmap->size().width() / LcdWidget::charsPerPixmap;
	m_cellHeight = m_lcdPixmap->size().height() / 2;

	m_marginWidth =  m_cellWidth / 2;

	updateSize();
}



#include "moc_LcdWidget.cxx"

