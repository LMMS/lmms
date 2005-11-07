/*
 * gui_templates.h - GUI-specific templates
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _GUI_TEMPLATES_H
#define _GUI_TEMPLATES_H

#include "qt3support.h"

#ifdef QT4

#include <QPaintDevice>
#include <QApplication>
#include <QFont>
#include <QDesktopWidget>

#else

#include <qpaintdevicemetrics.h>
#include <qfont.h>
#include <qapplication.h>
#include <qdesktopwidget.h>

#endif


// return DPI-independent font-size - font with returned font-size has always
// the same size in pixels
template<int SIZE>
inline QFont pointSize( QFont _f )
{
	static const int DPI = 96;
#ifdef QT4
	_f.setPointSizeF( SIZE * DPI /
			QPaintDevice( QApplication::desktop() ).logicalDpiY() );
#else
	_f.setPointSizeFloat( SIZE * DPI /
			QPaintDeviceMetrics( qApp->desktop() ).logicalDpiY() );
#endif
	return( _f );
}


#endif
