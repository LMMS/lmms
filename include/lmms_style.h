/*
 * lmms_style.h - the graphical style used by LMMS to create a consistent
 *                interface
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


#ifndef _LMMS_STYLE_H
#define _LMMS_STYLE_H


// Defines for widgets
#include "fx_mixer_view.h"


class LmmsStyle
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

  /* TODO: Still need to style:
   * combobox.cpp
   * fade_button.cpp
   * knob.cpp					- Will be reimplemented to use image-strips
   * tab_widget.cpp
   * tool_button.cpp	- Will be obsoleted in favor of real Qt toolbars
   */


	LmmsStyle()
    {
    }

	virtual ~LmmsStyle()
	{
	}


    virtual void drawFxLine(QPainter * _painter, const QWidget *_fxLine,
            const QString & _name, bool _active) = 0;

    virtual void drawTrackContentBackground(QPainter * _painter,
            const QSize & _size, const int _pixelsPerTact) = 0;

    virtual QColor color(ColorRole _role) const = 0;

/*
	virtual QPalette standardPalette( void ) const;

	virtual void drawComplexControl(
				ComplexControl control,
				const QStyleOptionComplex * option,
					QPainter *painter,
						const QWidget *widget ) const;
	virtual void drawPrimitive( PrimitiveElement element,
					const QStyleOption *option,
					QPainter *painter,
					const QWidget *widget = 0 ) const;

    virtual void drawControl( ControlElement element, const QStyleOption * option,
        QPainter * painter, const QWidget * widget ) const;

	virtual int pixelMetric( PixelMetric metric,
					const QStyleOption * option = 0,
					const QWidget * widget = 0 ) const;

    QSize sizeFromContents(ContentsType type, const QStyleOption *option,
                                            const QSize &size, const QWidget *widget) const;
    QRect subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                                          SubControl subControl, const QWidget *widget) const;


private:
    QImage colorizeXpm( const char * const * xpm, const QBrush & fill ) const;
*/
} ;

#endif
