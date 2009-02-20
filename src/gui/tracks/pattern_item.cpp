#include <QObject>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QStyleOptionGraphicsItem>

#include "gui/tracks/pattern_item.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_item.h"
#include "engine.h"
#include "lmms_style.h"
#include "track.h"



PatternItem::PatternItem( TrackItem * _track, trackContentObject * _object ) :
	TrackContentObjectItem( _track, _object )
{
}

void PatternItem::paint(
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
	options->type = LmmsStyleOptionTCO::Pattern;
	options->rect = rc;
	options->selected = isSelected();
	options->hovered = m_hover;
	options->duration = 0;

	engine::getLmmsStyle()->drawTrackContentObject( _painter, m_tco, options );

	_painter->restore();
	delete options;
	return;
}



/*
QVariant PatternItem::itemChange( QGraphicsItemChange _change, const QVariant & _value )
{
	return TrackContentObjectItem::itemChange( _change, _value );
}*/


void PatternItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
	TrackContentObjectItem::mousePressEvent( event );
}



void PatternItem::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
    TrackContentObjectItem::mouseReleaseEvent( event );
} 
 

#include "gui/tracks/moc_pattern_item.cxx"
