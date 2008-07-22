/*
 * combobox_model.cpp - implementation of comboBoxModel
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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
 

#include "combobox_model.h"
#include "embed.h"



void comboBoxModel::addItem( const QString & _item, pixmapLoader * _pl )
{
	m_items.push_back( qMakePair( _item, _pl ) );
	setRange( 0, m_items.size() - 1 );
}




void comboBoxModel::clear( void )
{
	setRange( 0, 0 );
	foreach( const item & _i, m_items )
	{
		delete _i.second;
	}
	m_items.clear();
	emit propertiesChanged();
}




int comboBoxModel::findText( const QString & _txt ) const
{
	for( QVector<item>::const_iterator it = m_items.begin();
						it != m_items.end(); ++it )
	{
		if( ( *it ).first == _txt )
		{
			return( it - m_items.begin() );
		}
	}
	return( -1 ); 
}




#include "moc_combobox_model.cxx"

