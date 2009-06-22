/*
 * ResourceDB.cpp - implementation of ResourceDB
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

#include "ResourceDB.h"
#include "ResourceProvider.h"
#include "mmp.h"


QMap<ResourceItem::Type, QString> ResourceDB::s_typeNames;
QMap<ResourceItem::BaseDirectory, QString> ResourceDB::s_baseDirNames;



ResourceDB::ResourceDB( ResourceProvider * _provider ) :
	m_provider( _provider )
{
	connect( m_provider, SIGNAL( itemsChanged() ),
			this, SIGNAL( itemsChanged() ) );

	if( s_typeNames.isEmpty() )
	{
		s_typeNames[ResourceItem::TypeUnknown] = "Unknown";
		s_typeNames[ResourceItem::TypeDirectory] = "Directory";
		s_typeNames[ResourceItem::TypeSample] = "Sample";
		s_typeNames[ResourceItem::TypeSoundFont] = "SoundFont";
		s_typeNames[ResourceItem::TypePreset] = "Preset";
		s_typeNames[ResourceItem::TypeProject] = "Project";
		s_typeNames[ResourceItem::TypeMidiFile] = "MidiFile";
		s_typeNames[ResourceItem::TypeForeignProject] = "ForeignProject";
		s_typeNames[ResourceItem::TypePlugin] = "Plugin";
		s_typeNames[ResourceItem::TypeImage] = "Image";
	}

	if( s_baseDirNames.isEmpty() )
	{
		s_baseDirNames[ResourceItem::BaseRoot] = "Root";
		s_baseDirNames[ResourceItem::BaseWorkingDir] = "WorkingDir";
		s_baseDirNames[ResourceItem::BaseDataDir] = "DataDir";
		s_baseDirNames[ResourceItem::BaseHome] = "Home";
		s_baseDirNames[ResourceItem::BaseURL] = "URL";
	}


}




ResourceDB::~ResourceDB()
{
}




void ResourceDB::init( void )
{
	if( QFileInfo( m_provider->localCacheFile() ).exists() )
	{
		load( m_provider->localCacheFile() );
	}

	m_provider->updateDatabase();

	if( m_provider->cacheDatabase() )
	{
		save( m_provider->localCacheFile() );
	}
}




void ResourceDB::load( const QString & _file )
{
	recursiveRemoveItems( topLevelNode(), false );

	multimediaProject m( _file );

	loadTreeItem( &m_topLevelNode, m.content() );
}




void ResourceDB::save( const QString & _file )
{
	multimediaProject m( multimediaProject::ResourceDatabase );

	if( m_topLevelNode.getChild( 0 ) )
	{
		saveTreeItem( m_topLevelNode.getChild( 0 ), m, m.content() );
	}

	m.writeFile( _file );
}




void ResourceDB::saveTreeItem( const ResourceTreeItem * _i,
							QDomDocument & _doc,
							QDomElement & _de )
{
	QDomElement e = _i->item() ? _doc.createElement( "item" ) : _de;
	foreachConstResourceTreeItem( _i->children() )
	{
		saveTreeItem( *it, _doc, e );
	}
	if( _i->item() )
	{
		const ResourceItem * it = _i->item();
		e.setAttribute( "name", it->name() );
		e.setAttribute( "type", typeName( it->type() ) );
		e.setAttribute( "basedir", baseDirName( it->baseDir() ) );
		e.setAttribute( "path", it->path() );
		e.setAttribute( "hash", it->hash() );
		e.setAttribute( "author", it->author() );
		e.setAttribute( "size", it->size() );
		e.setAttribute( "tags", it->tags() );
		e.setAttribute( "lastmod", it->lastMod().
						toString( Qt::ISODate ) );
		_de.appendChild( e );
	}
}




void ResourceDB::loadTreeItem( ResourceTreeItem * _i, QDomElement & _de )
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
ResourceItem * item = new ResourceItem( m_provider,
						e.attribute( "name" ),
				typeFromName( e.attribute( "type" ) ),
				baseDirFromName( e.attribute( "basedir" ) ),
						e.attribute( "path" ),
						h,
						e.attribute( "author" ),
						e.attribute( "tags" ),
						e.attribute( "size" ).toInt(),
	QDateTime::fromString( e.attribute( "lastmod" ), Qt::ISODate ) );
addItem( item );
ResourceTreeItem * treeItem = new ResourceTreeItem( _i, item );
if( item->type() == ResourceItem::TypeDirectory )
{
	emit directoryItemAdded( item->fullName() );
}
loadTreeItem( treeItem, e );
			}
		}
		node = node.nextSibling();
	}
}




const ResourceItem * ResourceDB::itemByHash( const QString & _hash ) const
{
	ItemList::ConstIterator it = m_items.find( _hash );
	if( it != m_items.end() )
	{
		return it.value();
	}
	return NULL;
}




const ResourceItem * ResourceDB::nearestMatch( const ResourceItem & _item )
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
	const ResourceItem * max_item = NULL;

	foreach( const ResourceItem * it, m_items )
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




void ResourceDB::addItem( ResourceItem * newItem )
{
	const QString hash = newItem->hash();
	ResourceItem * oldItem = m_items[hash];
	if( oldItem )
	{
		ResourceTreeItem * oldTreeItem = oldItem->treeItem();
		if( oldTreeItem )
		{
			recursiveRemoveItems( oldTreeItem, false );
			delete oldTreeItem;
		}
		if( oldItem->type() == ResourceItem::TypeDirectory )
		{
			emit directoryItemRemoved( oldItem->fullName() );
		}
		m_items.remove( hash );
		delete oldItem;
	}
	m_items[hash] = newItem;
}




void ResourceDB::recursiveRemoveItems( ResourceTreeItem * parent,
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
		if( parent->item()->type() == ResourceItem::TypeDirectory )
		{
			emit directoryItemRemoved( parent->item()->fullName() );
		}
		const QString & hash = parent->item()->hash();
		if( !hash.isEmpty() )
		{
			m_items.remove( hash );
		}
		delete parent;
	}
}





#include "moc_ResourceDB.cxx"
