/*
 * level_object.h - declaration of class levelObject
 *
 * Copyright (c) 2006-2008 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#ifndef _LEVEL_OBJECT_H
#define _LEVEL_OBJECT_H

#include <QtCore/QString>


class levelObject
{
public:
	virtual ~levelObject()
	{
	}

	inline int minLevel( void ) const
	{
		return( m_minLevel );
	}

	inline int maxLevel( void ) const
	{
		return( m_maxLevel );
	}

	virtual void setLevel( int _level ) = 0;

	virtual QString levelToLabel( int _level ) const = 0;
	virtual int labelToLevel( QString _label ) = 0;

	virtual QString displayName( void ) const
	{
		return( NULL );
	}


protected:
	int m_minLevel;
	int m_maxLevel;

} ;


#endif

