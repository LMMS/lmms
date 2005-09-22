/*
 * embed.h - misc. stuff for using embedded data (resources linked into binary)
 *
 * Linux MultiMedia Studio
 * Copyright (c) 2004-2005 Tobias Doerffel <tobydox@users.sourceforge.net>
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


#ifndef _EMBED_H
#define _EMBED_H

#include "qt3support.h"

#ifdef QT4

#include <QPixmap>
#include <QString>

#else

#include <qpixmap.h>
#include <qstring.h>

#endif

namespace embed
{

QPixmap getIconPixmap( const char *  _name, int _w = -1, int _h = -1 );
QString getText( const char * _name );
void loadTranslation( const QString & _tname );

}

#endif
