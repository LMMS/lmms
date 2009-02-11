
#include "track.h"
#include "gui/tracks/track_item.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"

#include "gui/tracks/bb_tco_item.h"
#include "gui/tracks/pattern_item.h"

#include "bb_track.h"
#include "pattern.h"

TrackItem::TrackItem( TrackContainerScene * _scene, track * _track )
{
	m_scene = _scene;
	m_track = _track;

	// create views for already existing TCOs
	const track::tcoVector & tcos = m_track->getTCOs();
	for( track::tcoVector::const_iterator it = tcos.begin(); it != tcos.end(); ++it )
	{
		addTCO( *it );
	}

	QObject * obj = _track;
	connect( obj, SIGNAL( trackContentObjectAdded( trackContentObject * ) ),
			this, SLOT( addTCO( trackContentObject * ) ),
			Qt::QueuedConnection );

	connect( obj, SIGNAL( trackContentObjectRemoved( trackContentObject * ) ),
			this, SLOT( removeTCO( trackContentObject * ) ) );
}



/* WTF?!?! */
TrackItem::~TrackItem()
{
    for( QMap<trackContentObject*,  TrackContentObjectItem*>::iterator i = m_tcoItems.begin();
            i != m_tcoItems.end(); ++i )
    {
		TrackContentObjectItem * item = i.value();
        m_scene->removeItem(item);
		m_tcoItems.erase(i);
		delete item;
    }
}


void TrackItem::addTCO( trackContentObject * _tco )
{
    // TODO move into a factory?
    TrackContentObjectItem * tcoItem;

    if( bbTCO * bbTco = dynamic_cast<bbTCO *>( _tco ) )
    {
        tcoItem = new BbTrackContentObjectItem( this, (trackContentObject*)bbTco );
    }
    else if( pattern * pat = dynamic_cast<pattern *>( _tco ) )
    {
        tcoItem = new PatternItem( this, (trackContentObject*)pat );
    }
    else
    {
        // Whoa.
        return;
    }
    
    // TODO refactor to private updateTCOGeometry
    tcoItem->setPos( tcoItem->x(), y() );

    m_tcoItems.insert( _tco, tcoItem );
    m_scene->addItem( tcoItem );
}



void TrackItem::removeTCO( trackContentObject * _tco )
{
	QMap<trackContentObject*,  TrackContentObjectItem*>::iterator i =
			m_tcoItems.find( _tco );

	if( i != m_tcoItems.end() && i.key() == _tco )
	{
		TrackContentObjectItem * item = i.value();
        m_scene->removeItem(*i);
		m_tcoItems.erase(i);
		delete item;
	}
 }


void TrackItem::setHeight( float _height )
{
	m_rect.setHeight( _height );
	for( QMap<trackContentObject*, TrackContentObjectItem*>::const_iterator it = m_tcoItems.constBegin();
			it != m_tcoItems.constEnd(); ++it )
	{
		(*it)->updateGeometry();
    }
}



void TrackItem::setY( float _y )
{
    //printf("TRK %lld: setY(%.2f)\n", m_track, _y);
	m_rect.moveTop( _y );
    for( QMap<trackContentObject*, TrackContentObjectItem*>::const_iterator it = m_tcoItems.constBegin();
			it != m_tcoItems.constEnd(); ++it )
	{
		(*it)->setPos( (*it)->x(), y() );
    }
}



float TrackItem::height()
{
	return m_rect.height();
}



float TrackItem::y()
{
	return m_rect.y();
}

#include "gui/tracks/moc_track_item.cxx"
