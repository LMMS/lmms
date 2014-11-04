/*
 * project_version.h - version compared in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _PROJECT_VERSION_H
#define _PROJECT_VERSION_H

#include <QtCore/QString>


class projectVersion : public QString
{
public:
	projectVersion( const QString & _s ) :
		QString( _s )
	{
	}

	static int compare( const projectVersion & _v1,
						const projectVersion & _v2 );

} ;




inline bool operator<( const projectVersion & _v1, const char * _str )
{
	return projectVersion::compare( _v1, projectVersion( _str ) ) < 0;
}




#endif
