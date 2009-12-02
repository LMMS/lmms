/*
 * cusis_style.h - the graphical style used by LMMS for new Cusis-inspired
 * theme
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


#ifndef _CUSIS_STYLE_H
#define _CUSIS_STYLE_H

#include <QtGui/QPlastiqueStyle>
#include "lmms_style.h"


class CusisStyle : public QPlastiqueStyle, public LmmsStyle
{
public:
	CusisStyle();
	virtual ~CusisStyle()
	{
	}

	virtual QPalette standardPalette() const;

	virtual void drawComplexControl( ComplexControl _control,
			const QStyleOptionComplex * _option, QPainter * _painter,
			const QWidget * _widget ) const;

	virtual void drawPrimitive( PrimitiveElement _element,
			const QStyleOption * _option, QPainter * _painter,
			const QWidget * _widget = 0 ) const;

	virtual void drawControl( ControlElement _element,
			const QStyleOption * _option, QPainter * _painter,
			const QWidget * _widget ) const;

	virtual int pixelMetric( PixelMetric _metric,
			const QStyleOption * _option = 0, const QWidget * _widget = 0 ) const;

	QSize sizeFromContents( ContentsType _type, const QStyleOption * _option,
			const QSize & _size, const QWidget * _widget ) const;

	QRect subControlRect( ComplexControl control, const QStyleOptionComplex *option,
			SubControl subControl, const QWidget *widget ) const;

	virtual void polish( QWidget * widget );
	virtual void unpolish( QWidget * widget );

	virtual void drawFxLine( QPainter * _painter, const QWidget *_fxLine,
			const QString & _name, bool _active, bool _sendToThis );

	virtual void drawTrackContentBackground( QPainter * _painter,
			const QSize & _size, const int _pixelsPerTact );

	virtual void drawTrackContentObject( QPainter * _painter, const trackContentObject * _model,
			const LmmsStyleOptionTCO * _options );

	virtual QColor color( LmmsStyle::ColorRole _role ) const;

private:
	QImage colorizeXpm( const char * const * _xpm, const QBrush & _fill ) const;
	void hoverColors( bool sunken, bool hover, bool active,
		QColor & color, QColor & blend ) const;
	QColor m_colors[ LmmsStyle::NumColorRoles ];

};

#endif
