/*
 * LocalResourceProvider.cpp - implementation of LocalResourceProvider
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

#include "LocalResourceProvider.h"
#include "ResourceDB.h"


LocalResourceProvider::LocalResourceProvider(
				ResourceItem::BaseDirectory _baseDir,
						const QString & _dir ) :
	ResourceProvider( ResourceItem::getBaseDirectory( _baseDir ) ),
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




LocalResourceProvider::~LocalResourceProvider()
{
	database()->save( localCacheFile() );
}




void LocalResourceProvider::updateDatabase( void )
{
	readDir( m_dir, database()->topLevelNode() );
}




int LocalResourceProvider::dataSize( const ResourceItem * _item ) const
{
	return QFileInfo( _item->fullName() ).size();
}




QByteArray LocalResourceProvider::fetchData( const ResourceItem * _item,
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




void LocalResourceProvider::addDirectory( const QString & _path )
{
	if( QFileInfo( _path ).isDir() )
	{
		m_watcher.addPath( _path );
	}
}




void LocalResourceProvider::removeDirectory( const QString & _path )
{
	m_watcher.removePath( _path );
}




void LocalResourceProvider::reloadDirectory( const QString & _path )
{
	ResourceItem::Relation * dirRelation = NULL;
	QString p = _path;
	if( !p.endsWith( QDir::separator() ) )
	{
		p += QDir::separator();
	}

	foreach( ResourceItem * it, database()->items() )
	{
		if( it->type() == ResourceItem::TypeDirectory &&
			it->fullName() == p )
		{
			dirRelation = it->relation();
		}
	}

	if( dirRelation )
	{
		ResourceItem * dirItem = dirRelation->item();
		if( dirItem )
		{
			m_scannedFolders.clear();
			readDir( dirItem->fullRelativeName(),
						dirRelation->parent() );
		}
	}

	emit itemsChanged();
}




void LocalResourceProvider::readDir( const QString & _dir,
					ResourceItem::Relation * _parent )
{
#ifdef LMMS_BUILD_LINUX
	if( _dir.startsWith( "/dev" ) ||
		_dir.startsWith( "/sys" ) ||
		_dir.startsWith( "/proc" ) )
	{
        	return;
	}
#endif

	QDir d( ResourceItem::getBaseDirectory( m_baseDir ) + _dir );
	m_scannedFolders << d.canonicalPath();

	ResourceItem * parentItem;
	ResourceItem::Relation * curParent = _parent->findChild( d.dirName() +
							QDir::separator(),
							m_baseDir );
printf("read dir: %s\n", d.canonicalPath().toUtf8().constData() );
	if( curParent )
	{
		parentItem = curParent->item();
		foreachResourceItemRelation( curParent->children() )
		{
			(*it)->setTemporaryMarker( false );
		}
	}
	else
	{
		// create new item for current dir
		parentItem = new ResourceItem( this,
					d.dirName(),
					ResourceItem::TypeDirectory,
					m_baseDir,
					_parent->item() && _parent->parent() &&
					_parent->parent()->item() ?
					_parent->item()->fullRelativeName() :
								QString::null );
		parentItem->setLastMod( QFileInfo(
					d.canonicalPath() ).lastModified() );
		database()->addItem( parentItem );
		curParent = new ResourceItem::Relation( _parent, parentItem );
		curParent->setTemporaryMarker( true );
		m_watcher.addPath( parentItem->fullName() );
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
		ResourceItem::Relation * curChild =
				curParent->findChild( fname, m_baseDir );
		if( curChild )
		{
			curChild->setTemporaryMarker( true );
			if( f.lastModified() > curChild->item()->lastMod() )
			{
				curChild->item()->setLastMod(
							f.lastModified() );
				if( curChild->item()->type() ==
						ResourceItem::TypeDirectory )
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
				ResourceItem * newItem =
					new ResourceItem( this,
						f.fileName(),
						ResourceItem::TypeUnknown,
						m_baseDir,
						_dir,
						QString::null, // hash
						QString::null // TODO: author
					);
				newItem->setLastMod( f.lastModified() );
				database()->addItem( newItem );
				ResourceItem::Relation * relation =
					new ResourceItem::Relation( curParent,
								newItem );
				relation->setTemporaryMarker( true );
			}
		}
	}

	for( ResourceItem::Relation::List::Iterator it =
			curParent->children().begin();
						it != curParent->children().end(); )
	{
		if( (*it)->temporaryMarker() == false )
		{
			ResourceItem::Relation * item = *it;
			it = curParent->children().erase( it );
			database()->recursiveRemoveItems( item );
		}
		else
		{
			++it;
		}
	}
}



#include "moc_LocalResourceProvider.cxx"

