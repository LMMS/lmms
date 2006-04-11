#ifndef SINGLE_SOURCE_COMPILE

/*
 * tooltip.cpp - namespace toolTip, a tooltip-wrapper for LMMS
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


#include "qt3support.h"

#ifdef QT4

#include <QtGui/QToolTip>

#else

#include <qtooltip.h>

#endif

#include "tooltip.h"
#include "config_mgr.h"


void toolTip::add( QWidget * _w, const QString & _txt )
{
	if( !configManager::inst()->value( "tooltips", "disabled" ).toInt() )
	{
#ifdef QT4
		//_w->setToolTip( _txt );
#else
		QToolTip::add( _w, _txt );
#endif
	}
}


#endif
