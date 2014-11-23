/*
 * LmmsStyle.cpp - the graphical style used by LMMS to create a consistent
 *				  interface
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QPainter>
#include <QtGui/QPixmapCache>
#include <QtGui/QStyleOption>

#include "LmmsStyle.h"
#include "LmmsPalette.h"

const int BUTTON_LENGTH = 24;

static const char * const s_scrollbarArrowUpXpm[] = {
		"7 6 8 1",
		" 	g None",
		".	g #000000",
		"+	g #101010",
		"@	g #A0A0A0",
		"#	g #C0C0C0",
		"$	g #FFFFFF",
		"%	g #808080",
		"&	g #202020",
		"..+@+..",
		"..#$#..",
		".%$$$%.",
		"&$$$$$&",
		"@$$$$$@",
		"@#####@"};

static const char * const s_scrollbarArrowRightXpm[] = {
		"6 7 8 1",
		" 	c None",
		".	c #A0A0A0",
		"+	c #202020",
		"@	c #000000",
		"#	c #C0C0C0",
		"$	c #FFFFFF",
		"%	c #808080",
		"&	c #101010",
		"..+@@@",
		"#$$%@@",
		"#$$$#&",
		"#$$$$.",
		"#$$$#&",
		"#$$%@@",
		"..+@@@"};

static const char * const s_scrollbarArrowDownXpm[] = {
		"7 6 8 1",
		" 	g None",
		".	g #000000",
		"+	g #101010",
		"@	g #A0A0A0",
		"#	g #C0C0C0",
		"$	g #FFFFFF",
		"%	g #808080",
		"&	g #202020",
		"@#####@",
		"@$$$$$@",
		"&$$$$$&",
		".%$$$%.",
		"..#$#..",
		"..+@+.."};

static const char * const s_scrollbarArrowLeftXpm[] = {
		"6 7 8 1",
		" 	g None",
		".	g #000000",
		"+	g #202020",
		"@	g #A0A0A0",
		"#	g #808080",
		"$	g #FFFFFF",
		"%	g #C0C0C0",
		"&	g #101010",
		"...+@@",
		"..#$$%",
		"&%$$$%",
		"@$$$$%",
		"&%$$$%",
		"..#$$%",
		"...+@@"};

QPalette * LmmsStyle::s_palette = NULL;

QLinearGradient getGradient( const QColor & _col, const QRectF & _rect )
{
	QLinearGradient g( _rect.topLeft(), _rect.bottomLeft() );

	qreal hue = _col.hueF();
	qreal value = _col.valueF();
	qreal saturation = _col.saturationF();

	QColor c = _col;
	c.setHsvF( hue, 0.42 * saturation, 0.98 * value ); // TODO: pattern: 1.08
	g.setColorAt( 0, c );
	c.setHsvF( hue, 0.58 * saturation, 0.95 * value ); // TODO: pattern: 1.05
	g.setColorAt( 0.25, c );
	c.setHsvF( hue, 0.70 * saturation, 0.93 * value ); // TODO: pattern: 1.03
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
		stops[i].second = color.lighter(133);
	}

	QLinearGradient g = _gradient;
	g.setStops(stops);
	return g;
}



void drawPath( QPainter *p, const QPainterPath &path,
			   const QColor &col, const QColor &borderCol,
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

	// TODO: Remove??
	/*
	QLinearGradient g(pathRect.topLeft(), pathRect.topRight());
	g.setCoordinateMode(QGradient::ObjectBoundingMode);

	p->setOpacity(0.2);
	p->fillPath(path, g);*/
	// END: Remove??

	p->setOpacity(0.5);

	// highlight (bb)
	if (dark)
		p->strokePath(path, QPen(borderCol.lighter(133), 2));
	else
		p->strokePath(path, QPen(borderCol, 2));
}



LmmsStyle::LmmsStyle() :
	QPlastiqueStyle()
{
	QFile file( "resources:style.css" );
	file.open( QIODevice::ReadOnly );
	qApp->setStyleSheet( file.readAll() );

	if( s_palette != NULL ) { qApp->setPalette( *s_palette ); }
}




QPalette LmmsStyle::standardPalette( void ) const
{
	if( s_palette != NULL) { return * s_palette; }

	QPalette pal = QPlastiqueStyle::standardPalette();

	return( pal );
}



/*
void LmmsStyle::drawControl( ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const
{

	switch( element )
	{
	case CE_ScrollBarAddLine:
		if( const QStyleOptionSlider * scrollBar = qstyleoption_cast<const QStyleOptionSlider *>( option ) )
		{

			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QString pixmapName = getCacheKey( QLatin1String( "ScrollBarAddLine" ), option, option->rect.size() );

			QPixmap cache;
			if( !QPixmapCache::find( pixmapName, cache ) )
			{
				cache = QPixmap( option->rect.size() );
				QPainter cachePainter( &cache );

				cache.fill( QColor( 48, 48, 48 ) );
				QColor sliderColor;
				QColor blurColor;
				hoverColors(sunken, hover,
						scrollBar->activeSubControls & SC_ScrollBarAddLine && isEnabled,
						sliderColor, blurColor);

				int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, option, widget );
				cachePainter.setPen( QPen( sliderColor, 0 ) );
				if( horizontal )
				{
					int r = cache.rect().right();
					cachePainter.fillRect(0, 2, BUTTON_LENGTH-2,
							scrollBarExtent-4, sliderColor);

					cachePainter.drawLine( r, 4, r, scrollBarExtent - 5 );
					cachePainter.drawLine( r - 1, 3, r - 1, scrollBarExtent - 4 );

					const QPointF points[4] = {
							QPoint( r, 3 ), QPoint( r, scrollBarExtent - 4 ),
							QPoint( r - 1, 2 ), QPoint( r - 1, scrollBarExtent - 3 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowRightXpm, option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarAddLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				else
				{
					int b = cache.rect().bottom();
					cachePainter.fillRect( 2, 0, scrollBarExtent - 4, BUTTON_LENGTH - 2, sliderColor );
					cachePainter.drawLine( 4, b, scrollBarExtent - 5, b );
					cachePainter.drawLine( 3, b - 1, scrollBarExtent - 4, b - 1 );

					const QPointF points[4] = {
							QPoint( 3, b ), QPoint( scrollBarExtent - 4, b ),
							QPoint( 2, b - 1 ), QPoint( scrollBarExtent - 3, b - 1 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowDownXpm, option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarAddLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				QPixmapCache::insert( pixmapName, cache );
			}
			painter->drawPixmap( option->rect.topLeft(), cache );

		}
		break;

	case CE_ScrollBarSubLine:
		if(const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>( option ) )
		{
			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QString pixmapName = getCacheKey( QLatin1String( "ScrollBarSubLine" ), option, option->rect.size() );
			QPixmap cache;
			if( !QPixmapCache::find( pixmapName, cache ) )
			{
				cache = QPixmap( option->rect.size() );
				QPainter cachePainter( &cache );

				cache.fill( QColor( 48, 48, 48 ) ); // TODO: Fill with CSS background
				QColor sliderColor;
				QColor blurColor;
				hoverColors(sunken, hover,
						scrollBar->activeSubControls & SC_ScrollBarSubLine && isEnabled,
						sliderColor, blurColor);

				int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, option, widget );
				cachePainter.setPen( QPen( sliderColor, 0 ) );
				if( horizontal )
				{
					cachePainter.fillRect( 2, 2, BUTTON_LENGTH - 2, scrollBarExtent - 4, sliderColor );
					cachePainter.drawLine( 0, 4, 0, scrollBarExtent - 5 );
					cachePainter.drawLine( 1, 3, 1, scrollBarExtent - 4 );

					const QPointF points[4] = {
							QPoint( 0, 3 ), QPoint( 0, scrollBarExtent - 4 ),
							QPoint( 1, 2 ), QPoint( 1, scrollBarExtent - 3 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowLeftXpm, option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarSubLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				else
				{
					cachePainter.fillRect( 2, 2, scrollBarExtent - 4, BUTTON_LENGTH - 2, sliderColor );
					cachePainter.drawLine( 4, 0, scrollBarExtent - 5, 0 );
					cachePainter.drawLine( 3, 1, scrollBarExtent - 4, 1 );

					const QPointF points[4] = {
							QPoint( 3, 0 ), QPoint( scrollBarExtent - 4, 0 ),
							QPoint( 2, 1 ), QPoint( scrollBarExtent - 3, 1 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowUpXpm, option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarSubLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				QPixmapCache::insert( pixmapName, cache );
			}
			painter->drawPixmap( option->rect.topLeft(), cache );

		}
		break;

	case CE_ScrollBarSubPage:
	case CE_ScrollBarAddPage:
		break;

	case CE_ScrollBarSlider:
		if( const QStyleOptionSlider* scrollBar = qstyleoption_cast<const QStyleOptionSlider *>( option ) )
		{
			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QColor sliderColor, blendColor;
			hoverColors( sunken, hover, (scrollBar->activeSubControls & SC_ScrollBarSlider) && isEnabled, sliderColor, blendColor);

			QRect rc = scrollBar->rect;
			if( horizontal )
			{
				painter->fillRect( rc.left(), rc.top() + 2, rc.width(), rc.height() - 4, sliderColor );
			}
			else
			{
				painter->fillRect( rc.left() + 2, rc.top(), rc.width() - 4, rc.height(), sliderColor );
			}
		}
		break;

	default:
		QPlastiqueStyle::drawControl( element, option, painter, widget );
		break;
	}
}

*/

void LmmsStyle::drawComplexControl( ComplexControl control,
					const QStyleOptionComplex * option,
					QPainter *painter,
						const QWidget *widget ) const
{
	// fix broken titlebar styling on win32
	if( control == CC_TitleBar )
	{
		const QStyleOptionTitleBar * titleBar =
			qstyleoption_cast<const QStyleOptionTitleBar *>(option );
		if( titleBar )
		{
			QStyleOptionTitleBar so( *titleBar );
			so.palette = standardPalette();
			so.palette.setColor( QPalette::HighlightedText,
				( titleBar->titleBarState & State_Active ) ?
					QColor( 255, 255, 255 ) :
						QColor( 192, 192, 192 ) );
			so.palette.setColor( QPalette::Text,
							QColor( 64, 64, 64 ) );
			QPlastiqueStyle::drawComplexControl( control, &so,
							painter, widget );
			return;
		}
	}
/*	else if( control == CC_ScrollBar )
	{
		painter->fillRect( option->rect, QApplication::palette().color( QPalette::Active,
							QPalette::Background ) );

	}*/
	QPlastiqueStyle::drawComplexControl( control, option, painter, widget );
}




void LmmsStyle::drawPrimitive( PrimitiveElement element,
		const QStyleOption *option, QPainter *painter,
		const QWidget *widget) const
{
	if( element == QStyle::PE_Frame ||
			element == QStyle::PE_FrameLineEdit ||
			element == QStyle::PE_PanelLineEdit )
	{
		const QRect rect = option->rect;

		QColor black = QColor( 0, 0, 0 );
		QColor shadow = option->palette.shadow().color();
		QColor highlight = option->palette.highlight().color();

		int a100 = 165;
		int a75 = static_cast<int>( a100 * .75 );
		int a50 = static_cast<int>( a100 * .6 );
		int a25 = static_cast<int>( a100 * .33 );

		QLine lines[4];
		QPoint points[4];

		// black inside lines
		// 50%
		black.setAlpha(a100);
		painter->setPen(QPen(black, 0));
		lines[0] = QLine(rect.left() + 2, rect.top() + 1,
					rect.right() - 2, rect.top() + 1);
		lines[1] = QLine(rect.left() + 2, rect.bottom() - 1,
					rect.right() - 2, rect.bottom() - 1);
		lines[2] = QLine(rect.left() + 1, rect.top() + 2,
					rect.left() + 1, rect.bottom() - 2);
		lines[3] = QLine(rect.right() - 1, rect.top() + 2,
					rect.right() - 1, rect.bottom() - 2);
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
		lines[0] = QLine(rect.left() + 2, rect.top(),
						rect.right() - 2, rect.top());
		lines[1] = QLine(rect.left(), rect.top() + 2,
						rect.left(), rect.bottom() - 2);
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
		lines[0] = QLine(rect.left() + 2, rect.bottom(),
					rect.right() - 2, rect.bottom());
		lines[1] = QLine(rect.right(), rect.top() + 2,
					rect.right(), rect.bottom() - 2);
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
		QPlastiqueStyle::drawPrimitive( element, option, painter,
								widget );
	}

}


int LmmsStyle::pixelMetric( PixelMetric _metric, const QStyleOption * _option,
						const QWidget * _widget ) const
{
	switch( _metric )
	{
		case QStyle::PM_ButtonMargin:
			return 3;

		case QStyle::PM_ButtonIconSize:
			return 20;

		case QStyle::PM_ToolBarItemMargin:
			return 1;

		case QStyle::PM_ToolBarItemSpacing:
			return 2;

		case QStyle::PM_TitleBarHeight:
			return 24;

		default:
			return QPlastiqueStyle::pixelMetric( _metric, _option,
								_widget );
	}
}

// QStyle::SH_TitleBar_NoBorder
/*
QSize LmmsStyle::sizeFromContents( ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget ) const
{
	if( type == CT_ScrollBar )
	{
		if( const QStyleOptionSlider * scrollBar = qstyleoption_cast<const QStyleOptionSlider *>( option ) )
		{
			int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, option, widget );
			int scrollBarSliderMinimum = pixelMetric( PM_ScrollBarSliderMin, option, widget );
			if( scrollBar->orientation == Qt::Horizontal )
			{
				return QSize( BUTTON_LENGTH * 2 + scrollBarSliderMinimum, scrollBarExtent );
			}
			else
			{
				return QSize( scrollBarExtent, BUTTON_LENGTH * 2 + scrollBarSliderMinimum );
			}
		}
	}

	return QPlastiqueStyle::sizeFromContents( type, option, size, widget );
}
*/
/*
QRect LmmsStyle::subControlRect( ComplexControl control, const QStyleOptionComplex* option, SubControl subControl, const QWidget* widget ) const
{
	QRect rect = QPlastiqueStyle::subControlRect( control, option, subControl, widget );

	switch( control )
	{
	case CC_ScrollBar:
		if( const QStyleOptionSlider* scrollBar = qstyleoption_cast<const QStyleOptionSlider *>( option ) )
		{
			int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, scrollBar, widget );
			int sliderMaxLength = (
					( scrollBar->orientation == Qt::Horizontal ) ?
							scrollBar->rect.width() :
							scrollBar->rect.height() ) -
					( BUTTON_LENGTH * 2 + 6 );
			int sliderMinLength = pixelMetric( PM_ScrollBarSliderMin, scrollBar, widget );
			int sliderLength;

			// calculate slider length
			if( scrollBar->maximum != scrollBar->minimum )
			{
				uint valueRange = scrollBar->maximum - scrollBar->minimum;
				sliderLength = ( scrollBar->pageStep * sliderMaxLength ) /
						( valueRange + scrollBar->pageStep );

				if( sliderLength < sliderMinLength || valueRange > ( INT_MAX ) / 2 )
				{
					sliderLength = sliderMinLength;
				}
				if( sliderLength > sliderMaxLength )
				{
					sliderLength = sliderMaxLength;
				}
			}
			else
			{
				sliderLength = sliderMaxLength;
			}

			int sliderStart = BUTTON_LENGTH + 3 + sliderPositionFromValue(
					scrollBar->minimum,
					scrollBar->maximum,
					scrollBar->sliderPosition,
					sliderMaxLength - sliderLength,
					scrollBar->upsideDown );

			QRect scrollBarRect = scrollBar->rect;

			switch( subControl )
			{
			case SC_ScrollBarSubLine: // top/left button
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect.setRect( scrollBarRect.left() + 2, scrollBarRect.top(),
							BUTTON_LENGTH,	scrollBarExtent );
				}
				else
				{
					rect.setRect( scrollBarRect.left(), scrollBarRect.top() + 2,
							scrollBarExtent, BUTTON_LENGTH );
				}
				break;

			case SC_ScrollBarAddLine: // bottom/right button
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect.setRect( scrollBarRect.right() - 1 - BUTTON_LENGTH,
							scrollBarRect.top(), BUTTON_LENGTH, scrollBarExtent );
				}
				else
				{
					rect.setRect( scrollBarRect.left(),
							scrollBarRect.bottom() - 1 - BUTTON_LENGTH,
							scrollBarExtent, BUTTON_LENGTH );
				}
				break;

			case SC_ScrollBarSubPage:
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect.setRect( scrollBarRect.left() + 2 + BUTTON_LENGTH,
							scrollBarRect.top(),
							sliderStart - ( scrollBarRect.left() + 2 + BUTTON_LENGTH ),
							scrollBarExtent );
				}
				else
				{
					rect.setRect( scrollBarRect.left(),
							scrollBarRect.top() + 2 + BUTTON_LENGTH, scrollBarExtent,
							sliderStart - ( scrollBarRect.left() + 2 + BUTTON_LENGTH ) );
				}
				break;

			case SC_ScrollBarAddPage:
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect.setRect( sliderStart + sliderLength, 0,
							sliderMaxLength - sliderStart - sliderLength,
							scrollBarExtent );
				}
				else
					rect.setRect( 0, sliderStart + sliderLength, scrollBarExtent,
							sliderMaxLength - sliderStart - sliderLength );
				break;

			case SC_ScrollBarGroove:
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect = scrollBarRect.adjusted( BUTTON_LENGTH, 0, -BUTTON_LENGTH, 0 );
				}
				else
				{
					rect = scrollBarRect.adjusted( 0, BUTTON_LENGTH, 0, -BUTTON_LENGTH );
				}
				break;

			case SC_ScrollBarSlider:
				if( scrollBar->orientation == Qt::Horizontal )
				{
					rect.setRect( sliderStart, 0, sliderLength, scrollBarExtent );
				}
				else
				{
					rect.setRect( 0, sliderStart, scrollBarExtent, sliderLength );
				}
				break;

			default:
				break;
			}
			rect = visualRect( scrollBar->direction, scrollBarRect, rect );
		}
		break;

	default:
		break;
	}

	return rect;
}
*/



QImage LmmsStyle::colorizeXpm( const char * const * xpm, const QBrush& fill ) const
{
	QImage arrowXpm( xpm );
	QImage arrow( arrowXpm.size(), QImage::Format_ARGB32 );
	QPainter arrowPainter( &arrow );
	arrowPainter.fillRect( arrow.rect(), fill );
	arrowPainter.end();
	arrow.setAlphaChannel( arrowXpm );

	return arrow;
}


void LmmsStyle::hoverColors( bool sunken, bool hover, bool active, QColor& color, QColor& blend ) const
{
	if( active )
	{
		if( sunken )
		{
			color = QColor( 75, 75, 75 );
			blend = QColor( 65, 65, 65 );
		}
		else if( hover )
		{
			color = QColor( 100, 100, 100 );
			blend = QColor( 75, 75, 75 );
		}
		else
		{
			color = QColor( 21, 21, 21 );
			blend = QColor( 33, 33, 33 );
		}
	}
	else
	{
		color = QColor( 21, 21, 21 );
		blend = QColor( 33, 33, 33 );
	}
}

