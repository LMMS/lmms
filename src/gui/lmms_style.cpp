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
	if( element == QStyle::PE_Frame || element == QStyle::PE_FrameLineEdit ||
			element == QStyle::PE_PanelLineEdit )
	{
		const QRect rect = option->rect;

		QColor black = QColor( 0, 0, 0 );
		QColor shadow = option->palette.shadow().color();
		QColor highlight = option->palette.highlight().color();

		int a100 = 165;
		int a75 = a100 * .75;
		int a50 = a100 * .6;
		int a25 = a100 * .33;

		QLine lines[4];
		QPoint points[4];

		// black inside lines
		// 50%
		black.setAlpha(a100);
		painter->setPen(QPen(black, 0));
		lines[0] = QLine(rect.left() + 2, rect.top() + 1, rect.right() - 2, rect.top() + 1);
		lines[1] = QLine(rect.left() + 2, rect.bottom() - 1, rect.right() - 2, rect.bottom() - 1);
		lines[2] = QLine(rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2);
		lines[3] = QLine(rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom() - 2);
		painter->drawLines(lines, 4);

		// black inside dots
		black.setAlpha(a50);
		painter->setPen(QPen(black, 0));
		points[0] = QPoint(rect.left() + 2, rect.top() + 2);
		points[1] = QPoint(rect.left() + 2, rect.bottom() - 2);
		points[2] = QPoint(rect.right() - 2, rect.top() + 2);
		points[3] = QPoint(rect.right() - 2, rect.bottom() - 2);
		painter->drawPoints(points, 4);


		// outside lines - shadow
		// 100%
		shadow.setAlpha(a75);
		painter->setPen(QPen(shadow, 0));
		lines[0] = QLine(rect.left() + 2, rect.top(), rect.right() - 2, rect.top());
		lines[1] = QLine(rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2);
		painter->drawLines(lines, 2);
		
		// outside corner dots - shadow
		// 75%
		shadow.setAlpha(a50);
		painter->setPen(QPen(shadow, 0));
		points[0] = QPoint(rect.left() + 1, rect.top() + 1);
		points[1] = QPoint(rect.right() - 1, rect.top() + 1);
		painter->drawPoints(points, 2);
		
		// outside end dots - shadow
		// 50%
		shadow.setAlpha(a25);
		painter->setPen(QPen(shadow, 0));
		points[0] = QPoint(rect.left() + 1, rect.top());
		points[1] = QPoint(rect.left(), rect.top() + 1);
		points[2] = QPoint(rect.right() - 1, rect.top());
		points[3] = QPoint(rect.left(), rect.bottom() - 1);
		painter->drawPoints(points, 4);
		

		// outside lines - highlight
		// 100%
		highlight.setAlpha(a75);
		painter->setPen(QPen(highlight, 0));
		lines[0] = QLine(rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom());
		lines[1] = QLine(rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2);
		painter->drawLines(lines, 2);
		
		// outside corner dots - highlight
		// 75%
		highlight.setAlpha(a50);
		painter->setPen(QPen(highlight, 0));
		points[0] = QPoint(rect.left() + 1, rect.bottom() - 1);
		points[1] = QPoint(rect.right() - 1, rect.bottom() - 1);
		painter->drawPoints(points, 2);
		
		// outside end dots - highlight
		// 50%
		highlight.setAlpha(a25);
		painter->setPen(QPen(highlight, 0));
		points[0] = QPoint(rect.right() - 1, rect.bottom());
		points[1] = QPoint(rect.right(), rect.bottom() - 1);
		points[2] = QPoint(rect.left() + 1, rect.bottom());
		points[3] = QPoint(rect.right(), rect.top() + 1);
		painter->drawPoints(points, 4);
	}
	else 
	{
		QPlastiqueStyle::drawPrimitive( element, option, painter, widget );
	}

}


int lmmsStyle::pixelMetric( PixelMetric _metric, const QStyleOption * _option, const QWidget * _widget ) const
{
	switch( _metric )
	{
		case QStyle::PM_ButtonMargin:
			return 3;

		case QStyle::PM_ButtonIconSize:
			return 20;

		default:
			return QPlastiqueStyle::pixelMetric( _metric, _option, _widget );
	}
}

