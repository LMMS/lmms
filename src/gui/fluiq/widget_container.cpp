/*
 * widget_container.cpp - implementation of FLUIQ::WidgetContainer
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

 
#include <QtGui/QVBoxLayout>
#include <QtGui/QScrollArea>

#include "fluiq/widget_container.h"
#include "fluiq/splitter.h"



FLUIQ::WidgetContainer::WidgetContainer( Qt::Orientation _o,
							Widget * _parent ) :
	Widget( _parent ),
	m_orientation( _o ),
	m_scrollArea( NULL ),
	m_splitter( NULL )
{
	QVBoxLayout * myLayout = new QVBoxLayout( this );
	myLayout->setSpacing( 0 );
	myLayout->setMargin( 0 );

	m_scrollArea = new QScrollArea( this );
	m_scrollArea->setWidgetResizable( true );
	m_scrollArea->setFrameStyle( QFrame::NoFrame );

	myLayout->addWidget( m_scrollArea );

	m_splitter = new Splitter( m_orientation );
	m_scrollArea->setWidget( m_splitter );
}




FLUIQ::WidgetContainer::~WidgetContainer()
{
}




void FLUIQ::WidgetContainer::addWidget( QWidget * _widget )
{
	m_splitter->addWidget( _widget );
}



#include "fluiq/moc_widget_container.cxx"

