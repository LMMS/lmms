#include <QObject>
#include <QGraphicsItem>
#include <QTimeLine>
#include <QGraphicsItemAnimation>
#include <QStyleOptionGraphicsItem>

#include "gui/tracks/bb_tco_item.h"
#include "gui/tracks/track_content_object_item.h"
#include "gui/tracks/track_container_scene.h"
#include "gui/tracks/track_item.h"
#include "track.h"

namespace BBTcoStuff {
QLinearGradient getGradient( const QColor & _col, const QRectF & _rect )
{
    QLinearGradient g( _rect.topLeft(), _rect.bottomLeft() );

    qreal hue = _col.hueF();
    qreal value = _col.valueF();
    qreal saturation = _col.saturationF();

    QColor c = _col;
    c.setHsvF( hue, 0.42 * saturation, 0.98 * value );
    g.setColorAt( 0, c );
    c.setHsvF( hue, 0.58 * saturation, 0.95 * value );
    g.setColorAt( 0.25, c );
    c.setHsvF( hue, 0.70 * saturation, 0.93 * value );
    g.setColorAt( 0.5, c );

    c.setHsvF( hue, 0.95 * saturation, 0.9 * value );
    g.setColorAt( 0.501, c );
    c.setHsvF( hue * 0.95, 0.95 * saturation, 0.95 * value );
    g.setColorAt( 0.75, c );
    c.setHsvF( hue * 0.90, 0.95 * saturation, 1 * value );
    g.setColorAt( 1.0, c );

    return g;
}

QLinearGradient darken( const QLinearGradient & _gradient )
{
    QGradientStops stops = _gradient.stops();
    for (int i = 0; i < stops.size(); ++i) {
        QColor color = stops.at(i).second;
        stops[i].second = color.darker(160);
    }

    QLinearGradient g = _gradient;
    g.setStops(stops);
    return g;
}

void drawPath( QPainter *p, const QPainterPath &path,
              const QColor &col, const QString &name,
              bool dark = false )
{
    const QRectF pathRect = path.boundingRect();

    const QLinearGradient baseGradient = getGradient(col, pathRect);
    const QLinearGradient darkGradient = darken(baseGradient);

    p->setOpacity(0.25);

    // glow
    if (dark)
        p->strokePath(path, QPen(darkGradient, 4));
    else
        p->strokePath(path, QPen(baseGradient, 4));

    p->setOpacity(1.0);

    // fill
    if (dark)
        p->fillPath(path, darkGradient);
    else
        p->fillPath(path, baseGradient);

    QLinearGradient g(pathRect.topLeft(), pathRect.topRight());
    g.setCoordinateMode(QGradient::ObjectBoundingMode);

    p->setOpacity(0.2);
    p->fillPath(path, g);

    p->setOpacity(0.5);

    // highlight
    if (dark)
        p->strokePath(path, QPen(col.lighter(160).darker(160), 2));
    else
        p->strokePath(path, QPen(col.lighter(160), 2));

}
}
using namespace BBTcoStuff;

BbTrackContentObjectItem::BbTrackContentObjectItem( TrackItem * _track, trackContentObject * _object ) :
        TrackContentObjectItem( _track, _object )
{
}

void BbTrackContentObjectItem::paint( QPainter * _painter,
		const QStyleOptionGraphicsItem * _option, QWidget * _widget )
{

    QColor col;
    if( !isSelected() )
    {
        col = QColor( 0x00, 0x33, 0x99 );
    }
    else
    {
        col = QColor( 0x00, 0x99, 0x33 );
    }

	QRectF rc = boundingRect();

    qreal xscale = _option->matrix.m11();
	_painter->save();
    _painter->scale( 1.0f/xscale, 1.0f );
    rc.setWidth( rc.width() * xscale );

    QPainterPath path;
    path.addRoundedRect(2, 2, rc.width()-4, rc.height()-4, 4, 4);
    drawPath( _painter, path, col, "hah", false );

    const float cellW = TrackContainerScene::DEFAULT_CELL_WIDTH;
    _painter->setOpacity(0.2);
    _painter->setRenderHint( QPainter::Antialiasing, false );
    _painter->setPen( QColor(0, 0, 0) );
    for( int i = 0; i < m_tco->length().getTact(); ++i )
    {
        float x = cellW * i;
        _painter->drawLine(x, 2, x, rc.height()-4);
    }

    _painter->translate( 1, 0 );
    _painter->setPen( col.lighter(160) );
    for( int i = 0; i < m_tco->length().getTact(); ++i )
    {
        float x = cellW * i;
        _painter->drawLine(x, 2, x, rc.height()-4);
    }

    _painter->setRenderHint( QPainter::Antialiasing, true );
    _painter->restore();

    return;


	QColor col0 = col.light( 130 );
	QColor col1 = col.light( 70 );
	QColor col2 = col.light( 40 );
	
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

QVariant BbTrackContentObjectItem::itemChange( GraphicsItemChange _change, const QVariant & _value )
{
    return TrackContentObjectItem::itemChange( _change, _value );
}


void BbTrackContentObjectItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    TrackContentObjectItem::mousePressEvent( event );
}



void BbTrackContentObjectItem::mouseReleaseEvent( QGraphicsSceneMouseEvent * event )
{
    TrackContentObjectItem::mouseReleaseEvent( event );
} 
 

#include "gui/tracks/moc_bb_tco_item.cxx"
