/*
 * string_pair_drag.h - class stringPairDrag which provides general support
 *                      for drag'n'drop of string-pairs
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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

#ifndef _STRING_PAIR_DRAG_H
#define _STRING_PAIR_DRAG_H

#include "qt3support.h"

#ifdef QT4

#include <QDrag>

#else

#include <qdragobject.h>

#endif


class QPixmap;


class stringPairDrag : public
#ifdef QT4
				QDrag
#else
				QStoredDrag
#endif
{
public:
	stringPairDrag( const QString & _key, const QString & _value,
					const QPixmap & _icon, QWidget * _w );
	~stringPairDrag();

	static void processDragEnterEvent( QDragEnterEvent * _dee,
						const QString & _allowed_keys );
	static QString decodeKey( QDropEvent * _de );
	static QString decodeValue( QDropEvent * _de );

} ;


#endif
