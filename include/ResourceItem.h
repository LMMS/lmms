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

#include "export.h"
#include "ResourceProvider.h"
#include "TreeRelation.h"


/*! \brief The ResourceItem class provides information about a local/remote file/directory.
 *
 * All relevant properties of a file or directory are stored within a
 * ResourceItem and can be accessed easily. All resources are identified by
 * a unique hash (based on file content or (absolute) directory name).
 *
 * ResourceItems are managed within a ResourceDB. Reading and writing resource
 * data is abstracted into the ResourceProvider class.
 *
 * The ResourceItem class does not provide any actual functionality.
 * Use ResourceAction or more high level classes like ResourcePreviewer and
 * ResourceBrowser.
 */

class EXPORT ResourceItem
{
public:
	/*! A relation specifies how ResourceItems are organized among each other.
	 * See documentation of TreeRelation for details. */
	typedef TreeRelation<ResourceItem> Relation;

	/*! Lists all supported base directories for ResourceItems. */
	enum BaseDirectories
	{
		BaseRoot,		/*!< Item is relative to root directory */
		BaseWorkingDir,	/*!< Item is relative to working directory */
		BaseDataDir,	/*!< Item is relative to LMMS' data directory */
		BaseHome,		/*!< Item is relative to user's home directory */
		BaseURL,		/*!< Item is relative to the URL of the ResourceProvider */
		NumBaseDirectories
	} ;
	typedef BaseDirectories BaseDirectory;

	/*! Lists all supported ResourceItem types. */
	enum Types
	{
		TypeUnknown,	/*!< No known resource type */
		TypeDirectory,	/*!< Item is a directory */
		TypeSample,		/*!< Item is a supported sample file */
		TypePreset,		/*!< Item is a LMMS-specific preset */
		TypePluginSpecificResource,	/* Item is a file supported by one of the available plugins */
		TypeProject,	/*!< Item is a LMMS project */
		TypeMidiFile,	/*!< Item is a MIDI file (and can be imported via MIDI import filter) */
		TypeForeignProject,	/*!< Item is any other kind of project which can be imported via an ImportFilter plugin */
		TypePlugin,		/*!< Item is a Plugin binary */
		TypeImage,		/*!< Item is an image */
		NumTypes
	} ;
	typedef Types Type;

	/*! \brief Constructs a ResourceItem object.
	* \param provider The provider this item belongs to
	* \param name The name used to identify the item towards the user
	* \param baseDir The base directory this item is relative to
	* \param path The path from base directory to this item
	* \param hash A unique hash based on file content, pass QString::null to compute it automatically
	* \param author A string describing the author
	* \param tags A comma-separated list of tags for this item
	* \param size The size of the item, pass -1 to compute it automatically
	* \param lastMod The date and time of the last modification of the item
	*/
	ResourceItem( ResourceProvider * provider,
			const QString & name,
			Type _type,
			BaseDirectory baseDir = BaseWorkingDir,
			const QString & path = QString::null,
			const QString & hash = QString::null,
			const QString & author = QString::null,
			const QString & tags = QString::null,
			int size = -1,
			const QDateTime & lastMod = QDateTime() );
	/*! \brief Copy constructor. */
	ResourceItem( const ResourceItem & item );

	/*! \brief Sets hidden property for the given item model
	* \param hidden A boolean specifying the desired value
	* \param model A pointer to a QAbstractItemModel (allows to use this item
	* for multiple models and views) */
	inline void setHidden( bool hidden, const QAbstractItemModel * model )
	{
		m_hidden[model] = hidden;
	}

	/*! \brief Returns whether item is hidden for the given item model
	* \param model A pointer to a QAbstractItemModel (allows to use this item
	* for multiple models and views)
	* \return true if the item is hidden, false otherwise */
	inline bool isHidden( const QAbstractItemModel * model ) const
	{
		return m_hidden[model];
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

	/*! \brief Fetch data (contents) of the resource via ResourceProvider.
	* \return QByteArray with complete contents of the resource */
	QByteArray fetchData( int _maxSize = -1 ) const
	{
		return m_provider->fetchData( this );
	}

	void reload();

	/*! \brief Returns, whether keywords match certain properties.
	* \return true if all given keywords match name, tags etc. */
	bool keywordMatch( const QStringList & _keywords ) const;

	/*! \brief Tests for equality with another ResourceItem.
	* \return true, if given ResourceItem is equal */
	bool operator==( const ResourceItem & _other ) const;

	/*! \brief Rates equality with another ResourceItem.
	* \return An integer specifying how close the two ResourceItems are (between 0 and about 250) */
	int equalityLevel( const ResourceItem & _other ) const;

	/*! \brief Guesses resource type by various criteria */
	Type guessType() const;

	static const char * mimeKey()
	{
		return "ResourceItem";
	}

	static QString getBaseDirectory( BaseDirectory _bd,
					const ResourceItem * _item = NULL );

	static QString descriptiveTypeName( Type _type );


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
