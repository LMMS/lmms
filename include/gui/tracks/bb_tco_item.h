/*
 * bb_tco_item.h - Beat & Bassline QGraphicsItem used in the song editor
 *
 * Copyright (c) 2009 Paul Giblock <pgib/at/users.sourceforge.net>
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


#ifndef _BB_TCO_ITEM_H_
#define _BB_TCO_ITEM_H_

#include <QtCore/QTimeLine>
#include <QtCore/QVector>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsItem>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QPainter>
#include <math.h>

class trackContentObject;
class TrackItem;

#include "gui/tracks/track_content_object_item.h"

class BbTrackContentObjectItem : public TrackContentObjectItem
{
	Q_OBJECT

public:
	BbTrackContentObjectItem( TrackItem * _track, trackContentObject * _object );

	virtual void paint( QPainter * _painter, const QStyleOptionGraphicsItem * _option,
			QWidget * _widget );


protected:
	virtual QVariant itemChange( GraphicsItemChange _change, const QVariant & _value );

	virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
	virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event );
	
};

#endif
