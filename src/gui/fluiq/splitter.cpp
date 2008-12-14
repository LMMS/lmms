/*
 * splitter.cpp - implementation of FLUIQ::Splitter
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


#include <QtGui/QLayout> 
#include <QtGui/QPainter> 
#include <QtGui/QPaintEvent> 

#include "fluiq/splitter.h"


FLUIQ::Splitter::Splitter( Qt::Orientation _o, QWidget * _parent ) :
	QWidget( _parent ),
	m_orientation( _o )
{
	if( m_orientation == Qt::Horizontal )
	{
		m_mainLayout = new QHBoxLayout( this );
	}
	else
	{
		m_mainLayout = new QVBoxLayout( this );
	}
	m_mainLayout->setMargin( 0 );
	m_mainLayout->setSpacing( 0 );
	m_mainLayout->insertStretch( 100, 0 );
}




FLUIQ::Splitter::~Splitter()
{
}




void FLUIQ::Splitter::addWidget( QWidget * _w )
{
	m_mainLayout->insertWidget( m_children.size(), _w, 1 );
	m_children << _w;
}




int FLUIQ::Splitter::indexOf( QWidget * _widget ) const
{
	int i = 0;
	foreach( QWidget * w , m_children )
	{
		if( w == _widget )
		{
			return i;
		}
		++i;
	}

	return -1;
}




QWidget * FLUIQ::Splitter::widget( int _idx )
{
	return m_children[_idx];
}


#include "fluiq/moc_splitter.cxx"

