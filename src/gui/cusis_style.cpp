/*
 * cusis_style.cpp - the graphical style used by LMMS for the Cusis-inspired
 *                  interface
 *
 * Copyright (c) 2009 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
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

const int BUTTON_LENGTH = 24;

#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QPainter>
#include <QtGui/QPixmapCache>
#include <QtGui/QStyleOption>
#include <QtGui/QLinearGradient>
#include <QtGui/QScrollBar>

#include "cusis_style.h"
#include "gui_templates.h"

// For TrackContainerScene::DEFAULT_CELL_WIDTH;
#include "gui/tracks/track_container_scene.h" 

#include <iostream>
#include <limits.h>

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



static QString getCacheKey( const QString & _key,
		const QStyleOption * _option, const QSize & _size )
{
	QString tmp;
	const QStyleOptionComplex *complexOption =
			qstyleoption_cast<const QStyleOptionComplex *> ( _option );
	tmp.sprintf( "%s,%d,%d,%d,%lld,%dx%d",
			_key.toLatin1().constData(),
			uint( _option->state ),
			complexOption ? uint( complexOption->activeSubControls ) : uint( 0 ),
			_option->direction,
			_option->palette.cacheKey(),
			_size.width(),
			_size.height() );
	return tmp;
}


static QString getTcoCacheKey( const QString & _key,
		const LmmsStyleOptionTCO * _option )
{
	QString tmp;
	tmp.sprintf( "%s,%d,%d,%s,%.2fx%.2f",
			_key.toLatin1().constData(),
			_option->type | ( _option->selected << 4 ) | ( _option->hovered << 5 ),
			_option->duration,
			_option->userColor.name().toLatin1().data(),
			_option->rect.width(),
			_option->rect.height());
	return tmp;
}


CusisStyle::CusisStyle() :
	QPlastiqueStyle(), LmmsStyle()
{
	QFile file( "resources:style.css" );
	file.open( QIODevice::ReadOnly );
	qApp->setStyleSheet( file.readAll() );

	qApp->setPalette( standardPalette() );

	// TODO: Load these from the LmmsStyle section of theme.xml
	m_colors[AutomationBarFill] = QColor( 0xFF, 0xCC, 0x33 );
	m_colors[AutomationBarValue] = QColor( 0xFF, 0xF7, 0x33 );
	m_colors[AutomationSelectedBarFill] = QColor( 0x00, 0x99, 0xFF );
	m_colors[AutomationCrosshair] = QColor( 0xFF, 0x33, 0x33 );

	m_colors[PianoRollDefaultNote] = QColor( 0x99, 0xff, 0x00 ); // or 00 ff 99
	m_colors[PianoRollFrozenNote] = QColor( 0x00, 0xE0, 0xFF );
	m_colors[PianoRollMutedNote] = QColor( 0x99, 0x99, 0x99 );
	m_colors[PianoRollStepNote] = QColor( 0xff, 0x99, 0x00 );
	m_colors[PianoRollSelectedNote] = QColor( 0x00, 0x99, 0xff );
	m_colors[PianoRollEditHandle] = QColor( 0xff, 0x99, 0x33 );
	m_colors[PianoRollSelectedLevel] = QColor( 0x00, 0x99, 0xff );
	m_colors[PianoRollVolumeLevel] = QColor( 0x99, 0xFF, 0x00 );
	m_colors[PianoRollPanningLevel] = QColor( 0xFF, 0x99, 0x00 );

	m_colors[TimelineForecolor] = QColor( 192, 192, 192 );

	m_colors[StandardGraphLine] = QColor( 0x33, 0xFF, 0x99 );
	m_colors[StandardGraphHandle] = QColor( 0xFF, 0xBF, 0x22 );
	m_colors[StandardGraphHandleBorder] = QColor( 0x00, 0x00, 0x02 );
	m_colors[StandardGraphCrosshair] = QColor( 0xAA, 0xFF, 0x00, 0x70 );

	m_colors[TextFloatForecolor] = QColor( 0, 0, 0 );
	m_colors[TextFloatFill] = QColor( 224, 224, 224 );

	m_colors[VisualizationLevelLow] = QColor( 128, 224, 128 );
	m_colors[VisualizationLevelMid] = QColor( 255, 192, 64 );
	m_colors[VisualizationLevelPeak] = QColor( 255, 64, 64 );
}



QPalette CusisStyle::standardPalette() const
{
	QPalette pal = QPlastiqueStyle::standardPalette();
	pal.setColor( QPalette::WindowText, QColor( 240, 240, 240 ) );
	//pal.setColor( QPalette::Base, QColor( 128, 128, 128 ) );
	pal.setColor( QPalette::Base, QColor( 0, 0, 0 ) );
	pal.setColor( QPalette::Shadow, QColor( 0, 0, 0 ) );
	pal.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::BrightText, QColor( 0, 255, 0 ) );
	pal.setColor( QPalette::Highlight, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::HighlightedText, QColor( 0, 0, 0 ) );

	// Cusis theme
	pal.setColor( QPalette::Background, QColor( 21, 21, 21 ) );
	pal.setColor( QPalette::Text, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::Button, QColor( 66, 66, 66 ) );

	return ( pal );
}



void CusisStyle::hoverColors( bool sunken, bool hover, bool active,
		QColor & color, QColor & blend ) const
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



void CusisStyle::drawComplexControl( ComplexControl _control,
		const QStyleOptionComplex * _option, QPainter * _painter,
		const QWidget * _widget ) const
{
	// fix broken titlebar styling on win32
	if( _control == CC_TitleBar )
	{
		const QStyleOptionTitleBar * titleBar =
				qstyleoption_cast<const QStyleOptionTitleBar *> ( _option );
		if( titleBar )
		{
			QStyleOptionTitleBar so( *titleBar );
			so.palette = standardPalette();
			so.palette.setColor( QPalette::HighlightedText,
				 ( titleBar->titleBarState & State_Active ) ?
						QColor( 255, 255, 255 ) :
						QColor( 192, 192, 192 ) );
			so.palette.setColor( QPalette::Text, QColor( 64, 64, 64 ) );
			QPlastiqueStyle::drawComplexControl( _control, &so, _painter, _widget );
			return;
		}
	}
	else if( _control == CC_ScrollBar )
	{
		QColor background = QColor( 48, 48, 48 );
		_painter->fillRect( _option->rect, background );
	}
	QPlastiqueStyle::drawComplexControl( _control, _option, _painter, _widget );
}



QSize CusisStyle::sizeFromContents( ContentsType _type, const QStyleOption * _option,
		const QSize & _size, const QWidget * _widget ) const
{
	if( _type == CT_ScrollBar )
	{
		if(const QStyleOptionSlider * scrollBar =
				qstyleoption_cast<const QStyleOptionSlider *>(_option))
		{
			int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, _option, _widget );
			int scrollBarSliderMinimum = pixelMetric( PM_ScrollBarSliderMin, _option, _widget );
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
	return QPlastiqueStyle::sizeFromContents( _type, _option, _size, _widget );
}



QRect CusisStyle::subControlRect( ComplexControl _control,
		const QStyleOptionComplex *_option, SubControl _subControl,
		const QWidget *_widget ) const
{
	QRect rect =
		QPlastiqueStyle::subControlRect( _control, _option, _subControl, _widget );

	switch( _control )
	{
	case CC_ScrollBar:
		if( const QStyleOptionSlider * scrollBar =
				qstyleoption_cast<const QStyleOptionSlider *> (_option ) )
		{
			int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, scrollBar, _widget );
			int sliderMaxLength = (
					( scrollBar->orientation == Qt::Horizontal ) ?
							scrollBar->rect.width() :
							scrollBar->rect.height() ) -
					( BUTTON_LENGTH * 2 + 6 );
			int sliderMinLength = pixelMetric( PM_ScrollBarSliderMin, scrollBar, _widget );
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

			switch( _subControl )
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



QImage CusisStyle::colorizeXpm( const char * const * _xpm,
		const QBrush & _fill ) const
{
	QImage arrowXpm( _xpm );
	QImage arrow( arrowXpm.size(), QImage::Format_ARGB32 );
	QPainter arrowPainter( &arrow );
	arrowPainter.fillRect( arrow.rect(), _fill );
	arrowPainter.end();
	arrow.setAlphaChannel( arrowXpm );

	return arrow;
}



void CusisStyle::drawControl( ControlElement _element, const QStyleOption * _option,
    QPainter * _painter, const QWidget * _widget ) const
{

	switch( _element )
	{
	case CE_ScrollBarAddLine:
		if(const QStyleOptionSlider * scrollBar =
				qstyleoption_cast<const QStyleOptionSlider *>(_option))
		{

			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QString pixmapName = getCacheKey(
					QLatin1String( "ScrollBarAddLine" ), _option, _option->rect.size() );

			QPixmap cache;
			if( !QPixmapCache::find( pixmapName, cache ) )
			{
				cache = QPixmap( _option->rect.size() );
				QPainter cachePainter( &cache );

				cache.fill( QColor( 48, 48, 48 ) );
				QColor sliderColor;
				QColor blurColor;
				hoverColors(sunken, hover, 
						scrollBar->activeSubControls & SC_ScrollBarAddLine && isEnabled,
						sliderColor, blurColor);

				int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, _option, _widget );
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

					QImage arrow = colorizeXpm( s_scrollbarArrowRightXpm,
					    _option->palette.foreground().color() );
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

					QImage arrow = colorizeXpm( s_scrollbarArrowDownXpm,
					    _option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarAddLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				QPixmapCache::insert( pixmapName, cache );
			}
			_painter->drawPixmap( _option->rect.topLeft(), cache );

		}
		break;

	case CE_ScrollBarSubLine:
		if(const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(_option))
		{
			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QString pixmapName = getCacheKey( QLatin1String( "ScrollBarSubLine" ),
					_option, _option->rect.size() );
			QPixmap cache;
			if( !QPixmapCache::find( pixmapName, cache ) )
			{
				cache = QPixmap( _option->rect.size() );
				QPainter cachePainter( &cache );

				cache.fill( QColor( 48, 48, 48 ) ); // TODO: Fill with CSS background
				QColor sliderColor;
				QColor blurColor;
				hoverColors(sunken, hover,
						scrollBar->activeSubControls & SC_ScrollBarSubLine && isEnabled,
						sliderColor, blurColor);

				int scrollBarExtent = pixelMetric( PM_ScrollBarExtent, _option, _widget );
				cachePainter.setPen( QPen( sliderColor, 0 ) );
				if( horizontal )
				{
					cachePainter.fillRect( 2, 2, BUTTON_LENGTH - 2,
							scrollBarExtent - 4, sliderColor );
					cachePainter.drawLine( 0, 4, 0, scrollBarExtent - 5 );
					cachePainter.drawLine( 1, 3, 1, scrollBarExtent - 4 );

					const QPointF points[4] = {
							QPoint( 0, 3 ), QPoint( 0, scrollBarExtent - 4 ),
							QPoint( 1, 2 ), QPoint( 1, scrollBarExtent - 3 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowLeftXpm,
							_option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarSubLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				else
				{
					cachePainter.fillRect( 2, 2, scrollBarExtent - 4,
							BUTTON_LENGTH - 2, sliderColor );
					cachePainter.drawLine( 4, 0, scrollBarExtent - 5, 0 );
					cachePainter.drawLine( 3, 1, scrollBarExtent - 4, 1 );

					const QPointF points[4] = {
							QPoint( 3, 0 ), QPoint( scrollBarExtent - 4, 0 ),
							QPoint( 2, 1 ), QPoint( scrollBarExtent - 3, 1 )};

					cachePainter.setPen( QPen( blurColor, 0 ) );
					cachePainter.drawPoints( points, 4 );

					QImage arrow = colorizeXpm( s_scrollbarArrowUpXpm,
					    _option->palette.foreground().color() );
					if( ( scrollBar->activeSubControls & SC_ScrollBarSubLine ) && sunken )
					{
						cachePainter.translate( 1, 1 );
					}
					cachePainter.drawImage( cache.rect().center() - QPoint( 3, 3 ), arrow );
				}
				QPixmapCache::insert( pixmapName, cache );
			}
			_painter->drawPixmap( _option->rect.topLeft(), cache );

		}
		break;

	case CE_ScrollBarSubPage:
	case CE_ScrollBarAddPage:
		break;

	case CE_ScrollBarSlider:
		if(const QStyleOptionSlider *scrollBar = qstyleoption_cast<const QStyleOptionSlider *>(_option))
		{
			bool horizontal = scrollBar->orientation == Qt::Horizontal;
			bool isEnabled = scrollBar->state & State_Enabled;
			bool sunken = scrollBar->state & State_Sunken;
			bool hover = scrollBar->state & State_MouseOver;

			QColor sliderColor, blendColor;
			hoverColors(sunken, hover,
					(scrollBar->activeSubControls & SC_ScrollBarSlider) && isEnabled,
					sliderColor, blendColor);

			QColor background = QColor( 48, 48, 48 );
			QRect rc = scrollBar->rect;
			if( horizontal )
			{
				_painter->fillRect( rc.left(), rc.top() + 2,
						rc.width(), rc.height() - 4, sliderColor );
			}
			else
			{
				_painter->fillRect( rc.left() + 2, rc.top(),
						rc.width() - 4, rc.height(), sliderColor );
			}
		}
		break;

	default:
		QPlastiqueStyle::drawControl( _element, _option, _painter, _widget );
		break;
	}
}



void CusisStyle::drawPrimitive( PrimitiveElement _element,
		const QStyleOption * _option, QPainter * _painter,
		const QWidget * _widget ) const
{
	if( _element == QStyle::PE_Frame ||
			_element == QStyle::PE_FrameLineEdit ||
			_element == QStyle::PE_PanelLineEdit )
	{
		const QRect rect = _option->rect;

		QColor bright = QColor( 48, 48, 48 );
		QColor outer = QColor( 29, 29, 29 );
		QColor outerDot = QColor( 27, 27, 27 );
		QColor innerDot = QColor( 41, 41, 41 );

		QLine lines[4];
		QPoint points[4];

		// inside lines
		// 50%
		_painter->setPen( QPen( bright, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top() + 1, rect.right() - 2, rect.top() + 1 );
		lines[1] = QLine( rect.left() + 2, rect.bottom() - 1, rect.right() - 2, rect.bottom()
		    - 1 );
		lines[2]
		    = QLine( rect.left() + 1, rect.top() + 2, rect.left() + 1, rect.bottom() - 2 );
		lines[3] = QLine( rect.right() - 1, rect.top() + 2, rect.right() - 1, rect.bottom()
		    - 2 );
		_painter->drawLines( lines, 4 );

		// black inside dots
		_painter->setPen( QPen( innerDot, 0 ) );
		points[0] = QPoint( rect.left() + 2, rect.top() + 2 );
		points[1] = QPoint( rect.left() + 2, rect.bottom() - 2 );
		points[2] = QPoint( rect.right() - 2, rect.top() + 2 );
		points[3] = QPoint( rect.right() - 2, rect.bottom() - 2 );
		_painter->drawPoints( points, 4 );

		// outside lines
		// 100%
		_painter->setPen( QPen( outer, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top(), rect.right() - 2, rect.top() );
		lines[1] = QLine( rect.left(), rect.top() + 2, rect.left(), rect.bottom() - 2 );
		lines[2] = QLine( rect.left() + 2, rect.bottom(), rect.right() - 2, rect.bottom() );
		lines[3] = QLine( rect.right(), rect.top() + 2, rect.right(), rect.bottom() - 2 );
		_painter->drawLines( lines, 4 );

		// outside corner dots
		// 75%
		_painter->setPen( QPen( outerDot, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.top() + 1 );
		points[1] = QPoint( rect.right() - 1, rect.top() + 1 );
		points[2] = QPoint( rect.left() + 1, rect.bottom() - 1 );
		points[3] = QPoint( rect.right() - 1, rect.bottom() - 1 );
		_painter->drawPoints( points, 4 );

		// outside end dots
		points[0] = QPoint( rect.left() + 1, rect.top() );
		points[1] = QPoint( rect.left(), rect.top() + 1 );
		points[2] = QPoint( rect.right() - 1, rect.top() );
		points[3] = QPoint( rect.left(), rect.bottom() - 1 );
		_painter->drawPoints( points, 4 );

		// outside end dots - highlight
		points[0] = QPoint( rect.right() - 1, rect.bottom() );
		points[1] = QPoint( rect.right(), rect.bottom() - 1 );
		points[2] = QPoint( rect.left() + 1, rect.bottom() );
		points[3] = QPoint( rect.right(), rect.top() + 1 );
		_painter->drawPoints( points, 4 );
	}
	else
	{
		QPlastiqueStyle::drawPrimitive( _element, _option, _painter, _widget );
	}

}


void CusisStyle::polish( QWidget * widget )
{
	if( qobject_cast<QScrollBar *>(widget) )
	{
		widget->setAttribute(Qt::WA_Hover, true);
	}
}



void CusisStyle::unpolish(QWidget *widget)
{
	if( qobject_cast<QScrollBar *>(widget) )
	{
		widget->setAttribute(Qt::WA_Hover, false);
	}
}



int CusisStyle::pixelMetric( PixelMetric _metric, const QStyleOption * _option,
    const QWidget * _widget ) const
{
	switch( _metric )
	{

	case QStyle::PM_ScrollBarExtent:
		return 18;
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
		return QPlastiqueStyle::pixelMetric( _metric, _option, _widget );
	}
}



void CusisStyle::drawFxLine( QPainter * _painter, const QWidget *_fxLine,
    const QString & _name, bool _active )
{
	int width = _fxLine->rect().width();
	int height = _fxLine->rect().height();

	QPainter * p = _painter;

	QLinearGradient grad( 0, 0, 0, height );
	grad.setColorAt( 0, QColor( 32, 32, 32 ) );
	grad.setColorAt( 1, QColor( 6, 6, 6 ) );
	p->setBrush( grad );
	p->setPen( Qt::NoPen );
	p->drawRect( _fxLine->rect() );

	p->rotate( -90 );
	p->setPen( _active ? QColor( 0, 255, 0 ) : Qt::white );
	p->setFont( pointSizeF( _fxLine->font(), 7.5f ) );
	p->drawText( -90, 20, _name );
}



void CusisStyle::drawTrackContentBackground( QPainter * _painter, const QSize & _size,
    const int _pixelsPerTact )
{
	const int w = _size.width();
	const int h = _size.height();

	_painter->fillRect( 0, 0, w, h, QColor( 34, 34, 34 ) );
	_painter->fillRect( w, 0, w, h, QColor( 21, 21, 21 ) );

	// draw vertical lines
	_painter->setPen( QPen( QColor( 53, 53, 53 ), 1 ) );
	for( float x = 0; x < w * 2; x += _pixelsPerTact )
	{
		_painter->drawLine( QLine( x, 0, x, h ) );
	}
	_painter->drawLine( 0, h - 1, w * 2, h - 1 );
}



void CusisStyle::drawTrackContentObject( QPainter * _painter,
		const trackContentObject * _model, const LmmsStyleOptionTCO * _option )
{
	QString pixmapName = getTcoCacheKey( "tco", _option );
	QPixmap cache;

	if( !QPixmapCache::find( pixmapName, cache ) )
	{
		printf("Creating pixmap\n");
		const QRectF & rc = _option->rect;

		// TODO: Consider matrix m11 and m22 for scaling the pixmap
		cache = QPixmap( rc.width(), rc.height() );
		cache.fill( Qt::transparent );
		QPainter painter( &cache );

		QColor col = _option->userColor;
		QColor colBorder;
		QColor col0;
		if( _option->type == LmmsStyleOptionTCO::BbTco )
		{
			colBorder = col;
			col0 = _option->selected ?
				QColor( 0x33, 0x00, 0x99 ) :
				col.darker(140);
		}
		else
		{
			colBorder = QColor( 0x00, 0x33, 0x99 );
			col0 = _option->selected ?
				QColor( 0x33, 0x00, 0x99 ).darker(180) :
				colBorder.darker(420);
		}

		painter.setRenderHint( QPainter::Antialiasing, true );

		QPainterPath path;
		path.addRoundedRect( 2, 2, rc.width()-4, rc.height()-4, 4, 4 );
		drawPath( &painter, path, col0, colBorder, _option->hovered );

		const float cellW = TrackContainerScene::DEFAULT_CELL_WIDTH;
		const tick_t t = _option->duration;

		if( _model->length() > midiTime::ticksPerTact() && t > 0 )
		{
			painter.setOpacity(0.2);
			painter.setRenderHint( QPainter::Antialiasing, false );
			painter.setPen( QColor(0, 0, 0) );

			for( float x = t * cellW; x < rc.width()-2; x += t * cellW )
			{
				painter.drawLine(x, 3, x, rc.height()-5);
			}

			painter.translate( 1, 0 );
			QColor faint = QColor::fromHsv( 
					colBorder.hue(),
					colBorder.saturation()/2,
					colBorder.value() );

			painter.setPen( faint );
			for( float x = t * cellW; x < rc.width()-2; x += t * cellW )
			{
				painter.drawLine(x, 2, x, rc.height()-5);
			}
		}

		QPixmapCache::insert( pixmapName, cache );
	}
	_painter->drawPixmap( 0, 0, cache );		
}



QColor CusisStyle::color( LmmsStyle::ColorRole _role ) const
{
	return m_colors[_role];
}
