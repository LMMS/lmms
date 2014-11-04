/*
 * tool_button.h - declaration of class toolButton 
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _TOOL_BUTTON_H
#define _TOOL_BUTTON_H 

#include <QtGui/QToolButton>
#include <QtGui/QColor>


class toolButton : public QToolButton
{
	Q_OBJECT
public:
	toolButton( const QPixmap & _pixmap, const QString & _tooltip,
			QObject * _receiver, const char * _slot,
			QWidget * _parent );

	inline toolButton( QWidget * _parent ) :
		QToolButton( _parent ),
		m_colorStandard( s_stdColor ),
		m_colorHighlighted( s_hlColor )
	{
		// setup colors
		leaveEvent( NULL );
	}

	virtual ~toolButton();

	inline void setStandardColor( const QColor & _color )
	{
		m_colorStandard = _color;
	}

	inline void setHighlightedColor( const QColor & _color )
	{
		m_colorHighlighted = _color;
	}


protected:
	virtual void enterEvent( QEvent * _ev );
	virtual void leaveEvent( QEvent * _ev );


private slots:
	void toggledBool( bool _on );


private:
	static const QColor s_stdColor;
	static const QColor s_hlColor;

	QColor m_colorStandard;
	QColor m_colorHighlighted;

} ;

#endif

