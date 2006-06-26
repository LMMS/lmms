/*
 * level_object.h - declaration of class levelObject
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef _LEVEL_OBJECT_H
#define _LEVEL_OBJECT_H

#include "midi_time.h"




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


protected:
	int m_minLevel;
	int m_maxLevel;

} ;




#endif

