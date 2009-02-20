#ifndef _BB_TCO_ITEM_H_
#define _BB_TCO_ITEM_H_

#include <QtCore/QVector>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QPainter>
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
