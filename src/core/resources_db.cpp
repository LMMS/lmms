/*
 * resources_db.cpp - implementation of ResourcesDB
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QFileInfo>

#include "resources_db.h"
#include "resources_provider.h"
#include "mmp.h"



ResourcesDB::ResourcesDB( ResourcesProvider * _provider ) :
	m_provider( _provider )
{
	connect( m_provider, SIGNAL( itemsChanged() ),
			this, SIGNAL( itemsChanged() ) );

}




ResourcesDB::~ResourcesDB()
{
	save( m_provider->localCatalogueFile() );
}




void ResourcesDB::init( void )
{
	if( QFileInfo( m_provider->localCatalogueFile() ).exists() )
	{
		load( m_provider->localCatalogueFile() );
	}

	m_provider->updateDatabase();

	save( m_provider->localCatalogueFile() );
}




void ResourcesDB::load( const QString & _file )
{
	recursiveRemoveItems( topLevelNode(), false );

	multimediaProject m( _file );

	loadTreeItem( &m_topLevelNode, m.content() );
}




void ResourcesDB::save( const QString & _file )
{
	multimediaProject m( multimediaProject::ResourcesDatabase );

	saveTreeItem( &m_topLevelNode, m, m.content() );

	m.writeFile( _file );
}




void ResourcesDB::saveTreeItem( const ResourcesTreeItem * _i,
							QDomDocument & _doc,
							QDomElement & _de )
{
	QDomElement e = _i->item() ? _doc.createElement( "item" ) : _de;
	foreachConstResourcesTreeItem( _i->children() )
	{
		saveTreeItem( *it, _doc, e );
	}
	if( _i->item() )
	{
		const ResourcesItem * it = _i->item();
		e.setAttribute( "name", it->name() );
		e.setAttribute( "type", it->type() );
		e.setAttribute( "basedir", it->baseDir() );
		e.setAttribute( "path", it->path() );
		e.setAttribute( "hash", it->hash() );
		e.setAttribute( "size", it->size() );
		e.setAttribute( "tags", it->tags() );
		e.setAttribute( "lastmod", it->lastMod().
						toString( Qt::ISODate ) );
		_de.appendChild( e );
	}
}




void ResourcesDB::loadTreeItem( ResourcesTreeItem * _i, QDomElement & _de )
{
	QDomNode node = _de.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			QDomElement e = node.toElement();
			const QString h = e.attribute( "hash" );
			if( !h.isEmpty() )
			{
ResourcesItem * item = new ResourcesItem( m_provider,
						e.attribute( "name" ),
	static_cast<ResourcesItem::Type>( e.attribute( "type" ).toInt() ),
	static_cast<ResourcesItem::BaseDirectory>(
					e.attribute( "basedir" ).toInt() ),
						e.attribute( "path" ),
						h,
						e.attribute( "tags" ),
						e.attribute( "size" ).toInt(),
	QDateTime::fromString( e.attribute( "lastmod" ), Qt::ISODate ) );
addItem( item );
ResourcesTreeItem * treeItem = new ResourcesTreeItem( _i, item );
if( item->type() == ResourcesItem::TypeDirectory )
{
	emit directoryItemAdded( item->fullPath() );
}
loadTreeItem( treeItem, e );
			}
		}
		node = node.nextSibling();
	}
}




const ResourcesItem * ResourcesDB::nearestMatch( const ResourcesItem & _item )
{
	if( !_item.hash().isEmpty() )
	{
		ItemList::ConstIterator it = m_items.find( _item.hash() );
		if( it != m_items.end() )
		{
			return it.value();
		}
	}

	int max_level = -1;
	const ResourcesItem * max_item = NULL;

	foreach( const ResourcesItem * it, m_items )
	{
		const int l = it->equalityLevel( _item );
		if( l > max_level )
		{
			max_item = it;
		}
	}

	Q_ASSERT( max_item != NULL );

	return max_item;
}




void ResourcesDB::addItem( ResourcesItem * newItem )
{
	const QString hash = newItem->hash();
	ResourcesItem * oldItem = m_items[hash];
	if( oldItem )
	{
		ResourcesTreeItem * oldTreeItem = oldItem->treeItem();
		if( oldTreeItem )
		{
			recursiveRemoveItems( oldTreeItem, false );
			delete oldTreeItem;
		}
		if( oldItem->type() == ResourcesItem::TypeDirectory )
		{
			emit directoryItemRemoved( oldItem->fullPath() );
		}
		m_items.remove( hash );
		delete oldItem;
	}
	m_items[hash] = newItem;
}




void ResourcesDB::recursiveRemoveItems( ResourcesTreeItem * parent,
						bool removeTopLevelParent )
{
	if( !parent )
	{
		return;
	}

	while( !parent->children().isEmpty() )
	{
		recursiveRemoveItems( parent->children().front() );
	}

	if( removeTopLevelParent && parent->item() )
	{
		if( parent->item()->type() == ResourcesItem::TypeDirectory )
		{
			emit directoryItemRemoved( parent->item()->fullPath() );
		}
		const QString & hash = parent->item()->hash();
		if( !hash.isEmpty() )
		{
			m_items.remove( hash );
		}
		delete parent;
	}
}





#include "moc_resources_db.cxx"
