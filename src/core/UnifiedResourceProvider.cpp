/*
 * UnifiedResourceProvider.cpp - implementation of UnifiedResourceProvider
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


#include "UnifiedResourceProvider.h"
#include "ResourceDB.h"


UnifiedResourceProvider::UnifiedResourceProvider() :
	ResourceProvider( QString() )
{
	database()->init();
}




UnifiedResourceProvider::~UnifiedResourceProvider()
{
	database()->items().clear();

	foreach( ResourceDB * db, m_mergedDatabases )
	{
		delete db->provider();
	}
}




void UnifiedResourceProvider::addDatabase( ResourceDB * _db )
{
	ResourceItem::Relation * childRoot = _db->topLevelNode()->getChild( 0 );
	if( childRoot )
	{
		m_mergedDatabases << _db;
		connect( _db, SIGNAL( itemsChanged() ),
				this, SLOT( remergeItems() ),
				Qt::DirectConnection );
		connect( _db, SIGNAL( itemsChanged() ),
				database(), SIGNAL( itemsChanged() ),
				Qt::DirectConnection );

		childRoot->setParent( database()->topLevelNode() );
		database()->topLevelNode()->addChild( childRoot );

		remergeItems();
	}
}




void UnifiedResourceProvider::updateDatabase()
{
	foreach( ResourceDB * db, m_mergedDatabases )
	{
		db->provider()->updateDatabase();
	}
}




void UnifiedResourceProvider::remergeItems()
{
	typedef QHash<const ResourceItem *,
			const ResourceItem *> PointerHashMap;
	PointerHashMap itemsSeen;

	ResourceDB::ItemHashMap & items = database()->items();

	itemsSeen.reserve( items.size() );

	for( ResourceDB::ItemHashMap::Iterator it = items.begin();
						it != items.end(); ++it )
	{
		itemsSeen[*it] = *it;
	}

	foreach( ResourceDB * db, m_mergedDatabases )
	{
		for( ResourceDB::ItemHashMap::ConstIterator it =
							db->items().begin();
						it != db->items().end(); ++it )
		{
			const QString & h = (*it)->hash();
			if( !items.contains( h ) )
			{
				items[(*it)->hash()] = *it;
			}
			else
			{
				itemsSeen[*it] = NULL;
			}
		}
	}

	for( ResourceDB::ItemHashMap::Iterator it = items.begin();
						it != items.end(); )
	{
		if( itemsSeen[*it] == *it )
		{
			it = items.erase( it );
		}
		else
		{
			++it;
		}
	}
}





#include "moc_UnifiedResourceProvider.cxx"

