/*
 * UnknownResourceProvider.h - header file for UnknownResourceProvider
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _UNKNOWN_RESOURCE_PROVIDER_H
#define _UNKNOWN_RESOURCE_PROVIDER_H

#include "ResourceProvider.h"


/*! \brief The UnknownResourceProvider class is a dummy provider used as
 * fallback when an unknown ResourceProvider has been specified.
 */

class UnknownResourceProvider : public ResourceProvider
{
public:
	UnknownResourceProvider( const QString & url ) :
		ResourceProvider( url )
	{
	}

	virtual ~UnknownResourceProvider()
	{
	}

	virtual QString providerName() const
	{
		return "UnknownResourceProvider";
	}

	virtual void updateDatabase()
	{
	}

	virtual int dataSize( const ResourceItem * ) const
	{
		return 0;
	}

	virtual QByteArray fetchData( const ResourceItem *, int = -1 ) const
	{
		return QByteArray();
	}

	virtual bool isLocal() const
	{
		return true;
	}

} ;


#endif
