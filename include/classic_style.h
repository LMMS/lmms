/*
 * classic_style.h - the graphical style used by LMMS for original look&feel
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


#ifndef _CLASSIC_STYLE_H
#define _CLASSIC_STYLE_H

#include <QtGui/QPlastiqueStyle>
#include "lmms_style.h"


class ClassicStyle : public QPlastiqueStyle, public LmmsStyle
{
public:
	ClassicStyle();
	virtual ~ClassicStyle()
	{
	}

    // QT stuff

	virtual QPalette standardPalette() const;

	virtual void drawComplexControl(
				ComplexControl control,
				const QStyleOptionComplex * option,
					QPainter *painter,
						const QWidget *widget ) const;
	virtual void drawPrimitive( PrimitiveElement element,
					const QStyleOption *option,
					QPainter *painter,
					const QWidget *widget = 0 ) const;

	virtual int pixelMetric( PixelMetric metric,
					const QStyleOption * option = 0,
					const QWidget * widget = 0 ) const;

// LMMS Stuff

	virtual void drawFxLine(QPainter * _painter, const QWidget *_fxLine,
			const QString & _name, bool _active);

	virtual void drawTrackContentBackground(QPainter * _painter,
			const QSize & _size, const int _pixelsPerTact);

	virtual QColor color(LmmsStyle::ColorRole _role) const;

	virtual void drawTrackContentObject( QPainter * _painter, const trackContentObject * _model,
			const LmmsStyleOptionTCO * _options );

	private:
	QImage colorizeXpm( const char * const * xpm, const QBrush & fill ) const;

	QColor colors[LmmsStyle::NumColorRoles];

};

#endif
