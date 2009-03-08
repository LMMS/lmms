/*
 * web_resources_provider.cpp - implementation of WebResourcesProvider^
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtNetwork/QHttp>

#include "web_resources_provider.h"
#include "resources_db.h"


QList<int> WebResourcesProvider::m_downloadIDs;


WebResourcesProvider::WebResourcesProvider( const QString & _url ) :
	ResourcesProvider( _url ),
	m_http( NULL ),
	m_indexBuffer()
{
	QUrl u( _url );
	m_http = new QHttp( u.host(), u.port() > 0 ? u.port() : 80, this );
	connect( m_http, SIGNAL( requestFinished( int, bool ) ),
			this, SLOT( finishDownload( int, bool ) ) );

	database()->init();
}




WebResourcesProvider::~WebResourcesProvider()
{
	database()->save( localCacheFile() );
}




void WebResourcesProvider::updateDatabase( void )
{
	m_indexBuffer.close();
	m_indexBuffer.open( QBuffer::ReadWrite );
	download( "/WebResources/Index", &m_indexBuffer, true );

	m_indexBuffer.seek( 0 );

	QDomDocument doc;
	doc.setContent( &m_indexBuffer );

	importNodeIntoDB( doc.firstChildElement( "webresources" ),
						database()->topLevelNode() );
}




QByteArray WebResourcesProvider::fetchData( const ResourcesItem * _item,
							int _maxSize ) const
{
	QBuffer buffer;
	buffer.open( QBuffer::ReadWrite );

	download( "/WebResources/" + _item->hash(), &buffer, true );

	return buffer.data();
}




void WebResourcesProvider::finishDownload( int _id, bool a )
{
	m_downloadIDs.removeAll( _id );
}




void WebResourcesProvider::importNodeIntoDB( const QDomNode & _n,
						ResourcesTreeItem * _parent )
{
	QDomNode n = _n;
	while( !n.isNull() )
	{
		ResourcesItem * item =
			new ResourcesItem( this,
				n.firstChildElement( "name" ).text(),
				ResourcesItem::TypeUnknown,
				ResourcesItem::BaseURL,
				n.firstChildElement( "dir" ).text(),
				n.firstChildElement( "hash" ).text(),
				n.firstChildElement( "tags" ).text(),
				n.firstChildElement( "size" ).text().toInt() );
		database()->addItem( item );

		ResourcesTreeItem * treeItem =
					new ResourcesTreeItem( _parent, item );
		if( n.nodeName() != "file" )
		{
			importNodeIntoDB( n.firstChild(), treeItem );
		}
		n = n.nextSibling();
	}
}





void WebResourcesProvider::download( const QString & _path,
					QBuffer * _target, bool _wait ) const
{
	const int id = m_http->get( _path, _target );
	m_downloadIDs << id;

	if( _wait )
	{
		while( m_downloadIDs.contains( id ) )
		{
			QCoreApplication::instance()->processEvents();
		}
	}
}



#include "moc_web_resources_provider.cxx"

