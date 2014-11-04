/*
 * LmmsPalette.h - dummy class for fetching palette qproperties from CSS
 *                
 *
 * Copyright (c) 2007-2014 Vesa Kivimäki <contact/dot/diizy/at/nbl/dot/fi>
 *
 * This file is part of LMMS - http://lmms.io
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

#include <QtGui/QWidget>
#include "export.h"

#ifndef LMMSPALETTE_H
#define LMMSPALETTE_H


class EXPORT LmmsPalette : public QWidget
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
	Q_PROPERTY( QColor toolTipText READ toolTipText WRITE setToolTipText )
	Q_PROPERTY( QColor toolTipBase READ toolTipBase WRITE setToolTipBase )

public:
	LmmsPalette( QWidget * parent, QStyle * stylearg  ); 
	virtual ~LmmsPalette();

#define ACCESSMET( read, write ) \
	QColor read () const; \
	void write ( const QColor & c ); \


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
	ACCESSMET( toolTipText, setToolTipText )
	ACCESSMET( toolTipBase, setToolTipBase )

#undef ACCESSMET

	QPalette palette() const;

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
	QColor m_toolTipText;
	QColor m_toolTipBase;
};




#endif
