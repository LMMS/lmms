/*
 * ComboBoxModel.cpp - implementation of ComboBoxModel
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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

#include "ComboBoxModel.h"
#include "embed.h"



void ComboBoxModel::addItem( const QString& item, PixmapLoader* loader )
{
	m_items.push_back( qMakePair( item, loader ) );
	setRange( 0, m_items.size() - 1 );
}




void ComboBoxModel::clear()
{
	setRange( 0, 0 );
	foreach( const Item& i, m_items )
	{
		delete i.second;
	}

	m_items.clear();

	emit propertiesChanged();
}




int ComboBoxModel::findText( const QString& txt ) const
{
	for( QVector<Item>::ConstIterator it = m_items.begin(); it != m_items.end(); ++it )
	{
		if( ( *it ).first == txt )
		{
			return it - m_items.begin();
		}
	}
	return -1; 
}



#include "moc_ComboBoxModel.cxx"

