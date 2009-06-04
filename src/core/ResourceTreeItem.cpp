/*
 * ResourceTreeItem.cpp - implementation of ResourceTreeItem
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


#include <QtCore/QHash>

#include "ResourceTreeItem.h"


ResourceTreeItem::ResourceTreeItem( ResourceTreeItem * _parent,
					ResourceItem * _item ) :
	m_parent( _parent ),
	m_hidden( false ),
	m_temporaryMarker( false ),
	m_item( _item )
{
	if( m_parent )
	{
		m_parent->addChild( this );
	}
	if( m_item )
	{
		m_item->setTreeItem( this );
	}
}




ResourceTreeItem::~ResourceTreeItem()
{
	foreachResourceTreeItem( m_children )
	{
		delete *it;
	}
	if( m_item )
	{
		m_item->setTreeItem( NULL );
	}
	if( m_parent )
	{
		m_parent->removeChild( this );
	}
}




int ResourceTreeItem::rowCount( void ) const
{
	int rc = 0;
	foreachConstResourceTreeItem( m_children )
	{
		if( !(*it)->isHidden() )
		{
			++rc;
		}
	}
	return rc;
}




ResourceTreeItem * ResourceTreeItem::getChild( int _row )
{
	int rc = 0;
	foreachResourceTreeItem( m_children )
	{
		if( !(*it)->isHidden() )
		{
			if( rc == _row )
			{
				return *it;
			}
			++rc;
		}
	}
	return NULL;
}




int ResourceTreeItem::row( void ) const
{
	if( !m_parent )
	{
		return 0;
	}

	int row = 0;
	foreachConstResourceTreeItem( m_parent->m_children )
	{
		if( !(*it)->isHidden() )
		{
			if( *it == this )
			{
				return row;
			}
			++row;
		}
	}
	return 0;
}




ResourceTreeItem * ResourceTreeItem::findChild(
					const QString & _name,
					ResourceItem::BaseDirectory _base_dir )
{
	if( _name.isNull() || _name.isEmpty() )
	{
		return NULL;
	}

	const int hash = qHash( _name );

	foreachResourceTreeItem( m_children )
	{
		ResourceTreeItem * rti = *it;
		if( rti->item() &&
			rti->item()->nameHash() == hash &&
			rti->item()->name() == _name &&
			rti->item()->baseDir() == _base_dir )
		{
			return rti;
		}
	}
	return NULL;
}


