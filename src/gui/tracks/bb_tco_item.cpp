#include <QObject>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QStyleOptionGraphicsItem>

#include "gui/tracks/bb_tco_item.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_item.h"
#include "lmms_style.h"
#include "bb_track.h"
#include "bb_track_container.h"
#include "track.h"
#include "engine.h"



BbTrackContentObjectItem::BbTrackContentObjectItem(
		TrackItem * _track,
		trackContentObject * _object ) :
	TrackContentObjectItem( _track, _object )
{
}



void BbTrackContentObjectItem::paint(
		QPainter * _painter,
		const QStyleOptionGraphicsItem * _option,
		QWidget * _widget )
{
	QRectF rc = boundingRect();

	qreal xscale = _option->matrix.m11();
	_painter->save();
	_painter->scale( 1.0f/xscale, 1.0f );
	rc.setWidth( rc.width() * xscale );

	// TODO: Use a proxy class
	LmmsStyleOptionTCO * options = new LmmsStyleOptionTCO();
	options->type = LmmsStyleOptionTCO::BbTco;
	options->rect = rc;
	options->selected = isSelected();
	options->hovered = m_hover;

	bbTCO * bbTco  = (bbTCO*)m_tco;
	int trackNum = bbTrack::numOfBBTrack( bbTco->getTrack() );
	options->duration = engine::getBBTrackContainer()->lengthOfBB( trackNum );

	engine::getLmmsStyle()->drawTrackContentObject( _painter, m_tco, options );

	_painter->restore();
	delete options;
	return;
}



QVariant BbTrackContentObjectItem::itemChange(
		GraphicsItemChange _change,
		const QVariant & _value )
{
	return TrackContentObjectItem::itemChange( _change, _value );
}



void BbTrackContentObjectItem::mousePressEvent(
		QGraphicsSceneMouseEvent * event )
{
	TrackContentObjectItem::mousePressEvent( event );
}



void BbTrackContentObjectItem::mouseReleaseEvent(
		QGraphicsSceneMouseEvent * event )
{
	TrackContentObjectItem::mouseReleaseEvent( event );
}


#include "gui/tracks/moc_bb_tco_item.cxx"
