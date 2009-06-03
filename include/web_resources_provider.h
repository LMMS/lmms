/*
 * web_resources_provider.h - header file for WebResourcesProvider
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

#ifndef _WEB_RESOURCES_PROVIDER_H
#define _WEB_RESOURCES_PROVIDER_H

#include <QtCore/QBuffer>
#include <QtXml/QDomNode>

#include "resources_provider.h"
#include "resources_item.h"


class QHttp;


class WebResourcesProvider : public ResourcesProvider
{
	Q_OBJECT
public:
	WebResourcesProvider( const QString & _url );
	virtual ~WebResourcesProvider();

	virtual QString providerName( void ) const
	{
		return "WebResourcesProvider";
	}

	virtual void updateDatabase( void );

	virtual int dataSize( const ResourcesItem * _item ) const
	{
		// asume that the size we have set before from the web
		// catalogue is correct
		return _item->size();
	}
	virtual QByteArray fetchData( const ResourcesItem * _item,
					int _maxSize = -1 ) const;

	virtual bool isLocal( void ) const
	{
		return false;
	}


private slots:
	void finishDownload( int _id, bool );


private:
	ResourcesTreeItem * addTreeItem( ResourcesTreeItem * _parent,
						ResourcesItem * _item );
	void importNodeIntoDB( const QDomNode & n,
						ResourcesTreeItem * _parent );
	void download( const QString & _path, QBuffer * _target,
						bool _wait = false ) const;

	QHttp * m_http;
	QBuffer m_indexBuffer;
	static QList<int> m_downloadIDs;

} ;


#endif
