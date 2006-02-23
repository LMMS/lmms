#ifndef SINGLE_SOURCE_COMPILE

/*
 * tool_button.cpp - implementation of LMMS-tool-button for common (cool) look
 *
 * Copyright (c) 2005-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "tool_button.h"
#include "tooltip.h"



const QColor toolButton::s_stdColor = QColor( 216, 216, 216 );
const QColor toolButton::s_hlColor = QColor( 240, 240, 240 );



toolButton::toolButton( const QPixmap & _pixmap, const QString & _tooltip,
			QObject * _receiver, const char * _slot,
			QWidget * _parent ) :
	QToolButton( _parent ),
	m_colorStandard( s_stdColor ),
	m_colorHighlighted( s_hlColor )
{
	if( _receiver != NULL && _slot != NULL )
	{
		connect( this, SIGNAL( clicked() ), _receiver, _slot );
	}
	toolTip::add( this, _tooltip );
	setFixedSize( 30, 30 );
	setIcon( _pixmap );
	leaveEvent( NULL );
	connect( this, SIGNAL( toggled( bool ) ), this,
						SLOT( toggledBool( bool ) ) );
}




toolButton::~toolButton()
{
}




void toolButton::enterEvent( QEvent * )
{
#ifdef QT4
	QPalette pal;
	pal.setColor( backgroundRole(), m_colorHighlighted );
	setPalette( pal );
#else
	setPaletteBackgroundColor( m_colorHighlighted );
#endif
}




void toolButton::leaveEvent( QEvent * )
{
#ifdef QT4
	QPalette pal;
	pal.setColor( backgroundRole(), m_colorStandard );
	setPalette( pal );
#else
	setPaletteBackgroundColor( m_colorStandard );
#endif
}




void toolButton::toggledBool( bool _on )
{
	if( _on == TRUE )
	{
		emit( clicked() );
	}
}



#include "tool_button.moc"


#endif
