/*
 * ResourceItem.h - header file for ResourceItem
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

#ifndef _RESOURCE_ITEM_H
#define _RESOURCE_ITEM_H

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QList>

#include "ResourceProvider.h"
#include "TreeRelation.h"


class ResourceItem
{
public:
	typedef TreeRelation<ResourceItem> Relation;

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
		TypePreset,
		TypePluginSpecificResource,
		TypeProject,
		TypeMidiFile,
		TypeForeignProject,
		TypePlugin,
		TypeImage,
		NumTypes
	} ;
	typedef Types Type;

	ResourceItem( ResourceProvider * _provider,
			const QString & _name,
			Type _type,
			BaseDirectory _base_dir = BaseWorkingDir,
			const QString & _path = QString::null,
			const QString & _hash = QString::null,
			const QString & _author = QString::null,
			const QString & _tags = QString::null,
			int _size = -1,
			const QDateTime & _last_mod = QDateTime() );
	// copy constructor
	ResourceItem( const ResourceItem & _item );

	inline void setHidden( bool _h, const QAbstractItemModel * _model )
	{
		m_hidden[_model] = _h;
	}

	inline bool isHidden( const QAbstractItemModel * _model ) const
	{
		return m_hidden[_model];
	}


	const ResourceProvider * provider() const
	{
		return m_provider;
	}

	const QString & name() const
	{
		return m_name;
	}

	inline int nameHash() const
	{
		return m_nameHash;
	}

	inline QString nameExtension() const
	{
		return name().section( '.', -1 ).toLower();
	}

	Type type() const
	{
		return m_type;
	}

	const QString & path() const
	{
		return m_path;
	}

	BaseDirectory baseDir() const
	{
		return m_baseDir;
	}

	QString fullPath() const
	{
		return getBaseDirectory( m_baseDir, this ) + m_path;
	}

	QString fullName() const
	{
		return fullPath() + name();
	}

	QString fullRelativeName() const
	{
		return path() + name();
	}

	const QString & hash() const
	{
		return m_hash;
	}

	const QString & author() const
	{
		return m_author;
	}

	int size() const
	{
		return m_size;
	}

	bool isShippedResource() const
	{
		return baseDir() == BaseDataDir;
	}

	bool isLocalResource() const
	{
		return m_provider->isLocal();
	}

	const QString & tags() const
	{
		return m_tags;
	}

	bool isValid() const
	{
		return m_type != TypeUnknown && !m_name.isEmpty();
	}

	void setRelation( Relation * _relation )
	{
		m_relation = _relation;
	}

	Relation * relation()
	{
		return m_relation;
	}

	const Relation * relation() const
	{
		return m_relation;
	}

	const QDateTime & lastMod() const
	{
		return m_lastMod;
	}

	void setLastMod( const QDateTime & _date )
	{
		m_lastMod = _date;
	}

	int realSize() const
	{
		return m_provider->dataSize( this );
	}

	QByteArray fetchData( int _maxSize = -1 ) const
	{
		return m_provider->fetchData( this );
	}

	void reload();

	// returns true if all given keywords match name, tags etc.
	bool keywordMatch( const QStringList & _keywords );

	// return true, if given ResourceItem is equal
	bool operator==( const ResourceItem & _other ) const;

	// rates equality with given item
	int equalityLevel( const ResourceItem & _other ) const;

	Type guessType() const;

	static const char * mimeKey()
	{
		return "ResourceItem";
	}

	static QString getBaseDirectory( BaseDirectory _bd,
					const ResourceItem * _item = NULL );


private:
	void init();

	ResourceProvider * m_provider;
	QString m_name;
	int m_nameHash;
	Type m_type;
	BaseDirectory m_baseDir;
	QString m_path;
	QString m_hash;
	QString m_author;
	int m_size;
	QDateTime m_lastMod;
	QString m_tags;

	QHash<const QAbstractItemModel *, bool> m_hidden;

	Relation * m_relation;

} ;


typedef QList<ResourceItem *> ResourceItemList;


#define foreachResourceItemRelation(list)								\
		for(ResourceItem::Relation::List::Iterator it=list.begin();		\
					it!=list.end();++it)

#define foreachConstResourceItemRelation(list)							\
		for(ResourceItem::Relation::List::ConstIterator it=list.begin();\
						it!=list.end();++it)

#endif
