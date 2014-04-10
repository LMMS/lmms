/*
 * LmmsStyle.h - the graphical style used by LMMS to create a consistent
 *                interface
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef LMMS_STYLE_H
#define LMMS_STYLE_H

#include <QtGui/QPlastiqueStyle>

#define ACCESSMET( read, write ) \
	QColor read () \
	{	return m_##read ; } \
	void write ( const QColor & _c ) \
	{	m_##read = QColor( _c ); }

class LmmsPalette: public QWidget
{
	Q_OBJECT
	Q_PROPERTY( QColor background READ background WRITE setBackground )
	Q_PROPERTY( QColor windowText READ windowText WRITE setWindowText )
	Q_PROPERTY( QColor base READ base WRITE setBase )
	Q_PROPERTY( QColor text READ text WRITE setText )
	Q_PROPERTY( QColor button READ button WRITE setButton )
	Q_PROPERTY( QColor shadow READ shadow WRITE setShadow )
	Q_PROPERTY( QColor buttonText READ buttonText WRITE setButtonText )
	Q_PROPERTY( QColor brightText READ brightText WRITE setBrightText )
	Q_PROPERTY( QColor highlight READ highlight WRITE setHighlight )
	Q_PROPERTY( QColor highlightedText READ highlightedText WRITE setHighlightedText )

public:
	LmmsPalette( QWidget * _parent ); 
	virtual ~LmmsPalette() {};

	ACCESSMET( background, setBackground )
	ACCESSMET( windowText, setWindowText )
	ACCESSMET( base, setBase )
	ACCESSMET( text, setText )
	ACCESSMET( button, setButton )
	ACCESSMET( shadow, setShadow )
	ACCESSMET( buttonText, setButtonText )
	ACCESSMET( brightText, setBrightText )
	ACCESSMET( highlight, setHighlight )
	ACCESSMET( highlightedText, setHighlightedText )

private:
	QColor m_background;
	QColor m_windowText;
	QColor m_base;
	QColor m_text;
	QColor m_button;
	QColor m_shadow;
	QColor m_buttonText;
	QColor m_brightText;
	QColor m_highlight;
	QColor m_highlightedText;
};

class LmmsStyle : public QPlastiqueStyle
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

	virtual QPalette standardPalette( void ) const;

//	virtual void drawControl( ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget ) const;

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

//	QSize sizeFromContents( ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget ) const;
//	QRect subControlRect( ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget ) const;

private:
	QImage colorizeXpm( const char * const * xpm, const QBrush& fill ) const;
	void hoverColors( bool sunken, bool hover, bool active, QColor& color, QColor& blend ) const;
	QColor m_colors[ LmmsStyle::NumColorRoles ];

} ;

#endif
