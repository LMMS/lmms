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

#include <QtCore/QByteArray>

#include "resources_provider.h"
#include "resources_item.h"


class WebResourcesProvider : public ResourcesProvider
{
public:
	WebResourcesProvider( const QString & _url );
	virtual ~WebResourcesProvider()
	{
	}

	virtual ResourcesDB * createResourcesDB( void );
	virtual int dataSize( const ResourcesItem * _item ) const
	{
		// asume that the size we have set before from the web
		// catalogue is correct
		return _item->size();
	}
	virtual QByteArray fetchData( const ResourcesItem * _item,
					int _maxSize = -1 ) const;


private:

} ;


#endif
