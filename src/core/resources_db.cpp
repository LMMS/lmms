/*
 * resources_db.cpp - implementation of ResourcesDB
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>

#include "resources_db.h"
#include "config_mgr.h"
#include "lmms_basics.h"
#include "mmp.h"



void ResourcesDB::Item::reload( void )
{
	m_hash.clear();
	m_size = -1;
	init();
}


bool ResourcesDB::Item::operator==( const Item & _other ) const
{
	return m_name == _other.m_name &&
		m_type == _other.m_type &&
		m_path == _other.m_path &&
		m_hash == _other.m_hash &&
		m_size == _other.m_size &&
		m_tags == _other.m_tags;
}




int ResourcesDB::Item::equalityLevel( const Item & _other ) const
{
	int l = 0;
	if( m_path == _other.m_path && m_name == _other.m_name )
	{
		l += 40;
	}
	else if( m_name == _other.m_name )
	{
		l += 30;
	}
	else if( m_path == _other.m_path )
	{
		l += 10;
	}

	if( m_type == _other.m_type )
	{
		l += 5;
	}

	if( !m_tags.isEmpty() && !_other.m_tags.isEmpty() )
	{
		QStringList my_tags = m_tags.split( " " );
		QStringList o_tags = _other.m_tags.split( " " );
		foreach( const QString & tag, o_tags )
		{
			if( my_tags.contains( tag ) )
			{
				l += 10;
			}
		}
	}

	if( m_size == _other.m_size )
	{
		l += 20;
	}

	if( m_hash == _other.m_hash )
	{
		l += 100;
	}
	return l;
}




ResourcesDB::Item::Types ResourcesDB::Item::guessType( void ) const
{
	static QMap<QString, Types> typeMap;
	if( typeMap.isEmpty() )
	{
		typeMap["wav"] = TypeSample;
		typeMap["ogg"] = TypeSample;
		typeMap["mp3"] = TypeSample;
		typeMap["ds"] = TypeSample;
		typeMap["flac"] = TypeSample;
		typeMap["spx"] = TypeSample;
		typeMap["voc"] = TypeSample;
		typeMap["au"] = TypeSample;
		typeMap["raw"] = TypeSample;
		typeMap["aif"] = TypeSample;
		typeMap["aiff"] = TypeSample;

		typeMap["sf2"] = TypeSoundFont;

		typeMap["xpf"] = TypePreset;

		typeMap["mmp"] = TypeProject;
		typeMap["mmpz"] = TypeProject;

		typeMap["mid"] = TypeMidiFile;

		typeMap["flp"] = TypeForeignProject;

		typeMap["dll"] = TypePlugin;
		typeMap["so"] = TypePlugin;

		typeMap["png"] = TypeImage;
		typeMap["jpg"] = TypeImage;
		typeMap["jpeg"] = TypeImage;
	}

	const QString s = QFileInfo( fullName() ).suffix().toLower();
	QMap<QString, Types>::ConstIterator it = typeMap.find( s );
	if( it != typeMap.end() )
	{
		return it.value();
	}
	return TypeUnknown;
}




void ResourcesDB::Item::init( void )
{
	if( name().isEmpty() )
	{
		return;
	}

	// ensure trailing slash for path property
	if( !m_path.isEmpty() && m_path.right( 1 ) != QDir::separator() )
	{
		m_path += QDir::separator();
	}

	if( m_type == TypeUnknown )
	{
		m_type = guessType();
	}

	// if item is a directory, ensure a trailing slash
	if( m_type == TypeDirectory )
	{
		if( !m_name.isEmpty() &&
				m_name.right( 1 ) != QDir::separator() )
		{
			m_name += QDir::separator();
		}
		if( m_hash.isEmpty() )
		{
			QCryptographicHash h( QCryptographicHash::Sha1 );
			h.addData( fullName().toUtf8() );
			m_hash = h.result().toHex();
		}
	}
	// only stat file if we really need to
	else if( ( m_hash.isEmpty() || m_size < 0 ) &&
						QFile::exists( fullName() ) )
	{
		if( m_size < 0 )
		{
			m_size = QFileInfo( fullName() ).size();
		}
		if( m_hash.isEmpty() )
		{
			QCryptographicHash h( QCryptographicHash::Sha1 );

			QFile f( fullName() );
			f.open( QFile::ReadOnly );

			const int chunkSize = 1024*1024;	// 1 MB
			for( int i = 0; i < f.size() / chunkSize; ++i )
			{
				h.addData( f.read( chunkSize ) );
			}
			h.addData( f.readAll() );

			m_hash = h.result().toHex();
		}
	}
}




QString ResourcesDB::Item::getBaseDirectory( BaseDirectories _bd )
{
	QString d;
	switch( _bd )
	{
		case BaseRoot:
			d = QDir::rootPath();
			break;
		case BaseWorkingDir:
			d = configManager::inst()->workingDir();
			break;
		case BaseDataDir:
			d = configManager::inst()->dataDir();
			break;
		case BaseHome:
		default:
			d = QDir::homePath();
			break;
	}
	if( !d.isEmpty() && d.right( 1 ) != QDir::separator() )
	{
		d += QDir::separator();
	}

	return d;
}








ResourcesDB::ResourcesDB( const QString & _db_file ) :
	m_watcher( this ),
	m_dbFile( _db_file )
{
	m_folders += qMakePair( Item::BaseDataDir, QString() );
	m_folders += qMakePair( Item::BaseWorkingDir, QString() );

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

	loadTreeItem( m_topLevelNode, m.content() );
}




void ResourcesDB::save( void )
{
	multimediaProject m( multimediaProject::ResourcesDatabase );
	saveTreeItem( m_topLevelNode, m, m.content() );

	m.writeFile( m_dbFile );
}




void ResourcesDB::saveTreeItem( const TreeItem & _i, QDomDocument & _doc,
							QDomElement & _de )
{
	QDomElement e = _i.item() ? _doc.createElement( "item" ) : _de;
	foreachConstTreeItem( _i.children() )
	{
		saveTreeItem( *it, _doc, e );
	}
	if( _i.item() )
	{
		const Item * it = _i.item();
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




void ResourcesDB::loadTreeItem( TreeItem & _i, QDomElement & _de )
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
				m_items[h] = Item( e.attribute( "name" ),
	static_cast<Item::Types>( e.attribute( "type" ).toInt() ),
	static_cast<Item::BaseDirectories>( e.attribute( "basedir" ).toInt() ),
						e.attribute( "path" ),
						h,
						e.attribute( "tags" ),
						e.attribute( "size" ).toInt(),
	QDateTime::fromString( e.attribute( "lastmod" ), Qt::ISODate ) );

				_i.addChild( TreeItem( &_i, &m_items[h] ) );
				m_items[h].setTreeItem( &_i.children().last() );
				if( m_items[h].type() == Item::TypeDirectory &&
			QFileInfo( m_items[h].fullPath() ).isDir() )
				{
					m_watcher.addPath(
							m_items[h].fullPath() );
				}
				loadTreeItem( _i.children().last(), e );
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




const ResourcesDB::Item & ResourcesDB::nearestMatch( const Item & _item )
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
	const Item * max_item = NULL;

	foreach( const Item & it, m_items )
	{
		const int l = it.equalityLevel( _item );
		if( l > max_level )
		{
			max_item = &it;
		}
	}

	Q_ASSERT( max_item != NULL );

	return *max_item;
}




void ResourcesDB::reloadDirectory( const QString & _path )
{
	TreeItem * dirTreeItem = NULL;

	foreach( Item it, m_items )
	{
		if( it.type() == Item::TypeDirectory && it.fullPath() == _path )
		{
			dirTreeItem = it.treeItem();
		}
	}

	if( dirTreeItem )
	{
		Item * dirItem = dirTreeItem->item();
		if( dirItem )
		{
			m_scannedFolders.clear();
			readDir( dirItem->path(), dirTreeItem->parent(),
							dirItem->baseDir() );
		}
	}

	emit itemsChanged();
}




void ResourcesDB::recursiveRemoveItems( TreeItemList::Iterator _it )
{
	for( TreeItemList::Iterator ch_it = _it->children().begin();
				ch_it != _it->children().end(); ++ch_it )
	{
		recursiveRemoveItems( ch_it );
	}
	if( _it->item() )
	{
		if( _it->item()->type() == Item::TypeDirectory )
		{
			m_watcher.removePath( _it->item()->fullPath() );
		}
		m_items.remove( _it->item()->hash() );
	}
}




void ResourcesDB::readDir( const QString & _dir, TreeItem * _parent,
					Item::BaseDirectories _base_dir )
{
#ifdef LMMS_BUILD_LINUX
	if( _dir.startsWith( "/dev" ) ||
		_dir.startsWith( "/sys" ) ||
		_dir.startsWith( "/proc" ) )
	{
        	return;
	}
#endif

	QDir d( Item::getBaseDirectory( _base_dir ) + _dir );
	m_scannedFolders << d.canonicalPath();

	Item * parentItem;
	TreeItem * curParent = _parent->findChild( d.dirName() +
							QDir::separator(),
							_base_dir );
printf("read dir: %s\n", d.canonicalPath().toAscii().constData() );
	if( curParent )
	{
		parentItem = curParent->item();
		foreachTreeItem( curParent->children() )
		{
			it->setTemporaryMarker( false );
		}
	}
	else
	{
		// create new item for current dir
		Item parent( d.dirName(), Item::TypeDirectory,
				_base_dir, _parent->item() ?
					_parent->item()->path() + d.dirName() +
							QDir::separator() :
								QString::null );
		parent.setLastMod( QFileInfo(
					d.canonicalPath() ).lastModified() );

		parentItem = &( m_items[parent.hash()] = parent );
		_parent->addChild( TreeItem( _parent, parentItem ) );
		curParent = &_parent->children().last();
		curParent->setTemporaryMarker( true );
		parentItem->setTreeItem( curParent );
		m_watcher.addPath( parent.fullPath() );
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
		TreeItem * curChild = curParent->findChild( fname, _base_dir );
		if( curChild )
		{
			curChild->setTemporaryMarker( true );
			if( f.lastModified() > curChild->item()->lastMod() )
			{
printf("reload: %s\n", fname.toAscii().constData());
				curChild->item()->setLastMod(
							f.lastModified() );
				if( curChild->item()->type() ==
							Item::TypeDirectory )
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
				Item i( f.fileName(), Item::TypeUnknown,
						_base_dir, _dir );
				i.setLastMod( f.lastModified() );
				TreeItem ti( curParent,
						&( m_items[i.hash()] = i ) );
				ti.setTemporaryMarker( true );
				curParent->addChild( ti );
			}
		}
	}

	for( TreeItemList::Iterator it = curParent->children().begin();
					it != curParent->children().end(); )
	{
		if( it->temporaryMarker() == false )
		{
	printf("removing %s\n", it->item()->name().toAscii().constData() );
			recursiveRemoveItems( it );
			it = curParent->children().erase( it );
		}
		else
		{
			++it;
		}
	}
}



#include "moc_resources_db.cxx"
