/*
 * WebResourceProvider.cpp - implementation of WebResourceProvider
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

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QUrl>

#include "WebResourceProvider.h"
#include "ResourceDB.h"


WebResourceProvider::DownloadMap WebResourceProvider::m_downloads;


WebResourceProvider::WebResourceProvider( const QString & _url ) :
	ResourceProvider( _url )
{
	database()->init();
}




WebResourceProvider::~WebResourceProvider()
{
	database()->save( localCacheFile() );
}




void WebResourceProvider::updateDatabase()
{
	QBuffer indexBuffer;
	indexBuffer.open( QBuffer::ReadWrite );
	download( url() + "/WebResources/Index", &indexBuffer );

	indexBuffer.seek( 0 );

	QDomDocument doc;
	doc.setContent( &indexBuffer );

	importNodeIntoDB( doc.firstChildElement( "webresources" ),
						database()->topLevelNode() );
}




QByteArray WebResourceProvider::fetchData( const ResourceItem * _item,
							int _maxSize ) const
{
	QBuffer buffer;
	buffer.open( QBuffer::ReadWrite );

	download( url() + "/WebResources/" + _item->hash(), &buffer );

	return buffer.data();
}




void WebResourceProvider::finishDownload( QNetworkReply * _reply )
{
	if( _reply->attribute(
			QNetworkRequest::HttpStatusCodeAttribute ).toInt() == 302 )
	{
		const QUrl newLocation =
			_reply->attribute(
						QNetworkRequest::RedirectionTargetAttribute ).toUrl();
		if( newLocation != _reply->url() )
		{
			qDebug() << "HTTP forwarding to" << newLocation;

			download( newLocation, m_downloads[_reply] );
		}
	}
	else
	{
		m_downloads[_reply]->write( _reply->readAll() );
	}
	m_downloads.remove( _reply );
}




// create a recursive tree from flat items and their path property
ResourceItem::Relation * WebResourceProvider::addRelation(
						ResourceItem::Relation * _parent,
						ResourceItem * _item )
{
	if( _parent == database()->topLevelNode() ||
						_item->path().isEmpty() )
	{
		return new ResourceItem::Relation( _parent, _item );
	}

	const QStringList itemPath = _item->path().split( '/' );

	QString pathComponent = itemPath.first() + "/";
	QString parentPath;

	if( _parent->item() )
	{
		parentPath = _parent->item()->path() + _parent->item()->name();
		const int parentPathSize = parentPath.count( '/' );
		if( parentPathSize+1 >= itemPath.size() )
		{
//			if( _item->path() == parentPath )
			{
				return new ResourceItem::Relation( _parent, _item );
			}
/*			else
			{
				// something went wrong
				return new ResourceItem::Relation( _parent, _item );
			}*/
		}

		pathComponent = itemPath[parentPathSize] + "/";
	}

	ResourceItem::Relation * subParent =
		_parent->findChild( pathComponent, ResourceItem::BaseURL );
	if( subParent == NULL )
	{
		ResourceItem * dirItem =
			new ResourceItem( this,
				pathComponent,
				ResourceItem::TypeDirectory,
				ResourceItem::BaseURL,
				parentPath );
		database()->addItem( dirItem );
		subParent = new ResourceItem::Relation( _parent, dirItem );
	}
	return addRelation( subParent, _item );
}




void WebResourceProvider::importNodeIntoDB( const QDomNode & _n,
											ResourceItem::Relation * _parent )
{
	QDomNode n = _n;
	while( !n.isNull() )
	{
		QString path = n.firstChildElement( "dir" ).text();
		ResourceItem::Type type = ResourceItem::TypeUnknown;

		if( !path.isEmpty() )
		{
			path += "/";
		}
		if( n.nodeName() == "webresources" )
		{
			type = ResourceItem::TypeDirectory;
		}

		ResourceItem * item =
			new ResourceItem( this,
				n.firstChildElement( "name" ).text(),
				type,
				ResourceItem::BaseURL,
				path,
				n.firstChildElement( "hash" ).text(),
				n.firstChildElement( "author" ).text(),
				n.firstChildElement( "tags" ).text(),
				n.firstChildElement( "size" ).text().toInt(),
				QDateTime::fromString(
					n.firstChildElement( "date" ).text(), Qt::ISODate ) );
		database()->addItem( item );

		ResourceItem::Relation * relation = addRelation( _parent, item );
		if( n.nodeName() != "file" )
		{
			importNodeIntoDB( n.firstChild(), relation );
		}
		n = n.nextSibling();
	}
}





void WebResourceProvider::download( const QUrl & _url, QBuffer * _target ) const
{
	ProgressTrackingNetworkAccessManager netAccMgr( NULL );
	connect( &netAccMgr, SIGNAL( finished( QNetworkReply * ) ),
				this, SLOT( finishDownload( QNetworkReply * ) ) );

	QNetworkReply * activeDownload = netAccMgr.get( QNetworkRequest( _url ) );
	m_downloads[activeDownload] = _target;

	// wait for the download to finish
	while( m_downloads.contains( activeDownload ) )
	{
		QCoreApplication::instance()->processEvents();
	}
}




void WebResourceProvider::downloadAsync( const QUrl & _url,
											QBuffer * _target ) const
{
	ProgressTrackingNetworkAccessManager * netAccMgr =
		new ProgressTrackingNetworkAccessManager( NULL );
	connect( netAccMgr, SIGNAL( finished( QNetworkReply * ) ),
				this, SLOT( finishDownload( QNetworkReply * ) ) );
	connect( netAccMgr, SIGNAL( finished( QNetworkReply * ) ),
				netAccMgr, SLOT( deleteLater() ) );

	QNetworkReply * activeDownload = netAccMgr->get( QNetworkRequest( _url ) );
	m_downloads[activeDownload] = _target;
}



#include "moc_WebResourceProvider.cxx"
#include "moc_ProgressTrackingNetworkAccessManager.cxx"

