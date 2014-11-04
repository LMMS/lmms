/*
 * gui_templates.h - GUI-specific templates
 *
 * Copyright (c) 2005-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _GUI_TEMPLATES_H
#define _GUI_TEMPLATES_H

#include "lmmsconfig.h"

#include <QtGui/QApplication>
#include <QtGui/QFont>
#include <QtGui/QDesktopWidget>



// return DPI-independent font-size - font with returned font-size has always
// the same size in pixels
template<int SIZE>
inline QFont pointSize( QFont _f )
{
	static const float DPI = 96;
#ifdef LMMS_BUILD_WIN32
	_f.setPointSizeF( ((float) SIZE+0.5f) * DPI /
			QApplication::desktop()->logicalDpiY() );
#else
	_f.setPointSizeF( (float) SIZE * DPI /
			QApplication::desktop()->logicalDpiY() );
#endif
	return( _f );
}


inline QFont pointSizeF( QFont _f, float SIZE )
{
	static const float DPI = 96;
#ifdef LMMS_BUILD_WIN32
	_f.setPointSizeF( (SIZE+0.5f) * DPI /
			QApplication::desktop()->logicalDpiY() );
#else
	_f.setPointSizeF( SIZE * DPI /
			QApplication::desktop()->logicalDpiY() );
#endif
	return( _f );
}


#endif
