/*
 * string_pair_drag.cpp - class stringPairDrag which provides general support
 *                        for drag'n'drop of string-pairs
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "string_pair_drag.h"



stringPairDrag::stringPairDrag( const QString & _key, const QString & _value,
					const QPixmap & _icon, QWidget * _w ) :
#ifdef QT4
				QDrag( _w )
#else
				QStoredDrag( "lmms/stringpair", _w )
#endif
{
	setPixmap( _icon );
	QString txt = _key + ":" + _value;
#ifdef QT4
	QMimeData * m = new QMimeData();
	m->setData( "lmms/stringpair", txt.toAscii() );
	setMimeData( m );
	start( /*Qt::CopyAction*/ Qt::IgnoreAction );
#else
	setEncodedData( txt.utf8() );
	drag();
#endif
}




stringPairDrag::~stringPairDrag()
{
	// TODO: do we have to delete anything???
}




void stringPairDrag::processDragEnterEvent( QDragEnterEvent * _dee,
						const QString & _allowed_keys )
{
#ifdef QT4
	if( !_dee->mimeData()->hasFormat( "lmms/stringpair" ) )
	{
		return;
	}
	QString txt = _dee->mimeData()->data();
#else
	QString txt = _dee->encodedData( "lmms/stringpair" );
#endif
	bool accepted = QStringList::split( ',', _allowed_keys ).contains(
						txt.section( ':', 0, 0 ) );
#ifdef QT4
	if( accepted )
	{
		_dee->acceptProposedAction();
	}
#else
	_dee->accept( accepted );
#endif

}




QString stringPairDrag::decodeKey( QDropEvent * _de )
{
#ifdef QT4
	return( QString( _de->mimeData()->data() ).section( ':', 0, 0 ) );
#else
	return( QString( _de->encodedData( "lmms/stringpair" ) ).section(
								':', 0, 0 ) );
#endif
}




QString stringPairDrag::decodeValue( QDropEvent * _de )
{
#ifdef QT4
	return( QString( _de->mimeData()->data() ).section( ':', 1, 1 ) );
#else
	return( QString( _de->encodedData( "lmms/stringpair" ) ).section(
								':', 1, 1 ) );
#endif
}

