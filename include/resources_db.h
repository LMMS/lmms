/*
 * resources_db.h - header file for ResourcesDB
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

#ifndef _RESOURCES_DB_H
#define _RESOURCES_DB_H

#include <QtCore/QDateTime>
#include <QtCore/QFileSystemWatcher>
#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtXml/QDomDocument>

#define foreachTreeItem(list)	\
		for(ResourcesDB::TreeItemList::Iterator it=list.begin();\
					it!=list.end();++it)

#define foreachConstTreeItem(list)					\
		for(ResourcesDB::TreeItemList::ConstIterator it=list.begin(); \
						it!=list.end();++it)

class ResourcesDB : public QObject
{
	Q_OBJECT
public:
	class Item;
	class TreeItem;
	typedef QHash<QString, Item *> ItemList;
	typedef QList<TreeItem *> TreeItemList;

	class Item
	{
	public:
		enum BaseDirectories
		{
			BaseRoot,
			BaseWorkingDir,
			BaseDataDir,
			BaseHome,
			NumBaseDirectories
		} ;
		typedef BaseDirectories BaseDirectory;

		enum Types
		{
			TypeUnknown,
			TypeDirectory,
			TypeSample,
			TypeSoundFont,
			TypePreset,
			TypeProject,
			TypeMidiFile,
			TypeForeignProject,
			TypePlugin,
			TypeImage,
			NumTypes
		} ;
		typedef Types Type;

		Item( const QString & _name,
				Types _type,
				BaseDirectories _base_dir = BaseWorkingDir,
				const QString & _path = QString::null,
				const QString & _hash = QString::null,
				const QString & _tags = QString::null,
				int _size = -1,
				const QDateTime & _last_mod = QDateTime() ) :
			m_name( _name ),
			m_nameHash( 0 ),
			m_type( _type ),
			m_baseDir( _base_dir ),
			m_path( _path ),
			m_hash( _hash ),
			m_size( _size ),
			m_lastMod( _last_mod ),
			m_tags( _tags ),
			m_treeItem( NULL )
		{
			init();
		}

		Item() :
			m_name(),
			m_nameHash( 0 ),
			m_type( TypeUnknown ),
			m_baseDir( BaseRoot ),
			m_path(),
			m_hash(),
			m_size( -1 ),
			m_lastMod(),
			m_tags(),
			m_treeItem( NULL )
		{
			init();
		}


		const QString & name( void ) const
		{
			return m_name;
		}

		inline int nameHash( void ) const
		{
			return m_nameHash;
		}

		Types type( void ) const
		{
			return m_type;
		}

		const QString & path( void ) const
		{
			return m_path;
		}

		BaseDirectories baseDir( void ) const
		{
			return m_baseDir;
		}

		QString fullPath( void ) const
		{
			return getBaseDirectory( m_baseDir ) + m_path;
		}

		QString fullName( void ) const
		{
			if( m_type == TypeDirectory )
			{
				return fullPath();
			}
			return fullPath()+name();
		}

		const QString & hash( void ) const
		{
			return m_hash;
		}

		int size( void ) const
		{
			return m_size;
		}

		bool isShippedResource( void ) const
		{
			return baseDir() == BaseDataDir;
		}

		const QString & tags( void ) const
		{
			return m_tags;
		}

		bool isValid( void ) const
		{
			return m_type != TypeUnknown && !m_name.isEmpty();
		}

		void setTreeItem( TreeItem * _ti )
		{
			m_treeItem = _ti;
		}

		TreeItem * treeItem( void )
		{
			return m_treeItem;
		}

		const QDateTime & lastMod( void ) const
		{
			return m_lastMod;
		}

		void setLastMod( const QDateTime & _date )
		{
			m_lastMod = _date;
		}

		void reload( void );

		bool operator==( const Item & _other ) const;

		// rates equality with given item
		int equalityLevel( const Item & _other ) const;

		Types guessType( void ) const;

		static QString getBaseDirectory( BaseDirectories _bd );


	private:
		void init( void );

		QString m_name;
		int m_nameHash;
		Types m_type;
		BaseDirectories m_baseDir;
		QString m_path;
		QString m_hash;
		int m_size;
		QDateTime m_lastMod;
		QString m_tags;

		TreeItem * m_treeItem;

	} ;

	class TreeItem
	{
	public:
		TreeItem( TreeItem * _parent = NULL, Item * _item = NULL ) :
			m_parent( _parent ),
			m_hidden( false ),
			m_temporaryMarker( false ),
			m_item( _item )
		{
			if( m_parent )
			{
				m_parent->addChild( this );
			}
			if( m_item )
			{
				m_item->setTreeItem( this );
			}
		}

		~TreeItem()
		{
			foreachTreeItem( m_children )
			{
				delete *it;
			}
			if( m_item )
			{
				m_item->setTreeItem( NULL );
			}
			if( m_parent )
			{
				m_parent->removeChild( this );
			}
		}

		inline void setHidden( bool _h )
		{
			m_hidden = _h;
		}

		inline bool isHidden( void ) const
		{
			return m_hidden;
		}

		inline int rowCount( void ) const
		{
			int rc = 0;
			foreachConstTreeItem( m_children )
			{
				if( !(*it)->isHidden() )
				{
					++rc;
				}
			}
			return rc;
		}

		TreeItem * getChild( int _row )
		{
			int rc = 0;
			foreachTreeItem( m_children )
			{
				if( !(*it)->isHidden() )
				{
					if( rc == _row )
					{
						return *it;
					}
					++rc;
				}
			}
			return NULL;
		}

		int row( void ) const
		{
			if( !m_parent )
			{
				return 0;
			}

			int row = 0;
			foreachConstTreeItem( m_parent->m_children )
			{
				if( !(*it)->isHidden() )
				{
					if( *it == this )
					{
						return row;
					}
					++row;
				}
			}
			return 0;
		}

		inline void addChild( TreeItem * _it )
		{
			m_children.push_back( _it );
		}

		inline void removeChild( TreeItem * _it )
		{
			m_children.removeAll( _it );
		}

		inline TreeItemList & children( void )
		{
			return m_children;
		}

		inline const TreeItemList & children( void ) const
		{
			return m_children;
		}

		TreeItem * findChild( const QString & _name,
					Item::BaseDirectories _base_dir );

		inline Item * item( void )
		{
			return m_item;
		}

		inline const Item * item( void ) const
		{
			return m_item;
		}

		inline TreeItem * parent( void )
		{
			return m_parent;
		}

		inline bool temporaryMarker( void ) const
		{
			return m_temporaryMarker;
		}

		inline void setTemporaryMarker( bool _on )
		{
			m_temporaryMarker = _on;
		}


	private:
		// hide copy-ctor
		TreeItem( const TreeItem & ) { }

		TreeItem * m_parent;
		QList<TreeItem *> m_children;

		bool m_hidden;
		bool m_temporaryMarker;

		Item * m_item;
	} ;


	ResourcesDB( const QString & _db_file );
	~ResourcesDB();

	void scanResources( void );
	void load( void );
	void save( void );

	inline const ItemList & items( void ) const
	{
		return m_items;
	}

	inline TreeItem * topLevelNode( void )
	{
		return &m_topLevelNode;
	}

	const Item * nearestMatch( const Item & _item );


private slots:
	void reloadDirectory( const QString & _path );

private:
	void readDir( const QString & _dir, TreeItem * _parent,
					Item::BaseDirectories _base_dir );
	void replaceItem( Item * newItem );
	void recursiveRemoveItems( TreeItem * parent,
					bool removeTopLevelParent = true );

	void saveTreeItem( const TreeItem * _i, QDomDocument & _doc,
							QDomElement & _de );
	void loadTreeItem( TreeItem * _i, QDomElement & _de );


	typedef QList<QPair<Item::BaseDirectories, QString> > FolderList;
	FolderList m_folders;
	QStringList m_scannedFolders;
	QFileSystemWatcher m_watcher;

	QString m_dbFile;

	ItemList m_items;
	TreeItem m_topLevelNode;


signals:
	void itemsChanged( void );

} ;


#endif
