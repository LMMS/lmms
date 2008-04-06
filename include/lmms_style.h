/*
 * lmms_style.h - the graphical style used my LMMS to create a consistent interface
 *
 * Copyright (c) 2004-2007 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _LMMS_STYLE_H
#define _LMMS_STYLE_H

#include <QtGui/QPlastiqueStyle>

class lmmsStyle : public QPlastiqueStyle
{
public:
	lmmsStyle() : 
		QPlastiqueStyle ()	
	{
	}

	virtual ~lmmsStyle() 
	{
	}

	virtual void drawPrimitive( PrimitiveElement element, const QStyleOption *option,
			QPainter *painter, const QWidget *widget = 0 ) const;

	virtual int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, 
			const QWidget * widget = 0 ) const;

};

#endif
