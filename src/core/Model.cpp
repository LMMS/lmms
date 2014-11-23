/*
 * Model.cpp - implementation of Model base class
 *
 * Copyright (c) 2007-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "Model.h"


QString Model::fullDisplayName() const
{
	const QString & n = displayName();
	if( parentModel() ) 
	{
		const QString p = parentModel()->fullDisplayName();
		if( n.isEmpty() && p.isEmpty() )
		{
			return QString::null;
		}
		else if( p.isEmpty() )
		{
			return n;
		}
		return p + ">" + n;
	}
	return n;
}



#include "moc_Model.cxx"

