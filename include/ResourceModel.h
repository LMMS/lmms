/*
 * ResourceModel.h - base class for all resource models
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

#ifndef _RESOURCE_MODEL_H
#define _RESOURCE_MODEL_H

#include <QtCore/QAbstractItemModel>

#include "lmms_basics.h"
#include "ResourceDB.h"


class ResourceModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ResourceModel( ResourceDB * _db, QObject * _parent = NULL );
	virtual ~ResourceModel()
	{
	}

	virtual QVariant data( const QModelIndex & _idx,
                                        int _role = Qt::DisplayRole ) const;

	virtual Qt::ItemFlags flags( const QModelIndex & _index ) const;

	virtual int columnCount( const QModelIndex & _parent = QModelIndex() ) const
	{
		return 1;
	}

	// return list of possible MIME types for items in this model
	virtual QStringList mimeTypes() const;

	// used for drag'n'drop - return proper MIME data for indexes
	virtual QMimeData * mimeData( const QModelIndexList & _indexes ) const;

	// return ResourceItem::Relation belonging to a certain index
	static inline ResourceItem::Relation * relation( const QModelIndex & _idx )
	{
		return static_cast<ResourceItem::Relation *>( _idx.internalPointer() );
	}

	// return ResourceItem belonging to a certain index
	static inline ResourceItem * item( const QModelIndex & _idx )
	{
		return relation( _idx )->item();
	}

	int totalItems() const;
	int shownItems() const;

	QStringList keywordFilter() const
	{
		return m_keywordFilter;
	}

	ResourceItem::Type typeFilter() const
	{
		return m_typeFilter;
	}


public slots:
	virtual void updateFilters() = 0;
	void setKeywordFilter( const QString & _keywords );
	void setTypeFilter( ResourceItem::Type _type = ResourceItem::TypeUnknown );


protected:
	inline bool itemMatchesFilter( const ResourceItem & _item ) const
	{
		return ( likely( typeFilter() == ResourceItem::TypeUnknown ) ||
					typeFilter() == _item.type() ) &&
				( likely( m_keywordFilterSet == false ) ||
						_item.keywordMatch( keywordFilter() ) );
	}

	ResourceDB * db() const
	{
		return m_db;
	}


private:
	ResourceDB * m_db;
	QStringList m_keywordFilter;
	bool m_keywordFilterSet;
	ResourceItem::Type m_typeFilter;


signals:
	void itemsChanged();

} ;

#endif
