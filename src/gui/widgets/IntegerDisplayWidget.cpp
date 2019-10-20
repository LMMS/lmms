/*
 * IntegerDisplayWidget.cpp - widget displaying numbers in modernized LCD style
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
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionFrameV2>
#include <QVBoxLayout>

#include "IntegerDisplayWidget.h"
#include "embed.h"
#include "gui_templates.h"
#include "MainWindow.h"




IntegerDisplayWidget::IntegerDisplayWidget( QWidget* parent, const QString& name ) :
	QWidget( parent ),
	m_numDigits( 1 )
{
	initUi( name );
}




IntegerDisplayWidget::IntegerDisplayWidget( int numDigits, QWidget* parent, const QString& name ) :
	QWidget( parent ),
	m_numDigits( numDigits )
{
	initUi( name );
}




IntegerDisplayWidget::IntegerDisplayWidget( int numDigits, const QString& style, QWidget* parent, const QString& name ) :
	QWidget( parent ),
	m_numDigits( numDigits )
{
	initUi( name, style );
}




IntegerDisplayWidget::~IntegerDisplayWidget()
{
}




void IntegerDisplayWidget::setValue( int value )
{
	QString s = m_textForValue[value];
	if( s.isEmpty() )
	{
		s = QString::number( value );
	}

	m_display = s;
	
	for ( int i=0; i < m_numDigits; i++ )
	{
		QString text = "0";
		if(i < s.size()) { text = m_display.at( s.size() - i - 1 ); }
		m_digitsList.at( m_numDigits - i - 1 )->setText( text );
	}

	update();
}




void IntegerDisplayWidget::initUi(const QString& name , const QString& style)
{
	setEnabled( true );

	setWindowTitle( name );
	
	// Create a layout with one widget per digit. This way, the digits won't
	// move around when they change value and the font isn't monospace:
	QHBoxLayout * digitsLayout = new QHBoxLayout( this );
	digitsLayout->setSpacing( 0 );
	digitsLayout->setMargin( 2 );
	
	digitsLayout->addStretch();
	
	for ( int i=0; i < m_numDigits; i++ )
	{
		QLabel * currentDigit = new QLabel( "0", this );
		currentDigit->setObjectName( "integerDisplayDigits" );
		currentDigit->setAttribute(Qt::WA_TransparentForMouseEvents);
		digitsLayout->addWidget(currentDigit);
		m_digitsList.push_back(currentDigit);
	}
	
	digitsLayout->addStretch();
	
	setLayout( digitsLayout );
}





