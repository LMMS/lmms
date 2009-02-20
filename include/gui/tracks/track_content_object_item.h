#ifndef TRACK_CONTENT_OBJECT_ITEM_H_
#define TRACK_CONTENT_OBJECT_ITEM_H_

#include <QtCore/QVector>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QPainter>
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

protected:
	virtual QVariant itemChange( GraphicsItemChange _change, const QVariant & _value );

	virtual void mousePressEvent( QGraphicsSceneMouseEvent * event );
	virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent * event );
	virtual void hoverEnterEvent( QGraphicsSceneHoverEvent * event );
	virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent * event );

	void prepareSnapBackAnimation( QTimeLine * timeLine );
	void prepareSnapBackAnimation( QTimeLine * timeLine, int newX );

protected slots:
	void updateLength( void );
	void updatePosition( void );

protected:
	QPointF m_lastPos;
	QPointF m_lastDest;
	tact m_length;
	QGraphicsItemAnimation * m_snapBackAnimation;
	static QTimeLine s_snapBackTimeLine;

	TrackItem * m_trackItem;
	trackContentObject * m_tco;
	bool m_hover;
};

#endif

