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


#include <QtGui/QPainter> 
#include <QtGui/QPaintEvent> 

#include "fluiq/splitter.h"


FLUIQ::Splitter::Splitter( Qt::Orientation _o, QWidget * _parent ) :
	QSplitter( _o, _parent )
{
	setChildrenCollapsible( false );
	setSizePolicy( QSizePolicy::MinimumExpanding,
					QSizePolicy::MinimumExpanding );
}




FLUIQ::Splitter::~Splitter()
{
}




QSplitterHandle * FLUIQ::Splitter::createHandle( void )
{
	return new FLUIQ::SplitterHandle( orientation(), this );
}





FLUIQ::SplitterHandle::SplitterHandle( Qt::Orientation _o,
							QSplitter * _parent ) :
	QSplitterHandle( _o, _parent )
{
}




FLUIQ::SplitterHandle::~SplitterHandle()
{
}




void FLUIQ::SplitterHandle::paintEvent( QPaintEvent * _event )
{
	QPainter painter( this );

	QLinearGradient gradient;
	gradient.setColorAt( 0, QColor( 128, 128, 128 ) );
	gradient.setColorAt( 1, QColor( 32, 32, 32 ) );
	if( orientation() == Qt::Horizontal )
	{
		gradient.setStart( rect().left(), rect().height()/2 );
		gradient.setFinalStop( rect().right(), rect().height()/2 );
	}
	else
	{
		gradient.setStart( rect().width()/2, rect().top() );
		gradient.setFinalStop( rect().width()/2, rect().bottom() );
	}
	painter.fillRect( _event->rect(), QBrush( gradient ) );
}



#include "fluiq/moc_splitter.cxx"

