/*
 * ResourceLocation.cpp - implementation of ResourceLocation
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "ResourceLocation.h"
#include "LocalResourceProvider.h"
#include "UnknownResourceProvider.h"
#include "WebResourceProvider.h"


ResourceLocation::ResourceLocation( const QString & name,
									Type type,
									const QString & address ) :
	m_name( name ),
	m_type( type ),
	m_address( address )
{
}




ResourceLocation::~ResourceLocation()
{
}




ResourceProvider * ResourceLocation::createResourceProvider()
{
	switch( type() )
	{
		case LocalDirectory:
			return new LocalResourceProvider( ResourceItem::BaseRoot, address() );
		case Web:
			return new WebResourceProvider( address() );
		default: break;
	}

	return new UnknownResourceProvider( address() );
}



#include "moc_ResourceLocation.cxx"

