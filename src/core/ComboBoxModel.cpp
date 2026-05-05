/*
 * ComboBoxModel.cpp - implementation of ComboBoxModel
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <cassert>

namespace lmms
{

void ComboBoxModel::addItem(QString item, std::unique_ptr<PixmapLoader> loader)
{
	m_items.emplace_back(std::move(item), std::move(loader));
	setRange( 0, m_items.size() - 1 );
}


void ComboBoxModel::replaceItem(std::size_t index, QString item, std::unique_ptr<PixmapLoader> loader)
{
	assert(index < m_items.size());
	m_items[index] = Item(std::move(item), std::move(loader));
	emit propertiesChanged();
}


void ComboBoxModel::clear()
{
	setRange( 0, 0 );

	m_items.clear();

	emit propertiesChanged();
}




int ComboBoxModel::findText( const QString& txt ) const
{
	for( auto it = m_items.begin(); it != m_items.end(); ++it )
	{
		if( ( *it ).first == txt )
		{
			return it - m_items.begin();
		}
	}
	return -1; 
}


} // namespace lmms


