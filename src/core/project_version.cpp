/*
 * project_version.cpp - compare versions in import upgrades
 *
 * Copyright (c) 2007 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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




#include "project_version.h"




int projectVersion::compare( const projectVersion & _v1,
						const projectVersion & _v2 )
{
	int n1, n2;

	// Major
	n1 = _v1.section( '.', 0, 0 ).toInt();
	n2 = _v2.section( '.', 0, 0 ).toInt();
	if( n1 != n2 )
	{
		return n1 - n2;
	}

	// Minor
	n1 = _v1.section( '.', 1, 1 ).toInt();
	n2 = _v2.section( '.', 1, 1 ).toInt();
	if( n1 != n2 )
	{
		return n1 - n2;
	}

	// Release
	n1 = _v1.section( '.', 2 ).section( '-', 0, 0 ).toInt();
	n2 = _v2.section( '.', 2 ).section( '-', 0, 0 ).toInt();
	if( n1 != n2 )
	{
		return n1 - n2;
	}

	// Build
	const QString b1 = _v1.section( '.', 2 ).section( '-', 1 );
	const QString b2 = _v2.section( '.', 2 ).section( '-', 1 );

	// make sure 0.x.y > 0.x.y-patch
	if( b1.isEmpty() )
	{
		return 1;
	}
	if( b2.isEmpty() )
	{
		return -1;
	}

	return QString::compare( b1, b2 );
}




