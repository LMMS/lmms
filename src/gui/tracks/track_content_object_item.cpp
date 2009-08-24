/*
 * track_content_object_item.cpp - the base-class for TCOs on the song Editor.
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


#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_item.h"
#include "track.h"


TrackContentObjectItem::TrackContentObjectItem(
		TrackItem * _track,
		trackContentObject * _object ) :
	QObject(),
	QGraphicsItem(),
	m_trackItem( _track ),
	m_tco( _object ),
	m_snapBackAnimation( NULL ),
	m_hover( false )
{
	setFlags( QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable );
	setAcceptHoverEvents( true );
	setCursor( Qt::OpenHandCursor );

	updateLength();
	updatePosition();

	m_snapBackAnimation = new QGraphicsItemAnimation();
	m_snapBackAnimation->setItem( this );


	connect( m_tco, SIGNAL( lengthChanged() ),
	         this, SLOT( updateLength() ), Qt::QueuedConnection );

	connect( m_tco, SIGNAL( positionChanged() ),
	         this, SLOT( updatePosition() ), Qt::QueuedConnection );

}



void TrackContentObjectItem::updateLength()
{
	// TODO: only change if different?
	prepareGeometryChange();
	m_length = m_tco->length().getTicks() / midiTime::ticksPerTact();
}



void TrackContentObjectItem::updatePosition()
{
	midiTime startPos = m_tco->startPosition();
	float x = TrackContainerScene::DEFAULT_CELL_WIDTH *
			startPos.getTicks() / midiTime::ticksPerTact();
	setPos( x, m_trackItem->y() );
	setZValue(x);
}



QRectF TrackContentObjectItem::boundingRect() const
{
	qreal penWidth = 1;
	// Must not call tco->length directly since boundingRect must
	// not change until after a call to prepareGeometryChange()
	// TODO: Calculate based on track length and an adjustable height.
	/*return QRectF( -64 - penWidth / 2, -16 - penWidth / 2,
			128 + penWidth / 2, 32 + penWidth / 2 );*/
	QRectF rc = QRectF( 0, 0, TrackContainerScene::DEFAULT_CELL_WIDTH *
			m_length,
			m_trackItem->height() );
	return rc;
}



qreal TrackContentObjectItem::zValue() const
{
	return QGraphicsItem::zValue();
}



void TrackContentObjectItem::updateGeometry()
{
	prepareGeometryChange();
}



void TrackContentObjectItem::paint( QPainter * _painter,
		const QStyleOptionGraphicsItem * _option, QWidget * _widget )
{
}



// I think this was the best from Cubic thru Quint
float easeInOutQuart(float t, float b, float c, float d) 
{
    float t_adj = 2.0f * (float)t / (float)d;
    if (t_adj < 1) return c/2*t_adj*t_adj*t_adj*t_adj + b;
    else {
        t_adj -= 2.0f;
        return -c/2 * (t_adj*t_adj*t_adj*t_adj - 2) + b;
    }
} 


QVariant TrackContentObjectItem::itemChange( GraphicsItemChange _change,
                                             const QVariant & _value )
{
	if( _change == ItemPositionChange && scene( ) )
	{
		// value is the new position
		QPointF newPos = _value.toPointF( );

		//printf("TCO %lld: itemChange (%.2f %.2f) -> ", m_tco, newPos.x(), newPos.y());

		if( newPos.x() < 0 )
		{
			newPos.setX( 0 );
		}
		/* FUCK_GIT
		if( newPos.y() < 0 )
		{
			newPos.setY( 0 );
		} */

		/* Let's just short-circuit the Y for now 
		if( fmod( newPos.y(), 32 ) != 16 )
		{
			newPos.setY( newPos.y() + 16 - ( fmod( newPos.y(), 32 ) ) );
		} */
		newPos.setY( m_trackItem->y() );
		
		float cellW = 16.0f;
		float xmod = fmod( newPos.x(), cellW);

		newPos.setX( easeInOutQuart( xmod, newPos.x() - xmod, cellW, cellW ) );

		/*
		if( fmod( newPos.x(), 16 ) != 0 )
		{
			newPos.setX( newPos.x() - ( fmod( newPos.x(), 16 ) ) );
		}

		if( newPos.x() != x() ) {
			setZValue(newPos.x());
		}*/


		//printf("(%.2f %.2f)\n", newPos.x(), newPos.y());


		return newPos;
	}
	else if( _change == QGraphicsItem::ItemSelectedChange && scene() )
	{
		bool sel = _value.toBool();
		if( sel )
		{
			setCursor( Qt::OpenHandCursor );
		}
		else
		{
			setCursor( Qt::OpenHandCursor );
		}
		update();
	}

	return QGraphicsItem::itemChange( _change, _value );
}



void TrackContentObjectItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
	QGraphicsItem::mousePressEvent( event );

	if( isSelected() )
	{
		setCursor( Qt::ClosedHandCursor );
	}
}



void TrackContentObjectItem::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
	QGraphicsItem::mouseReleaseEvent( event );

	setCursor( Qt::OpenHandCursor );

	QTimeLine * timeLine = new QTimeLine();
	//prepareSnapBackAnimation( timeLine );

	const float ppt = 16.0f;
	const bool doSnap = true;

	QList<QGraphicsItem*> selItems = scene()->selectedItems();

	if( selItems.count() <= 1 )
	{
		midiTime t = qMax( 0, (int)( x() * midiTime::ticksPerTact() / ppt ) );
		m_tco->movePosition( doSnap ? t.toNearestTact() : t );
	}
	else
	{
		// Find first item, TODO: Or maybe the item-under-mouse?
		qreal earliestX = x();
		midiTime earliestTime = m_tco->startPosition();

		for( QList<QGraphicsItem *>::iterator it = selItems.begin();
				it != selItems.end(); ++it )
		{
			
			TrackContentObjectItem * tcoItem =
				dynamic_cast<TrackContentObjectItem*>( *it );
			if( tcoItem->x() < earliestX )
			{
				earliestX = qMin( earliestX, tcoItem->x() );
				earliestTime = tcoItem->m_tco->startPosition();
			}
		}

		midiTime t = qMax( 0, (int)( earliestX * midiTime::ticksPerTact() / ppt ) );
		midiTime dt = ( doSnap ? t.toNearestTact() : t ) - earliestTime; 

		for( QList<QGraphicsItem *>::iterator it = selItems.begin();
				it != selItems.end(); ++it ) {
			
			trackContentObject * tco =
				dynamic_cast<TrackContentObjectItem*>( *it )->m_tco;
				
			tco->movePosition( tco->startPosition() + dt );
		}

	}


	/* FUCK_GIT
	if( isSelected() ) {
		QList<QGraphicsItem*> selItems = scene()->selectedItems();
		for( QList<QGraphicsItem *>::iterator it = selItems.begin();
				it != selItems.end(); ++it )
		{
			TrackContentObjectItem * tcoItem =
				dynamic_cast<TrackContentObjectItem*>( *it );
			if( tcoItem )
			{
				tcoItem->prepareSnapBackAnimation( timeLine );
			}
		}

	}

	timeLine->setCurrentTime( 0.0f );
	timeLine->setDuration( 300 );
	timeLine->setCurveShape( QTimeLine::EaseInOutCurve );
	connect( timeLine, SIGNAL(finished()), timeLine, SLOT(deleteLater()));
	timeLine->start();
	*/
} 
 


void TrackContentObjectItem::hoverEnterEvent( QGraphicsSceneHoverEvent * event )
{
	m_hover = true;
	QGraphicsItem::hoverEnterEvent( event );
	update();
}



void TrackContentObjectItem::hoverLeaveEvent( QGraphicsSceneHoverEvent * event )
{
	QGraphicsItem::hoverLeaveEvent( event );
	m_hover = false;
	update();
}




void TrackContentObjectItem::prepareSnapBackAnimation( QTimeLine * timeLine )
{
	prepareSnapBackAnimation( timeLine, x() );
}



void TrackContentObjectItem::prepareSnapBackAnimation( QTimeLine * timeLine, int newX )
{
	const qreal cellWidth = TrackContainerScene::DEFAULT_CELL_WIDTH;
	const qreal xVal = newX + TrackContainerScene::DEFAULT_CELL_WIDTH * 0.5f;

	QPointF newPos( xVal - ( fmod( xVal, cellWidth) ), y() );

	m_snapBackAnimation->setTimeLine( timeLine );
	m_snapBackAnimation->setPosAt( 0.0, pos() );
	m_snapBackAnimation->setPosAt( 1.0, newPos );
}


#include "gui/tracks/moc_track_content_object_item.cxx"
