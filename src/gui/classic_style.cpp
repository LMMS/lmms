/*
 * lmms_style.cpp - the graphical style used by LMMS to create a consistent
 *                  interface
 *
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QFile>
#include <QtGui/QApplication>
#include <QtGui/QFrame>
#include <QtGui/QPainter>
#include <QtGui/QStyleOption>

#include "lmms_style.h"
#include "classic_style.h"
#include "gui_templates.h"


ClassicStyle::ClassicStyle() :
	QPlastiqueStyle(), LmmsStyle()
{
	QFile file( "resources:style.css" );
	file.open( QIODevice::ReadOnly );
	qApp->setStyleSheet( file.readAll() );

	qApp->setPalette( standardPalette() );

	colors[AutomationBarFill] = QColor( 0xFF, 0xB0, 0x00 );
	colors[AutomationBarValue] = QColor( 0xFF, 0xDF, 0x20 );
	colors[AutomationSelectedBarFill] = QColor( 0x00, 0x40, 0xC0 );
	colors[AutomationCrosshair] = QColor( 0xFF, 0x33, 0x33 );
	colors[PianoRollDefaultNote] = QColor( 0x00, 0xAA, 0x00 ); // 0.3.x: QColor( 0x99, 0xff, 0x00 )
	colors[PianoRollFrozenNote] = QColor( 0x00, 0xE0, 0xFF );
	colors[PianoRollMutedNote] = QColor( 160, 160, 160 );
	colors[PianoRollStepNote] = QColor( 0x00, 0xFF, 0x00 );
	colors[PianoRollSelectedNote] = QColor( 0x00, 0x40, 0xC0 );
	colors[PianoRollEditHandle] = QColor( 0xEA, 0xA1, 0x00 );
	colors[PianoRollSelectedLevel] = QColor( 0x00, 0x40, 0xC0 );
	colors[PianoRollVolumeLevel] = QColor( 0x00, 0xFF, 0x00 );
	colors[PianoRollPanningLevel] = QColor( 0xFF, 0xB0, 0x00 );

	colors[TimelineForecolor] = QColor( 192, 192, 192 );

	colors[StandardGraphLine] = QColor( 96, 255, 128 );
	colors[StandardGraphHandle] = QColor( 0xFF, 0xBF, 0x22 );
	colors[StandardGraphHandleBorder] = QColor( 0x00, 0x00, 0x02 );
	colors[StandardGraphCrosshair] = QColor( 0xAA, 0xFF, 0x00, 0x70 );

	colors[TextFloatForecolor] = QColor( 0, 0, 0 );
	colors[TextFloatFill] = QColor( 224, 224, 224 );

	colors[VisualizationLevelLow] = QColor( 128, 224, 128 );
	colors[VisualizationLevelMid] = QColor( 255, 192, 64 );
	colors[VisualizationLevelPeak] = QColor( 255, 64, 64 );
}



QColor ClassicStyle::color( LmmsStyle::ColorRole _role ) const
{
	return colors[_role];
}



QPalette ClassicStyle::standardPalette( void ) const
{
	QPalette pal = QPlastiqueStyle::standardPalette();
	pal.setColor( QPalette::Background, QColor( 72, 76, 88 ) );
	pal.setColor( QPalette::WindowText, QColor( 240, 240, 240 ) );
	pal.setColor( QPalette::Base, QColor( 128, 128, 128 ) );
	pal.setColor( QPalette::Text, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::Button, QColor( 172, 176, 188 ) );
	pal.setColor( QPalette::Shadow, QColor( 0, 0, 0 ) );
	pal.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );
	pal.setColor( QPalette::BrightText, QColor( 0, 255, 0 ) );
	pal.setColor( QPalette::Highlight, QColor( 224, 224, 224 ) );
	pal.setColor( QPalette::HighlightedText, QColor( 0, 0, 0 ) );
	return( pal );
}




void ClassicStyle::drawComplexControl( ComplexControl control,
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
	QPlastiqueStyle::drawComplexControl( control, option, painter, widget );
}




void ClassicStyle::drawPrimitive( PrimitiveElement element,
				const QStyleOption *option,
				QPainter *painter,
				const QWidget *widget ) const
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
		black.setAlpha( a100 );
		painter->setPen( QPen( black, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top() + 1,
					rect.right() - 2, rect.top() + 1 );
		lines[1] = QLine( rect.left() + 2, rect.bottom() - 1,
					rect.right() - 2, rect.bottom() - 1 );
		lines[2] = QLine( rect.left() + 1, rect.top() + 2,
					rect.left() + 1, rect.bottom() - 2 );
		lines[3] = QLine( rect.right() - 1, rect.top() + 2,
					rect.right() - 1, rect.bottom() - 2 );
		painter->drawLines( lines, 4 );

		// black inside dots
		black.setAlpha( a50 );
		painter->setPen( QPen( black, 0 ) );
		points[0] = QPoint( rect.left() + 2, rect.top() + 2 );
		points[1] = QPoint( rect.left() + 2, rect.bottom() - 2 );
		points[2] = QPoint( rect.right() - 2, rect.top() + 2 );
		points[3] = QPoint( rect.right() - 2, rect.bottom() - 2 );
		painter->drawPoints( points, 4 );


		// outside lines - shadow
		// 100%
		shadow.setAlpha( a75 );
		painter->setPen( QPen( shadow, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.top(),
						rect.right() - 2, rect.top() );
		lines[1] = QLine( rect.left(), rect.top() + 2,
						rect.left(), rect.bottom() - 2 );
		painter->drawLines( lines, 2 );

		// outside corner dots - shadow
		// 75%
		shadow.setAlpha( a50 );
		painter->setPen( QPen( shadow, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.top() + 1 );
		points[1] = QPoint( rect.right() - 1, rect.top() + 1 );
		painter->drawPoints( points, 2 );

		// outside end dots - shadow
		// 50%
		shadow.setAlpha( a25 );
		painter->setPen( QPen( shadow, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.top() );
		points[1] = QPoint( rect.left(), rect.top() + 1 );
		points[2] = QPoint( rect.right() - 1, rect.top() );
		points[3] = QPoint( rect.left(), rect.bottom() - 1 );
		painter->drawPoints( points, 4 );


		// outside lines - highlight
		// 100%
		highlight.setAlpha( a75 );
		painter->setPen( QPen( highlight, 0 ) );
		lines[0] = QLine( rect.left() + 2, rect.bottom(),
					rect.right() - 2, rect.bottom() );
		lines[1] = QLine( rect.right(), rect.top() + 2,
					rect.right(), rect.bottom() - 2 );
		painter->drawLines( lines, 2 );

		// outside corner dots - highlight
		// 75%
		highlight.setAlpha( a50 );
		painter->setPen( QPen(highlight, 0 ) );
		points[0] = QPoint( rect.left() + 1, rect.bottom() - 1 );
		points[1] = QPoint( rect.right() - 1, rect.bottom() - 1 );
		painter->drawPoints( points, 2 );

		// outside end dots - highlight
		// 50%
		highlight.setAlpha( a25 );
		painter->setPen( QPen(highlight, 0 ) );
		points[0] = QPoint( rect.right() - 1, rect.bottom() );
		points[1] = QPoint( rect.right(), rect.bottom() - 1 );
		points[2] = QPoint( rect.left() + 1, rect.bottom() );
		points[3] = QPoint( rect.right(), rect.top() + 1 );
		painter->drawPoints( points, 4 );
	}
	else
	{
		QPlastiqueStyle::drawPrimitive( element, option, painter, widget );
	}

}


int ClassicStyle::pixelMetric( PixelMetric _metric,
				const QStyleOption * _option,
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
			return QPlastiqueStyle::pixelMetric(
					_metric, _option, _widget );
	}
}


void ClassicStyle::drawFxLine( QPainter * _painter, const QWidget *_fxLine,
				const QString & _name, bool _active )
{
	int width = _fxLine->rect().width();
	int height = _fxLine->rect().height();

	QPainter * p = _painter;
	p->fillRect( _fxLine->rect(), QColor( 72, 76, 88 ) );
	p->setPen( QColor( 40, 42, 48 ) );
	p->drawRect( 0, 0, width-2, height-2 );
	p->setPen( QColor( 108, 114, 132 ) );
	p->drawRect( 1, 1, width-2, height-2 );
	p->setPen( QColor( 20, 24, 32 ) );
	p->drawRect( 0, 0, width-1, height-1 );

	p->rotate( -90 );
	p->setPen( _active ? QColor( 0, 255, 0 ) : Qt::white );
	p->setFont( pointSizeF( _fxLine->font(), 7.5f ) );
	p->drawText( -90, 20, _name );
}

void ClassicStyle::drawTrackContentBackground(QPainter * _painter,
        const QSize & _size, const int _pixelsPerTact)
{
    const int w = _size.width();
    const int h = _size.height();

    QLinearGradient grad( 0, 1, 0, h-2 );
    _painter->fillRect( 0, 0, 1, h, QColor( 96, 96, 96 ) );
    _painter->fillRect( 1, 0, w+1, h, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.0, QColor( 64, 64, 64 ) );
    grad.setColorAt( 0.3, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.5, QColor( 128, 128, 128 ) );
    grad.setColorAt( 0.95, QColor( 160, 160, 160 ) );
    _painter->fillRect( 0, 1, w, h-2, grad );

    QLinearGradient grad2( 0,1, 0, h-2 );
    _painter->fillRect( w+1, 0, w , h, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.0, QColor( 48, 48, 48 ) );
    grad2.setColorAt( 0.3, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.5, QColor( 96, 96, 96 ) );
    grad2.setColorAt( 0.95, QColor( 120, 120, 120 ) );
    _painter->fillRect( w, 1, w , h-2, grad2 );

    // draw vertical lines
    _painter->setPen( QPen( QColor( 0, 0, 0, 112 ), 1 ) );
    for( float x = 0.5; x < w * 2; x += _pixelsPerTact )
    {
        _painter->drawLine( QLineF( x, 1.0, x, h-2.0 ) );
    }
    _painter->drawLine( 0, 1, w*2, 1 );

    _painter->setPen( QPen( QColor( 255, 255, 255, 32 ), 1 ) );
    for( float x = 1.5; x < w * 2; x += _pixelsPerTact )
    {
        _painter->drawLine( QLineF( x, 1.0, x, h-2.0 ) );
    }
    _painter->drawLine( 0, h-2, w*2, h-2 );
}
