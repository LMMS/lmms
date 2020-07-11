/*
 * IntegerDisplayWidget.cpp - widget displaying numbers in modernized LCD style
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
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
#include <QStyle>
#include <QVBoxLayout>

#include "IntegerDisplayWidget.h"




IntegerDisplayWidget::IntegerDisplayWidget( QWidget* parent, const QString& name ) :
	QLabel( parent ),
	m_numDigits( 1 )
{
	initUi( name );
}




IntegerDisplayWidget::IntegerDisplayWidget( int numDigits, QWidget* parent, const QString& name ) :
	QLabel( parent ),
	m_numDigits( numDigits )
{
	initUi( name );
}




IntegerDisplayWidget::~IntegerDisplayWidget()
{
}




void IntegerDisplayWidget::setValue( int value )
{
	m_value = value;
	
	QString s = m_textForValue[value];
	if (s.isEmpty())
	{
		s = m_zeroesVisible ?
				QString( "%1" ).arg( value, m_numDigits, 10, QChar( '0' ) ) :
				QString::number( value );
	}
	
	if ((m_forceSign == true) && (value >= 0))
	{
		s = "+" + s;
	}
	
	setText( s );

	update();
}




void IntegerDisplayWidget::initUi( const QString& name )
{
	setEnabled( true );

	setWindowTitle( name );
	setObjectName( name );
	
	setAlignment( Qt::AlignHCenter | Qt::AlignTop );
	
	setStyle( QApplication::style() );
	
	setValue( 0 );
	
	ensurePolished();
	const QString fixedWidthString = QString( "%1" ).arg( 0, m_numDigits, 10, QChar( '0' ) );
	int characterWidth = fontMetrics().boundingRect( fixedWidthString ).width();
	setFixedWidth(characterWidth + 1);
}





