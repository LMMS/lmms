/*
 * resources_item.h - header file for ResourcesItem
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

#ifndef _RESOURCES_ITEM_H
#define _RESOURCES_ITEM_H

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>

#include "resources_provider.h"

class ResourcesTreeItem;


class ResourcesItem
{
public:
	enum BaseDirectories
	{
		BaseRoot,
		BaseWorkingDir,
		BaseDataDir,
		BaseHome,
		BaseURL,
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

	ResourcesItem( ResourcesProvider * _provider,
			const QString & _name,
			Type _type,
			BaseDirectory _base_dir = BaseWorkingDir,
			const QString & _path = QString::null,
			const QString & _hash = QString::null,
			const QString & _tags = QString::null,
			int _size = -1,
			const QDateTime & _last_mod = QDateTime() );


	const ResourcesProvider * provider( void ) const
	{
		return m_provider;
	}

	const QString & name( void ) const
	{
		return m_name;
	}

	inline int nameHash( void ) const
	{
		return m_nameHash;
	}

	Type type( void ) const
	{
		return m_type;
	}

	const QString & path( void ) const
	{
		return m_path;
	}

	BaseDirectory baseDir( void ) const
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

	void setTreeItem( ResourcesTreeItem * _ti )
	{
		m_treeItem = _ti;
	}

	ResourcesTreeItem * treeItem( void )
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

	int realSize( void ) const
	{
		return m_provider->dataSize( this );
	}

	QByteArray fetchData( int _maxSize = -1 ) const
	{
		return m_provider->fetchData( this );
	}

	void reload( void );

	bool operator==( const ResourcesItem & _other ) const;

	// rates equality with given item
	int equalityLevel( const ResourcesItem & _other ) const;

	Type guessType( void ) const;

	static QString getBaseDirectory( BaseDirectory _bd );


private:
	void init( void );

	ResourcesProvider * m_provider;
	QString m_name;
	int m_nameHash;
	Type m_type;
	BaseDirectory m_baseDir;
	QString m_path;
	QString m_hash;
	int m_size;
	QDateTime m_lastMod;
	QString m_tags;

	ResourcesTreeItem * m_treeItem;

} ;


#endif
