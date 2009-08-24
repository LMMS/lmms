/*
 * track_content_object_item.h - the base-class for TCOs on the song Editor.
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


#ifndef TRACK_CONTENT_OBJECT_ITEM_H_
#define TRACK_CONTENT_OBJECT_ITEM_H_

#include <QtCore/QTimeLine>
#include <QtCore/QVector>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsItem>
#include <QtGui/QGraphicsItemAnimation>
#include <QtGui/QPainter>
#include <math.h>

#include "lmms_basics.h"

class trackContentObject;
class TrackItem;

class TrackContentObjectItem : public QObject, public QGraphicsItem
{
	Q_OBJECT

    friend class TrackContainerScene;

public:
	TrackContentObjectItem( TrackItem * _track, trackContentObject * _object );

	virtual ~TrackContentObjectItem()
	{
		if( m_snapBackAnimation != NULL )
		{
			delete m_snapBackAnimation;
		}
	};

	QRectF boundingRect() const;

	void paint( QPainter * _painter, const QStyleOptionGraphicsItem * _option,
			QWidget * _widget );


	virtual qreal zValue() const;

	// For TrackItem to call
	void updateGeometry();

	virtual bool resizable()
	{
		return false;
	}

protected:
	virtual QVariant itemChange( GraphicsItemChange _change, const QVariant & _value );

	virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
	virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event );
	virtual void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );

	void prepareSnapBackAnimation( QTimeLine * timeLine );
	void prepareSnapBackAnimation( QTimeLine * timeLine, int newX );

protected slots:
	void updateLength();
	void updatePosition();

protected:
	QPointF m_lastPos;
	QPointF m_lastDest;
	tact_t m_length;
	QGraphicsItemAnimation * m_snapBackAnimation;
	static QTimeLine s_snapBackTimeLine;

	TrackItem * m_trackItem;
	trackContentObject * m_tco;
	bool m_hover;
};

#endif

