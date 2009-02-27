/*
 * local_resources_provider.cpp - implementation of LocalResourcesProvider
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

#include "local_resources_provider.h"
#include "resources_db.h"


LocalResourcesProvider::LocalResourcesProvider(
				ResourcesItem::BaseDirectory _baseDir,
						const QString & _dir ) :
	ResourcesProvider( ResourcesItem::getBaseDirectory( _baseDir ) ),
	m_baseDir( _baseDir ),
	m_dir( _dir ),
	m_watcher( this )
{
	connect( &m_watcher, SIGNAL( directoryChanged( const QString & ) ),
			this, SLOT( reloadDirectory( const QString & ) ) );

	connect( database(), SIGNAL( directoryItemAdded( const QString & ) ),
			this, SLOT( addDirectory( const QString & ) ) );
	connect( database(), SIGNAL( directoryItemRemoved( const QString & ) ),
			this, SLOT( removeDirectory( const QString & ) ) );

	database()->init();
}




void LocalResourcesProvider::updateDatabase( void )
{
	readDir( m_dir, database()->topLevelNode() );
}




int LocalResourcesProvider::dataSize( const ResourcesItem * _item ) const
{
	return QFileInfo( _item->fullName() ).size();
}




QByteArray LocalResourcesProvider::fetchData( const ResourcesItem * _item,
							int _maxSize ) const
{
	QFile f( _item->fullName() );
	f.open( QFile::ReadOnly );

	if( _maxSize == -1 )
	{
		return f.readAll();
	}

	return f.read( _maxSize );
}




void LocalResourcesProvider::addDirectory( const QString & _path )
{
	if( QFileInfo( _path ).isDir() )
	{
		m_watcher.addPath( _path );
	}
}




void LocalResourcesProvider::removeDirectory( const QString & _path )
{
	m_watcher.removePath( _path );
}




void LocalResourcesProvider::reloadDirectory( const QString & _path )
{
	ResourcesTreeItem * dirTreeItem = NULL;

	foreach( ResourcesItem * it, database()->items() )
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
			readDir( dirItem->path(), dirTreeItem->parent() );
		}
	}

	emit itemsChanged();
}




void LocalResourcesProvider::readDir( const QString & _dir,
					ResourcesTreeItem * _parent )
{
#ifdef LMMS_BUILD_LINUX
	if( _dir.startsWith( "/dev" ) ||
		_dir.startsWith( "/sys" ) ||
		_dir.startsWith( "/proc" ) )
	{
        	return;
	}
#endif

	QDir d( ResourcesItem::getBaseDirectory( m_baseDir ) + _dir );
	m_scannedFolders << d.canonicalPath();

	ResourcesItem * parentItem;
	ResourcesTreeItem * curParent = _parent->findChild( d.dirName() +
							QDir::separator(),
							m_baseDir );
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
		parentItem = new ResourcesItem( this,
					d.dirName(),
					ResourcesItem::TypeDirectory,
					m_baseDir,
					_parent->item() ?
					_parent->item()->path() + d.dirName() +
							QDir::separator() :
								QString::null );
		parentItem->setLastMod( QFileInfo(
					d.canonicalPath() ).lastModified() );
		database()->addItem( parentItem );
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
				curParent->findChild( fname, m_baseDir );
		if( curChild )
		{
			curChild->setTemporaryMarker( true );
			if( f.lastModified() > curChild->item()->lastMod() )
			{
				curChild->item()->setLastMod(
							f.lastModified() );
				if( curChild->item()->type() ==
						ResourcesItem::TypeDirectory )
				{
					readDir( _dir + fname, curParent );
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
				readDir( _dir + fname, curParent );
			}
			else if( f.isFile() )
			{
				ResourcesItem * newItem =
					new ResourcesItem( this,
						f.fileName(),
						ResourcesItem::TypeUnknown,
						m_baseDir, _dir );
				newItem->setLastMod( f.lastModified() );
				database()->addItem( newItem );
				ResourcesTreeItem * rti =
					new ResourcesTreeItem( curParent,
								newItem );
				rti->setTemporaryMarker( true );
			}
		}
	}

	for( ResourcesTreeItemList::Iterator it = curParent->children().begin();
					it != curParent->children().end(); )
	{
		if( (*it)->temporaryMarker() == false )
		{
			ResourcesTreeItem * item = *it;
			it = curParent->children().erase( it );
			database()->recursiveRemoveItems( item );
		}
		else
		{
			++it;
		}
	}
}



#include "moc_local_resources_provider.cxx"

