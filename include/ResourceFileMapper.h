/*
 * ResourceFileMapper.h - header file for ResourceFileMapper, an inline helper
 *                        class allowing local access to any kind of ResourceItem's
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

#ifndef _RESOURCE_FILE_MAPPER_H
#define _RESOURCE_FILE_MAPPER_H

#include <QtCore/QTemporaryFile>

#include "ResourceItem.h"


class ResourceFileMapper
{
public:
	ResourceFileMapper( const ResourceItem * _item ) :
		m_item( _item ),
		m_tempFile( m_item->isLocalResource() ?
				NULL : new QTemporaryFile )
	{
		if( m_tempFile && m_tempFile->open() )
		{
			m_tempFile->write( m_item->fetchData() );
			m_tempFile->flush();
		}
	}

	~ResourceFileMapper()
	{
		delete m_tempFile;
	}

	QString fileName() const
	{
		return m_tempFile ? m_tempFile->fileName() : m_item->fullName();
	}


private:
	const ResourceItem * m_item;
	QTemporaryFile * m_tempFile;

} ;


#endif
