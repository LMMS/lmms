/*
 * ComboBoxModel.h - declaration of class ComboBoxModel
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

#ifndef COMBOBOX_MODEL_H
#define COMBOBOX_MODEL_H

#include <memory>
#include <utility>
#include <vector>

#include "AutomatableModel.h"
#include "embed.h"


class LMMS_EXPORT ComboBoxModel : public IntModel
{
	Q_OBJECT
public:
	ComboBoxModel( Model* parent = NULL,
					const QString& displayName = QString(),
					bool isDefaultConstructed = false ) :
		IntModel( 0, 0, 0, parent, displayName, isDefaultConstructed )
	{
	}

	virtual ~ComboBoxModel()
	{
		clear();
	}

	void addItem( QString item, std::unique_ptr<PixmapLoader> loader = nullptr );

	void clear();

	int findText( const QString& txt ) const;

	QString currentText() const
	{
		return ( size() > 0 && value() < size() ) ? m_items[value()].first : QString();
	}

	const PixmapLoader* currentData() const
	{
		return m_items[value()].second.get();
	}

	const QString & itemText( int i ) const
	{
		return m_items[qBound<int>( minValue(), i,  maxValue() )].first;
	}

	const PixmapLoader* itemPixmap( int i ) const
	{
		return m_items[qBound<int>( minValue(), i, maxValue() )].second.get();
	}

	int size() const
	{
		return m_items.size();
	}


private:
	typedef std::pair<QString, std::unique_ptr<PixmapLoader> > Item;

	std::vector<Item> m_items;

} ;


#endif
