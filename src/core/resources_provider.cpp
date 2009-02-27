/*
 * resources_provider.cpp - implementation of ResourcesProvider
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


#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>

#include "resources_provider.h"
#include "resources_db.h"
#include "config_mgr.h"


ResourcesProvider::ResourcesProvider( const QString & _url ) :
	m_database( new ResourcesDB( this ) ),
	m_url( _url )
{
}




ResourcesProvider::~ResourcesProvider()
{
	delete m_database;
}




QString ResourcesProvider::localCatalogueFile( void ) const
{
	const QString dir = configManager::inst()->workingDir() +
					"catalogs" + QDir::separator();
	if( !QDir( dir ).exists() )
	{
		QDir().mkpath( dir );
	}

	QCryptographicHash h( QCryptographicHash::Md5 );

	h.addData( QString( providerName() + url() ).toUtf8() );

	return dir + h.result().toHex();
}



#include "moc_resources_provider.cxx"

