/*
 * ToolButtonList.cpp - horizontal list of tightly packed toolbar buttons
 *
 * Copyright (c) 2019 Lathigos <lathigos/at/tutanota.com>
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


#include "ToolButtonList.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QFrame>

#include "ToolTip.h"


ToolButtonList::ToolButtonList( const QString & customName, QWidget * _parent ) :
				QWidget( _parent )
{
	m_layout = new QHBoxLayout( this );
	m_layout->setSpacing( 5 );
	m_layout->setContentsMargins( 0, 0, 0, 0 );
	setObjectName( customName );
	
	addSeparator();
}




QToolButton * ToolButtonList::addToolButton( const QPixmap & _pixmap,
				const QString & _tooltip,
				QObject * _receiver,
				const char * _slot )
{
	QToolButton * button = new QToolButton( this );
	
	if (_receiver != NULL && _slot != NULL)
	{
		connect( button, SIGNAL( clicked() ), _receiver, _slot );
	}
	ToolTip::add( button, _tooltip );
	button->setIcon( _pixmap );
	button->setObjectName( startNewRegion ? "single" : "right" );
	
	if (previousButton != nullptr)
	{
		bool isPreviousSingle = previousButton->objectName() == "single";
		previousButton->setObjectName( isPreviousSingle ? "left" : "center" );
	}
	
	m_currentFrameLayout->addWidget( button );
	previousButton = button;
	
	startNewRegion = false;
	
	return button;
}




void ToolButtonList::addSeparator()
{
	startNewRegion = true;
	previousButton = nullptr;
	
	m_currentFrame = new QFrame( this );
	m_layout->addWidget( m_currentFrame );
	m_currentFrame->setObjectName( "buttonListFrame" );
	m_currentFrame->setSizePolicy( QSizePolicy::Fixed,
					QSizePolicy::Fixed );
	
	m_currentFrameLayout = new QHBoxLayout( m_currentFrame );
	m_currentFrameLayout->setSpacing( 1 );
	m_currentFrameLayout->setContentsMargins( 0, 0, 0, 0 );
	
	m_layout->addWidget( m_currentFrame );
}




void ToolButtonList::resizeEvent( QResizeEvent * event )
{
	emit resized( event );
}



