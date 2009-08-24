/*
 * LocalResourceProvider.h - header file for LocalResourceProvider
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

#ifndef _LOCAL_RESOURCE_PROVIDER_H
#define _LOCAL_RESOURCE_PROVIDER_H

#include <QtCore/QFileSystemWatcher>
#include <QtCore/QStringList>

#include "ResourceProvider.h"
#include "ResourceItem.h"


class LocalResourceProvider : public ResourceProvider
{
	Q_OBJECT
public:
	LocalResourceProvider( ResourceItem::BaseDirectory _baseDir,
						const QString & _dir );
	virtual ~LocalResourceProvider();

	virtual QString providerName() const
	{
		return "LocalResourceProvider";
	}

	virtual void updateDatabase();

	virtual int dataSize( const ResourceItem * _item ) const;
	virtual QByteArray fetchData( const ResourceItem * _item,
						int _maxSize = -1 ) const;

	virtual bool isLocal() const
	{
		return true;
	}


private slots:
	void addDirectory( const QString & _path );
	void removeDirectory( const QString & _path );
	void reloadDirectory( const QString & _path );


private:
	void readDir( const QString & _dir, ResourceItem::Relation * _parent );

	ResourceItem::BaseDirectory m_baseDir;
	const QString m_dir;

	QStringList m_scannedFolders;

	QFileSystemWatcher m_watcher;

} ;


#endif
