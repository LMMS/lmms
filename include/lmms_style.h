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

#include <QtCore/QRect>
#include <QtGui/QColor>

#include "lmms_basics.h"

class QPainter;
class QSize;
class QString;
class QWidget;

class LmmsStyleOptionTCO;
class trackContentObject;


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
			const QString & _name, bool _active, bool _sendToThis) = 0;

	virtual void drawTrackContentBackground(QPainter * _painter,
			const QSize & _size, const int _pixelsPerTact) = 0;

	virtual void drawTrackContentObject( QPainter * _painter, const trackContentObject * _model,
			const LmmsStyleOptionTCO * _options ) = 0;

	virtual QColor color(ColorRole _role) const = 0;

};



class LmmsStyleOptionTCO
{
	public:
		enum TcoTypes
		{
			BbTco,
			Pattern,
			NumTcoTypes
		};
		TcoTypes type;

		QRectF rect;
		bool selected;
		bool hovered;
		QColor userColor;
		tick_t duration;
};


#endif
