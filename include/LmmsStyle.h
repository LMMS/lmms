/*
 * LmmsStyle.h - the graphical style used by LMMS to create a consistent
 *                interface
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


#ifndef LMMS_STYLE_H
#define LMMS_STYLE_H

#include <QProxyStyle>



class LmmsStyle : public QProxyStyle
{
public:
	enum ColorRole
	{
		AutomationBarFill,
		AutomationBarValue,
		AutomationSelectedBarFill,
		AutomationCrosshair,
		PianoRollStepNote,
		PianoRollSelectedNote,
		PianoRollDefaultNote,
		PianoRollFrozenNote,
		PianoRollMutedNote,
		PianoRollEditHandle,
		PianoRollVolumeLevel,
		PianoRollPanningLevel,
		PianoRollSelectedLevel,
		TimelineForecolor,
		StandardGraphLine,
		StandardGraphHandle,
		StandardGraphHandleBorder,
		StandardGraphCrosshair,
		TextFloatForecolor,
		TextFloatFill,
		VisualizationLevelLow,
		VisualizationLevelMid,
		VisualizationLevelPeak,
		NumColorRoles
	};

	LmmsStyle();
	virtual ~LmmsStyle()
	{
	}

	QPalette standardPalette( void ) const override;

	virtual void drawComplexControl(
				ComplexControl control,
				const QStyleOptionComplex * option,
					QPainter *painter,
						const QWidget *widget ) const override;
	virtual void drawPrimitive( PrimitiveElement element,
					const QStyleOption *option,
					QPainter *painter,
					const QWidget *widget = 0 ) const override;

	virtual int pixelMetric( PixelMetric metric,
					const QStyleOption * option = 0,
					const QWidget * widget = 0 ) const override;

	static QPalette * s_palette;

private:
	QImage colorizeXpm( const char * const * xpm, const QBrush& fill ) const;
	void hoverColors( bool sunken, bool hover, bool active, QColor& color, QColor& blend ) const;
	QColor m_colors[ LmmsStyle::NumColorRoles ];

};

#endif
