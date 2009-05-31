/*
 * track_container_scene.cpp - A TrackContainer is represented in the
 * SongEditor as the GraphicsScene.  This is the implementation.
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


#include <QtGui/QKeyEvent>

#include <stdio.h>

#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_item.h"
#include "track_container.h"

TrackContainerScene::TrackContainerScene( QObject * parent, trackContainer * _tc ) :
	QGraphicsScene( parent ),
	m_trackContainer( _tc ),
	m_ppt( 16 )
{
	connect( m_trackContainer, SIGNAL( trackAdded( track * ) ),
	         this, SLOT( addTrack( track * ) ),
	         Qt::QueuedConnection );

	connect( m_trackContainer, SIGNAL( trackRemoved( track * ) ),
	         this, SLOT( removeTrack( track * ) ) );
}


TrackContainerScene::~TrackContainerScene()
{

}



void TrackContainerScene::setPixelsPerTact( float _ppt )
{
	m_ppt = _ppt;
}



void TrackContainerScene::addTrack( track * _t )
{
	TrackItem * item = new TrackItem( this, _t );
	item->setHeight( DEFAULT_CELL_HEIGHT );
	item->setY( m_trackItems.size() * DEFAULT_CELL_HEIGHT );

	m_trackItems.insert( _t, item );
}


void TrackContainerScene::removeTrack( track * _t )
{
	QMap<track*,  TrackItem*>::iterator i = m_trackItems.find( _t );

	if( i != m_trackItems.end() && i.key() == _t )
	{
		TrackItem * item = i.value();
		qreal h = item->height();

		i = m_trackItems.erase(i);
		delete item;

		// Now move everything after back up
		while( i != m_trackItems.end() )
		{
			(*i)->setY( (*i)->y() - h );
			++i;
		}
	}
}



void TrackContainerScene::keyPressEvent( QKeyEvent * event )
{
	if( event->modifiers() == Qt::ShiftModifier )
	{
		const qreal cellWidth = TrackContainerScene::DEFAULT_CELL_WIDTH;

		if( event->key() == Qt::Key_Left )
		{
		}
		else if( event->key() == Qt::Key_Right )
		{
			QTimeLine * timeLine = new QTimeLine();

			// TODO: Cleanup the friendly references
			QList<QGraphicsItem*> selItems = selectedItems();
			for( QList<QGraphicsItem *>::iterator it = selItems.begin();
					it != selItems.end(); ++it )
			{
				TrackContentObjectItem * tcoItem =
					dynamic_cast<TrackContentObjectItem*>( *it );
				if( tcoItem )
				{
					qreal destPos = tcoItem->m_snapBackAnimation->posAt( 1.0 ).x();
					tcoItem->prepareSnapBackAnimation( timeLine, destPos + cellWidth );
				}
			}

			timeLine->setCurrentTime( 0.0f );
			timeLine->setDuration( 300 );
			timeLine->setCurveShape( QTimeLine::EaseInOutCurve );
			connect( timeLine, SIGNAL(finished()), timeLine, SLOT(deleteLater()));
			timeLine->start();
		}
	}
}



#include "gui/tracks/moc_track_container_scene.cxx"

