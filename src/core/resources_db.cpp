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


#include <QtCore/QDir>

#include "resources_db.h"
#include "config_mgr.h"
#include "lmms_basics.h"
#include "mmp.h"



ResourcesDB::ResourcesDB( const QString & _db_file ) :
	m_watcher( this ),
	m_dbFile( _db_file )
{
	m_folders += qMakePair( ResourcesItem::BaseDataDir, QString() );
	m_folders += qMakePair( ResourcesItem::BaseWorkingDir, QString() );

	if( QFile::exists( m_dbFile ) )
	{
		load();
	}
	// (re-) scan directories
	scanResources();
	save();

	connect( &m_watcher, SIGNAL( directoryChanged( const QString & ) ),
			this, SLOT( reloadDirectory( const QString & ) ) );
}




ResourcesDB::~ResourcesDB()
{
	save();
}




void ResourcesDB::load( void )
{
	m_items.clear();

	multimediaProject m( m_dbFile );

	loadTreeItem( &m_topLevelNode, m.content() );
}




void ResourcesDB::save( void )
{
	multimediaProject m( multimediaProject::ResourcesDatabase );
	saveTreeItem( &m_topLevelNode, m, m.content() );

	m.writeFile( m_dbFile );
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
ResourcesItem * item = new ResourcesItem( e.attribute( "name" ),
	static_cast<ResourcesItem::Type>( e.attribute( "type" ).toInt() ),
	static_cast<ResourcesItem::BaseDirectory>(
					e.attribute( "basedir" ).toInt() ),
						e.attribute( "path" ),
						h,
						e.attribute( "tags" ),
						e.attribute( "size" ).toInt(),
	QDateTime::fromString( e.attribute( "lastmod" ), Qt::ISODate ) );
replaceItem( item );
ResourcesTreeItem * treeItem = new ResourcesTreeItem( _i, item );
if( item->type() == ResourcesItem::TypeDirectory &&
				QFileInfo( item->fullPath() ).isDir() )
{
	m_watcher.addPath( item->fullPath() );
}
loadTreeItem( treeItem, e );
			}
		}
		node = node.nextSibling();
	}
}




void ResourcesDB::scanResources( void )
{
	for( FolderList::ConstIterator it = m_folders.begin();
						it != m_folders.end(); ++it )
	{
		readDir( it->second, &m_topLevelNode, it->first );
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




void ResourcesDB::reloadDirectory( const QString & _path )
{
	ResourcesTreeItem * dirTreeItem = NULL;

	foreach( ResourcesItem * it, m_items )
	{
		if( it->type() == ResourcesItem::TypeDirectory &&
						it->fullPath() == _path )
		{
			dirTreeItem = it->treeItem();
		}
	}

	if( dirTreeItem )
	{
		ResourcesItem * dirItem = dirTreeItem->item();
		if( dirItem )
		{
			m_scannedFolders.clear();
			readDir( dirItem->path(), dirTreeItem->parent(),
							dirItem->baseDir() );
		}
	}

	emit itemsChanged();
}





void ResourcesDB::replaceItem( ResourcesItem * newItem )
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
			m_watcher.removePath( oldItem->fullPath() );
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
			m_watcher.removePath( parent->item()->fullPath() );
		}
		const QString & hash = parent->item()->hash();
		if( !hash.isEmpty() )
		{
			m_items.remove( hash );
		}
		delete parent;
	}
}




void ResourcesDB::readDir( const QString & _dir, ResourcesTreeItem * _parent,
					ResourcesItem::BaseDirectory _base_dir )
{
#ifdef LMMS_BUILD_LINUX
	if( _dir.startsWith( "/dev" ) ||
		_dir.startsWith( "/sys" ) ||
		_dir.startsWith( "/proc" ) )
	{
        	return;
	}
#endif

	QDir d( ResourcesItem::getBaseDirectory( _base_dir ) + _dir );
	m_scannedFolders << d.canonicalPath();

	ResourcesItem * parentItem;
	ResourcesTreeItem * curParent = _parent->findChild( d.dirName() +
							QDir::separator(),
							_base_dir );
printf("read dir: %s\n", d.canonicalPath().toAscii().constData() );
	if( curParent )
	{
		parentItem = curParent->item();
		foreachResourcesTreeItem( curParent->children() )
		{
			(*it)->setTemporaryMarker( false );
		}
	}
	else
	{
		// create new item for current dir
		parentItem = new ResourcesItem( d.dirName(),
						ResourcesItem::TypeDirectory,
						_base_dir,
				_parent->item() ?
					_parent->item()->path() + d.dirName() +
							QDir::separator() :
								QString::null );
		parentItem->setLastMod( QFileInfo(
					d.canonicalPath() ).lastModified() );
		replaceItem( parentItem );
		curParent = new ResourcesTreeItem( _parent, parentItem );
		curParent->setTemporaryMarker( true );
		m_watcher.addPath( parentItem->fullPath() );
	}


	QFileInfoList list = d.entryInfoList( QDir::NoDotAndDotDot |
						QDir::Dirs | QDir::Files |
						QDir::Readable,
						QDir::Name | QDir::DirsFirst );
	foreach( QFileInfo f, list )
	{
		if( f.isSymLink() )
		{
			f = QFileInfo( f.symLinkTarget() );
		}

		QString fname = f.fileName();
		if( f.isDir() )
		{
			fname += QDir::separator();
		}
		ResourcesTreeItem * curChild =
				curParent->findChild( fname, _base_dir );
		if( curChild )
		{
			curChild->setTemporaryMarker( true );
			if( f.lastModified() > curChild->item()->lastMod() )
			{
//printf("reload: %s\n", fname.toAscii().constData());
				curChild->item()->setLastMod(
							f.lastModified() );
				if( curChild->item()->type() ==
						ResourcesItem::TypeDirectory )
				{
					readDir( _dir + fname, curParent,
								_base_dir );
				}
				else
				{
					curChild->item()->reload();
				}
			}
		}
		else
		{
			if( f.isDir() &&
				!m_scannedFolders.contains(
						f.canonicalFilePath() ) )

			{
				readDir( _dir + fname, curParent, _base_dir );
			}
			else if( f.isFile() )
			{
				ResourcesItem * newItem =
					new ResourcesItem( f.fileName(),
						ResourcesItem::TypeUnknown,
							_base_dir, _dir );
				newItem->setLastMod( f.lastModified() );
				replaceItem( newItem );
				ResourcesTreeItem * ti =
					new ResourcesTreeItem( curParent,
								newItem );
				ti->setTemporaryMarker( true );
			}
		}
	}

	for( ResourcesTreeItemList::Iterator it = curParent->children().begin();
					it != curParent->children().end(); )
	{
		if( (*it)->temporaryMarker() == false )
		{
			//printf("removing %d %s\n", (*it)->item(), (*it)->item()->name().toAscii().constData() );
			recursiveRemoveItems( *it );
			it = curParent->children().erase( it );
		}
		else
		{
			++it;
		}
	}
}



#include "moc_resources_db.cxx"
