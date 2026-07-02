/*
 * LmmsStyle.cpp - the graphical style used by LMMS to create a consistent
 *				  interface
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <array>

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPainterPath>  // IWYU pragma: keep
#include <QStyleFactory>
#include <QStyleOption>

#include "embed.h"
#include "LmmsStyle.h"
#include "TextFloat.h"


namespace lmms::gui
{


QPalette * LmmsStyle::s_palette = nullptr;

QLinearGradient getGradient( const QColor & _col, const QRectF & _rect )
{
	QLinearGradient g( _rect.topLeft(), _rect.bottomLeft() );

	qreal hue = _col.hueF();
	qreal value = _col.valueF();
	qreal saturation = _col.saturationF();

	QColor c = _col;
	c.setHsvF( hue, 0.42 * saturation, 0.98 * value ); // TODO: MIDI clip: 1.08
	g.setColorAt( 0, c );
	c.setHsvF( hue, 0.58 * saturation, 0.95 * value ); // TODO: MIDI clip: 1.05
	g.setColorAt( 0.25, c );
	c.setHsvF( hue, 0.70 * saturation, 0.93 * value ); // TODO: MIDI clip: 1.03
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
	for (auto& stop : stops)
	{
		QColor color = stop.second;
		stop.second = color.lighter(133);
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

	// highlight (pattern)
	if (dark)
		p->strokePath(path, QPen(borderCol.lighter(133), 2));
	else
		p->strokePath(path, QPen(borderCol, 2));
}



LmmsStyle::LmmsStyle() :
	QProxyStyle()
{
	QFile file( "resources:style.css" );
	file.open( QIODevice::ReadOnly );
	qApp->setStyleSheet( file.readAll() );

	m_styleReloader.addPath(QFileInfo{file}.absoluteFilePath());
	connect(&m_styleReloader, &QFileSystemWatcher::fileChanged, this,
		[this](const QString& path)
		{
			if (auto file = QFile{path}; file.exists())
			{
				file.open(QIODevice::ReadOnly);
				qApp->setStyleSheet(file.readAll());
				TextFloat::displayMessage(
					tr("Theme updated"),
					tr("LMMS theme file %1 has been reloaded.").arg(file.fileName()),
					embed::getIconPixmap("colorize"),
					3000
				);
				// Handle delete + overwrite events
				if (!m_styleReloader.files().contains(path))
				{
					m_styleReloader.addPath(path);
				}
			}
		}
	);

	if( s_palette != nullptr ) { qApp->setPalette( *s_palette ); }

	setBaseStyle( QStyleFactory::create( "Fusion" ) );
}




QPalette LmmsStyle::standardPalette() const
{
	if( s_palette != nullptr) { return * s_palette; }

	QPalette pal = QProxyStyle::standardPalette();

	return( pal );
}


void LmmsStyle::drawComplexControl( ComplexControl control,
					const QStyleOptionComplex * option,
					QPainter *painter,
						const QWidget *widget ) const
{
	// fix broken titlebar styling on win32
	if( control == CC_TitleBar )
	{
		const auto titleBar = qstyleoption_cast<const QStyleOptionTitleBar*>(option);
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
			QProxyStyle::drawComplexControl( control, &so,
							painter, widget );
			return;
		}
	}
	else if (control == CC_MdiControls)
	{
		QStyleOptionComplex so(*option);
		so.palette.setColor(QPalette::Button, QColor(223, 228, 236));
		QProxyStyle::drawComplexControl(control, &so, painter, widget);
		return;
	}
/*	else if( control == CC_ScrollBar )
	{
		painter->fillRect( option->rect, QApplication::palette().color( QPalette::Active,
							QPalette::Window ) );

	}*/
	QProxyStyle::drawComplexControl( control, option, painter, widget );
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

		auto lines = std::array<QLine, 4>{};
		auto points = std::array<QPoint, 4>{};

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
		painter->drawLines(lines.data(), 4);

		// black inside dots
		black.setAlpha(a50);
		painter->setPen(QPen(black, 0));
		points[0] = QPoint(rect.left() + 2, rect.top() + 2);
		points[1] = QPoint(rect.left() + 2, rect.bottom() - 2);
		points[2] = QPoint(rect.right() - 2, rect.top() + 2);
		points[3] = QPoint(rect.right() - 2, rect.bottom() - 2);
		painter->drawPoints(points.data(), 4);


		// outside lines - shadow
		// 100%
		shadow.setAlpha(a75);
		painter->setPen(QPen(shadow, 0));
		lines[0] = QLine(rect.left() + 2, rect.top(),
						rect.right() - 2, rect.top());
		lines[1] = QLine(rect.left(), rect.top() + 2,
						rect.left(), rect.bottom() - 2);
		painter->drawLines(lines.data(), 2);

		// outside corner dots - shadow
		// 75%
		shadow.setAlpha(a50);
		painter->setPen(QPen(shadow, 0));
		points[0] = QPoint(rect.left() + 1, rect.top() + 1);
		points[1] = QPoint(rect.right() - 1, rect.top() + 1);
		painter->drawPoints(points.data(), 2);

		// outside end dots - shadow
		// 50%
		shadow.setAlpha(a25);
		painter->setPen(QPen(shadow, 0));
		points[0] = QPoint(rect.left() + 1, rect.top());
		points[1] = QPoint(rect.left(), rect.top() + 1);
		points[2] = QPoint(rect.right() - 1, rect.top());
		points[3] = QPoint(rect.left(), rect.bottom() - 1);
		painter->drawPoints(points.data(), 4);


		// outside lines - highlight
		// 100%
		highlight.setAlpha(a75);
		painter->setPen(QPen(highlight, 0));
		lines[0] = QLine(rect.left() + 2, rect.bottom(),
					rect.right() - 2, rect.bottom());
		lines[1] = QLine(rect.right(), rect.top() + 2,
					rect.right(), rect.bottom() - 2);
		painter->drawLines(lines.data(), 2);

		// outside corner dots - highlight
		// 75%
		highlight.setAlpha(a50);
		painter->setPen(QPen(highlight, 0));
		points[0] = QPoint(rect.left() + 1, rect.bottom() - 1);
		points[1] = QPoint(rect.right() - 1, rect.bottom() - 1);
		painter->drawPoints(points.data(), 2);

		// outside end dots - highlight
		// 50%
		highlight.setAlpha(a25);
		painter->setPen(QPen(highlight, 0));
		points[0] = QPoint(rect.right() - 1, rect.bottom());
		points[1] = QPoint(rect.right(), rect.bottom() - 1);
		points[2] = QPoint(rect.left() + 1, rect.bottom());
		points[3] = QPoint(rect.right(), rect.top() + 1);
		painter->drawPoints(points.data(), 4);
	}
	else
	{
		QProxyStyle::drawPrimitive( element, option, painter, widget );
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
			return QProxyStyle::pixelMetric( _metric, _option, _widget );
	}
}


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


} // namespace lmms::gui
