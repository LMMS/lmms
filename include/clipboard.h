/*
 * clipboard.h - the clipboard for patterns, notes etc.
 *
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _CLIPBOARD_H
#define _CLIPBOARD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "qt3support.h"

#ifdef QT4

#include <QMap>
#include <Qt/QtXml>

#else

#include <qmap.h>
#include <qdom.h>

#endif


class settings;

namespace clipboard
{
	typedef QMap<QString, QDomElement> map;
	extern map content;

	void FASTCALL copy( settings * _settings_object );
	const QDomElement * FASTCALL getContent( const QString & _node_name );
} ;

#endif
