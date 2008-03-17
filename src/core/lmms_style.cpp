#include <QtGui/QPlastiqueStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QPainter>
#include <QtGui/QWidget>
#include <QtGui/QFrame>

#include "lmms_style.h"


void lmmsStyle::drawPrimitive( PrimitiveElement element, 
		const QStyleOption *option, QPainter *painter, 
		const QWidget *widget) const
{
	QPlastiqueStyle::drawPrimitive( element, option, painter, widget );
		/*
	if( element == QStyle::PE_Frame ) {
		printf("Frame\n");

		QFrame::Shadow shadow = QFrame::Plain;
        if (option->state & State_Sunken)
            shadow = QFrame::Sunken;
        else if (option->state & State_Raised)
            shadow = QFrame::Raised;

 QPen oldPen = painter->pen();
    QBrush border;
    QBrush corner;
    QBrush innerTopLeft;
    QBrush innerBottomRight;

    if (shadow != QFrame::Plain && (option->state & QStyle::State_HasFocus)) {
        border = option->palette.highlight();
        qBrushSetAlphaF(&border, 0.8);
        corner = option->palette.highlight();
        qBrushSetAlphaF(&corner, 0.5);
        innerTopLeft = qBrushDark(option->palette.highlight(), 125);
        innerBottomRight = option->palette.highlight();
        qBrushSetAlphaF(&innerBottomRight, 0.65);
    } else {
        border = option->palette.shadow();
        qBrushSetAlphaF(&border, 0.4);
        corner = option->palette.shadow();
        qBrushSetAlphaF(&corner, 0.25);
        innerTopLeft = option->palette.shadow();
        innerBottomRight = option->palette.highlight();
        if (shadow == QFrame::Sunken) {
            qBrushSetAlphaF(&innerTopLeft, 0.23);
            qBrushSetAlphaF(&innerBottomRight, 0.075);
        } else {
            qBrushSetAlphaF(&innerTopLeft, 0.075);
            qBrushSetAlphaF(&innerBottomRight, 0.23);
        }
    }

    QLine lines[4];
    QPoint points[8];

    // Opaque corner lines
    painter->setPen(QPen(border, 0));
    lines[0] = QLine(rect.left() + 2, rect.top(), rect.right() - 2, rect.top());
    lines[1] = QLine(rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom());
    lines[2] = QLine(rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2);
    lines[3] = QLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2);
    painter->drawLines(lines, 4);

    // Opaque corner dots
    points[0] = QPoint(rect.left() + 1, rect.top() + 1);
    points[1] = QPoint(rect.left() + 1, rect.bottom() - 1);
    points[2] = QPoint(rect.right() - 1, rect.top() + 1);
    points[3] = QPoint(rect.right() - 1, rect.bottom() - 1);
    painter->drawPoints(points, 4);

    // Shaded corner dots
    painter->setPen(QPen(corner, 0));
    points[0] = QPoint(rect.left(), rect.top() + 1);
    points[1] = QPoint(rect.left(), rect.bottom() - 1);
    points[2] = QPoint(rect.left() + 1, rect.top());
    points[3] = QPoint(rect.left() + 1, rect.bottom());
    points[4] = QPoint(rect.right(), rect.top() + 1);
    points[5] = QPoint(rect.right(), rect.bottom() - 1);
    points[6] = QPoint(rect.right() - 1, rect.top());
    points[7] = QPoint(rect.right() - 1, rect.bottom());
    painter->drawPoints(points, 8);

    // Shadows
    if (shadow != QFrame::Plain) {
        painter->setPen(QPen(innerTopLeft, 0));
        lines[0] = QLine(rect.left() + 2, rect.top() + 1, rect.right() - 2, rect.top() + 1);
        lines[1] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
        painter->drawLines(lines, 2);
        painter->setPen(QPen(innerBottomRight, 0));
        lines[0] = QLine(rect.left() + 2, rect.bottom() - 1, rect.right() - 2, rect.bottom() - 1);
        lines[1] = QLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);
        painter->drawLines(lines, 2);
    }

    painter->setPen(oldPen);

	}
	else {
		QPlastiqueStyle::drawPrimitive( element, option, painter, widget );
	}
	// brighter line at bottom/right
	p.setPen( QColor( 160, 160, 160 ) );
	p.drawLine( width() - 1, 0, width() - 1, height() - 1 );
	p.drawLine( 0, height() - 1, width() - 1, height() - 1 );
*/


}
