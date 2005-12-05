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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "qt3support.h"

#ifdef QT4

#include <QPushButton>

#else

#include <qpushbutton.h>

#endif

#include "tooltip.h"


class toolButton : public QPushButton
{
public:
	toolButton( const QPixmap & _pixmap, const QString & _tooltip,
			QObject * _receiver, const char * _slot,
			QWidget * _parent ) :
		QPushButton( _parent )
	{
		connect( this, SIGNAL( clicked() ), _receiver, _slot );
		toolTip::add( this, _tooltip );
		setPaletteBackgroundColor( QColor( 224, 224, 224 ) );
		setFixedSize( 30, 30 );
		setPixmap( _pixmap );
	}

	~toolButton()
	{
	}

} ;

#endif
