/*
 * UnifiedResourceProvider.h - header file for UnifiedResourceProvider
 *
 * Copyright (c) 2009-2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _UNIFIED_RESOURCE_PROVIDER_H
#define _UNIFIED_RESOURCE_PROVIDER_H

#include "ResourceProvider.h"
#include "ResourceItem.h"


/*! \brief The UnifiedResourceProvider class merges a given set of ResourceDB's
 * into one ResourceDB.
 *
 * This provider usually is used as the top level provider whose database
 * contains all available resources (various local directories and various
 * online catalogues).
 */

class UnifiedResourceProvider : public ResourceProvider
{
	Q_OBJECT
public:
	UnifiedResourceProvider();
	virtual ~UnifiedResourceProvider();

	void addDatabase( ResourceDB * _db );

	virtual QString providerName() const
	{
		return "UnifiedResourceProvider";
	}

	virtual void updateDatabase();

	virtual int dataSize( const ResourceItem * _item ) const
	{
		if( _item->provider() != this )
		{
			return _item->provider()->dataSize( _item );
		}
		return 0;
	}

	virtual QByteArray fetchData( const ResourceItem * _item,
					int _maxSize = -1 ) const
	{
		if( _item->provider() != this )
		{
			return _item->provider()->fetchData( _item );
		}
		return QByteArray();
	}

	virtual bool isLocal() const
	{
		return false;
	}

	virtual bool cacheDatabase() const
	{
		return false;
	}


private slots:
	void remergeItems();


private:
	QList<ResourceDB *> m_mergedDatabases;

} ;


#endif
