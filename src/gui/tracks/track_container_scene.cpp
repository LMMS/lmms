#include <QGraphicsScene>
#include <QKeyEvent>
#include <QTimeLine>

#include <stdio.h>

#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_item.h"
#include "track_container.h"

TrackContainerScene::TrackContainerScene( QObject * parent, trackContainer * _tc ) :
		QGraphicsScene( parent ),
		//modelView( NULL, this ),
		//journallingObject(),
		//serializingObjectHook(),
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
	QMap<track*,  TrackItem*>::iterator i =
			m_trackItems.find( _t );

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

