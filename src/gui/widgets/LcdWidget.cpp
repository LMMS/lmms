/*
 * LcdWidget.cpp - a widget for displaying numbers in LCD style
 *
 * Copyright (c) 2005-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2008 Paul Giblock <pgllama/at/gmail.com>
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



#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFrameV2>

#include "LcdWidget.h"
#include "DeprecationHelper.h"
#include "embed.h"
#include "gui_templates.h"
#include "MainWindow.h"




LcdWidget::LcdWidget( QWidget* parent, const QString& name ) :
	LcdWidget( 1, parent, name )
{
}




LcdWidget::LcdWidget( int numDigits, QWidget* parent, const QString& name ) :
	LcdWidget( numDigits, QString("19green"), parent, name )
{
}




LcdWidget::LcdWidget( int numDigits, const QString& style, QWidget* parent, const QString& name ) :
	QWidget( parent ),
	m_label(),
	m_textColor( 255, 255, 255 ),
	m_textShadowColor( 64, 64, 64 ),
	m_numDigits( numDigits )
{
	initUi( name, style );
}




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




QColor LcdWidget::textColor() const
{
	return m_textColor;
}

void LcdWidget::setTextColor( const QColor & c )
{
	m_textColor = c;
}




QColor LcdWidget::textShadowColor() const
{
	return m_textShadowColor;
}

void LcdWidget::setTextShadowColor( const QColor & c )
{
	m_textShadowColor = c;
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
		p.setPen( textShadowColor() );
		p.drawText(width() / 2 -
				horizontalAdvance(p.fontMetrics(), m_label) / 2 + 1,
						height(), m_label);
		p.setPen( textColor() );
		p.drawText(width() / 2 -
				horizontalAdvance(p.fontMetrics(), m_label) / 2,
						height() - 1, m_label);
	}

}




void LcdWidget::setLabel( const QString& label )
{
	m_label = label;
	updateSize();
}




void LcdWidget::setMarginWidth( int width )
{
	m_marginWidth = width;

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
		setFixedSize(qMax<int>(
				m_cellWidth * m_numDigits + 2*(margin+m_marginWidth),
				horizontalAdvance(QFontMetrics(pointSizeF(font(), 6.5)), m_label)),
				m_cellHeight + (2*margin) + 9);
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





