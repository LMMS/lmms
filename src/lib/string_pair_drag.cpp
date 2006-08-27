#ifndef SINGLE_SOURCE_COMPILE

/*
 * string_pair_drag.cpp - class stringPairDrag which provides general support
 *                        for drag'n'drop of string-pairs and which is the base
 *                        for all drag'n'drop-actions within LMMS
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "string_pair_drag.h"
#include "main_window.h"

#ifdef QT4

#include <QtCore/QMimeData>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>

#endif


stringPairDrag::stringPairDrag( const QString & _key, const QString & _value,
				const QPixmap & _icon, QWidget * _w,
							engine * _engine ) :
#ifdef QT4
				QDrag( _w ),
#else
				QStoredDrag( "lmms/stringpair", _w ),
#endif
				engineObject( _engine )
{
	setPixmap( _icon );
	QString txt = _key + ":" + _value;
#ifdef QT4
	QMimeData * m = new QMimeData();
	m->setData( "lmms/stringpair", txt.toAscii() );
	setMimeData( m );
	start( Qt::IgnoreAction );
#else
	setEncodedData( txt.utf8() );
	drag( QDragObject::DragDefault );
#endif
}




stringPairDrag::~stringPairDrag()
{
	// during a drag, we might have lost key-press-events, so reset
	// modifiers of main-win
	eng()->getMainWindow()->clearKeyModifiers();
	// TODO: do we have to delete anything???
}




bool stringPairDrag::processDragEnterEvent( QDragEnterEvent * _dee,
						const QString & _allowed_keys )
{
#ifdef QT4
	if( !_dee->mimeData()->hasFormat( "lmms/stringpair" ) )
	{
		return( FALSE );
	}
	QString txt = _dee->mimeData()->data( "lmms/stringpair" );
	if( _allowed_keys.split( ',' ).contains( txt.section( ':', 0, 0 ) ) )
	{
		_dee->acceptProposedAction();
		return( TRUE );
	}
	_dee->ignore();
	return( FALSE );
#else
	QString txt = _dee->encodedData( "lmms/stringpair" );
	bool accepted = QStringList::split( ',', _allowed_keys ).contains(
						txt.section( ':', 0, 0 ) );
	_dee->accept( accepted );
	return( accepted );
#endif
}




QString stringPairDrag::decodeKey( QDropEvent * _de )
{
#ifdef QT4
	return( QString( _de->mimeData()->data( "lmms/stringpair"
						) ).section( ':', 0, 0 ) );
#else
	return( QString( _de->encodedData( "lmms/stringpair" ) ).section(
								':', 0, 0 ) );
#endif
}




QString stringPairDrag::decodeValue( QDropEvent * _de )
{
#ifdef QT4
	return( QString( _de->mimeData()->data( "lmms/stringpair"
						) ).section( ':', 1, 1 ) );
#else
	return( QString( _de->encodedData( "lmms/stringpair" ) ).section(
								':', 1, 1 ) );
#endif
}


#endif
