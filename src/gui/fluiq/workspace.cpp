/*
 * workspace.cpp - implementation of FLUIQ::Workspace
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
 

#include <QtGui/QBoxLayout>
#include <QtGui/QWidget>

#include "fluiq/workspace.h"


FLUIQ::Workspace::Workspace( QWidget * _parent ) :
	Widget( _parent ),
	m_masterLayout( new QVBoxLayout( this ) )
{
	m_masterLayout->setMargin( 0 );
	m_masterLayout->setSpacing( 0 );

	m_currentSplitter = new Splitter( Qt::Horizontal, this );
	m_masterLayout->addWidget( m_currentSplitter );
}




FLUIQ::Workspace::~Workspace()
{
}




void FLUIQ::Workspace::addWidget( QWidget * _w )
{
	m_currentSplitter = new Splitter( Qt::Horizontal, this );
	m_masterLayout->addWidget( m_currentSplitter );

	m_currentSplitter->addWidget( _w );
}




void FLUIQ::Workspace::addWidgetToExistingRow( QWidget * _w )
{
	m_currentSplitter->addWidget( _w );
}




#include "fluiq/moc_workspace.cxx"

