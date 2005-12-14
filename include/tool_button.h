/*
 * tool_button.h - declaration of class toolButton 
 *
 * Copyright (c) 2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _TOOL_BUTTON_H
#define _TOOL_BUTTON_H 

#include "qt3support.h"

#ifdef QT4

#include <QToolButton>
#include <QColor>

#else

#include <qtoolbutton.h>
#include <qcolor.h>

#endif


class toolButton : public QToolButton
{
public:
	toolButton( const QPixmap & _pixmap, const QString & _tooltip,
			QObject * _receiver, const char * _slot,
			QWidget * _parent );

	inline toolButton( QWidget * _parent ) :
		QToolButton( _parent ),
		m_colorStandard( s_stdColor ),
		m_colorHighlighted( s_hlColor )
	{
	}

	~toolButton();

	inline void setStandardColor( const QColor & _color )
	{
		m_colorStandard = _color;
	}

	inline void setHighlightedColor( const QColor & _color )
	{
		m_colorHighlighted = _color;
	}
#ifndef QT4
	inline void setIcon( const QPixmap & _icon )
	{
		setIconSet( _icon );
	}
#endif

protected:
	virtual void enterEvent( QEvent * _ev );
	virtual void leaveEvent( QEvent * _ev );


private:
	static const QColor s_stdColor;
	static const QColor s_hlColor;

	QColor m_colorStandard;
	QColor m_colorHighlighted;

} ;

#endif

