#include <QObject>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QStyleOptionGraphicsItem>

#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_item.h"
#include "track.h"


TrackContentObjectItem::TrackContentObjectItem( TrackItem * _track, trackContentObject * _object ) :
        QObject(),
		QGraphicsItem(),
		m_trackItem( _track ),
		m_tco( _object ),
        m_snapBackAnimation( NULL )
{
    //m_object =  _object;
	setFlags( QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable );
    setCursor( Qt::OpenHandCursor );

    midiTime startPos = _object->startPosition();
    float x = TrackContainerScene::DEFAULT_CELL_WIDTH *
            startPos.getTicks() / midiTime::ticksPerTact();
    setPos( x, _track->y() );
    setZValue(x);

    m_snapBackAnimation = new QGraphicsItemAnimation();
    m_snapBackAnimation->setItem( this );
}

QRectF TrackContentObjectItem::boundingRect() const
{
	qreal penWidth = 1;
	// TODO: Calculate based on track length and an adjustable height.
	/*return QRectF( -64 - penWidth / 2, -16 - penWidth / 2,
			128 + penWidth / 2, 32 + penWidth / 2 );*/
    QRectF rc = QRectF( 0, 0, TrackContainerScene::DEFAULT_CELL_WIDTH * 
            m_tco->length().getTicks() / midiTime::ticksPerTact(), m_trackItem->height() );
    return rc;
}

qreal TrackContentObjectItem::zValue() const
{
    //return x(); 
    //return m_tco->startPotion().getTicks();
    return QGraphicsItem::zValue();
}
    
void TrackContentObjectItem::updateGeometry()
{
    prepareGeometryChange();
}


void TrackContentObjectItem::paint( QPainter * _painter,
		const QStyleOptionGraphicsItem * _option, QWidget * _widget )
{

    QColor col;
    if( !isSelected() )
    {
        col = QColor( 0x00, 0x33, 0x99 );
    }
    else
    {
        col = QColor( 0x99, 0x33, 0x00 );
    }
	QColor col0 = col.light( 130 );
	QColor col1 = col.light( 70 );
	QColor col2 = col.light( 40 );
	
	QRectF rc = boundingRect();

    qreal xscale = _option->matrix.m11();
	_painter->save();
    _painter->scale( 1.0f/xscale, 1.0f );
    rc.setWidth( rc.width() * xscale );

    QLinearGradient lingrad( 0, 0, 0, rc.height() );
	lingrad.setColorAt( 0, col0 );
	lingrad.setColorAt( 1, col1 );

    QLinearGradient bordergrad( 0, 0, 0, rc.height() );
	bordergrad.setColorAt( 1, col2 );
	bordergrad.setColorAt( 0, col1 );

    
    rc.adjust( 0, 0, -1, -1 );


    _painter->setRenderHint( QPainter::Antialiasing, true );
	_painter->setBrush( lingrad );
	_painter->setPen( QPen( bordergrad, 1.5 ) ); // QColor( 6, 6, 6 ) );
	_painter->drawRoundedRect( rc, 4, 4 );
    _painter->setRenderHint( QPainter::Antialiasing, false );

	_painter->restore();
}

QVariant TrackContentObjectItem::itemChange( GraphicsItemChange _change, const QVariant & _value )
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
		if( newPos.y() < 0 )
		{
			newPos.setY( 0 );
		}

        /* Let's just short-circuit the Y for now 
		if( fmod( newPos.y(), 32 ) != 16 )
		{
			newPos.setY( newPos.y() + 16 - ( fmod( newPos.y(), 32 ) ) );
		} */
        newPos.setY( m_trackItem->y() );
		
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
    prepareSnapBackAnimation( timeLine );

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
} 
 


void TrackContentObjectItem::hoverEnterEvent( QGraphicsSceneHoverEvent * event )
{
    QGraphicsItem::hoverEnterEvent( event );
}



void TrackContentObjectItem::hoverLeaveEvent( QGraphicsSceneHoverEvent * event )
{
    QGraphicsItem::hoverLeaveEvent( event );
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
