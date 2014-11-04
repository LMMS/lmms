/*
 * string_pair_drag.h - class stringPairDrag which provides general support
 *                      for drag'n'drop of string-pairs
 *
 * Copyright (c) 2005-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of LMMS - http://lmms.io
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

#ifndef _STRING_PAIR_DRAG_H
#define _STRING_PAIR_DRAG_H

#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>

#include "export.h"

class QPixmap;


class EXPORT stringPairDrag : public QDrag
{
public:
	stringPairDrag( const QString & _key, const QString & _value,
					const QPixmap & _icon, QWidget * _w );
	~stringPairDrag();

	static bool processDragEnterEvent( QDragEnterEvent * _dee,
						const QString & _allowed_keys );
	static QString decodeKey( QDropEvent * _de );
	static QString decodeValue( QDropEvent * _de );

	static const char * mimeType()
	{
		return( "application/x-lmms-stringpair" );
	}

} ;


#endif
