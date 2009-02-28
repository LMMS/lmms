/*
 * unified_resources_provider.cpp - implementation of UnifiedResourcesProvider
 *
 * Copyright (c) 2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QDir>

#include "unified_resources_provider.h"
#include "resources_db.h"


UnifiedResourcesProvider::UnifiedResourcesProvider() :
	ResourcesProvider( QString() )
{
	database()->init();
}




void UnifiedResourcesProvider::addDatabase( ResourcesDB * _db )
{
	ResourcesTreeItem * childRoot = _db->topLevelNode()->getChild( 0 );
	if( childRoot )
	{
		m_mergedDatabases << _db;
		connect( _db, SIGNAL( itemsChanged() ),
				this, SIGNAL( itemsChanged() ) );

		childRoot->setParent( database()->topLevelNode() );
		database()->topLevelNode()->addChild( childRoot );
	}
}




void UnifiedResourcesProvider::updateDatabase( void )
{
	foreach( ResourcesDB * db, m_mergedDatabases )
	{
		db->provider()->updateDatabase();
	}
}





#include "moc_unified_resources_provider.cxx"

