#ifndef _PATTERN_ITEM_H_
#define _PATTERN_ITEM_H_

#include <QtCore/QVector>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QPainter>
#include <math.h>

#include "gui/tracks/track_content_object_item.h"

class trackContentObject;
class TrackItem;

class PatternItem : public TrackContentObjectItem
{
	Q_OBJECT

public:
    PatternItem( TrackItem * _track, trackContentObject * _object );

	void paint( QPainter * _painter, const QStyleOptionGraphicsItem * _option,
						QWidget * _widget );

	QVariant itemChange( GraphicsItemChange _change, const QVariant & _value );


protected:
    virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event );
	
};

#endif
