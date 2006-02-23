#ifndef SINGLE_SOURCE_COMPILE

/*
 * rubberband.cpp - rubberband - either own implementation for Qt3 or wrapper
 *                               for Qt4
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#include "rubberband.h"

#ifdef QT3

#include <qbitmap.h>
#include <qevent.h>
#include <qobjectlist.h>
#include <qpainter.h>

#endif



rubberBand::rubberBand( QWidget * _parent ) :
#ifndef QT3
	QRubberBand( Rectangle, _parent )
#else
	QWidget( _parent )
#endif
{
#ifdef QT3
	setBackgroundColor( QColor( 0, 64, 255 ) );
#endif
}




rubberBand::~rubberBand()
{
}




vvector<selectableObject *> rubberBand::selectedObjects( void ) const
{
	vvector<selectableObject *> so = selectableObjects();
	for( vvector<selectableObject *>::iterator it = so.begin();
							it != so.end(); )
	{
		if( ( *it )->isSelected() == FALSE )
		{
			so.erase( it );
		}
		else
		{
			++it;
		}
	}
	return( so );
}




void rubberBand::resizeEvent( QResizeEvent * _re )
{
	rubberBandBase::resizeEvent( _re );
#ifdef QT3
	updateMask();
#endif
	vvector<selectableObject *> so = selectableObjects();
	for( vvector<selectableObject *>::iterator it = so.begin();
							it != so.end(); ++it )
	{
		( *it )->setSelected( QRect( pos(), size() ).intersects(
				QRect( ( *it )->mapTo( parentWidget(),
								QPoint() ),
							( *it )->size() ) ) );
	}
}




#ifdef QT3

bool rubberBand::event( QEvent * _e )
{
	bool ret = QWidget::event( _e );
	if( isVisible() == TRUE )
	{
		raise();
	}
	return( ret );
}




void rubberBand::updateMask( void )
{
	QBitmap rb_mask( size(), TRUE );
	QPainter p( &rb_mask );
	p.setPen( Qt::color1 );
	p.drawRect( 0, 0, width() - 1, height() - 1 );
	p.end();
	setMask( rb_mask );
}

#endif




vvector<selectableObject *> rubberBand::selectableObjects( void ) const
{
	vvector<selectableObject *> so;
	if( parentWidget() == NULL )
	{
		return( so );
	}
#ifndef QT3
	QList<selectableObject *> l =
			parentWidget()->findChildren<selectableObject *>();
	for( QList<selectableObject *>::iterator it = l.begin(); it != l.end();
									++it )
	{
		so.push_back( *it );
	}
#else
	QObjectList * l = parentWidget()->queryList( "selectableObject" );
	for( QObjectListIt it = *l; it.current() != NULL; ++it )
	{
		so.push_back( static_cast<selectableObject *>( *it ) );
	}
	delete l;
#endif
	return( so );
}




#include "rubberband.moc"


#endif
