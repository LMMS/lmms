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

#ifndef LMMS_COMBOBOX_MODEL_H
#define LMMS_COMBOBOX_MODEL_H

#include <memory>
#include <utility>
#include <vector>

#include "AutomatableModel.h"
#include "embed.h"

namespace lmms
{

class LMMS_EXPORT ComboBoxModel : public IntModel
{
	Q_OBJECT
	MODEL_IS_VISITABLE
public:
	ComboBoxModel( Model* parent = nullptr,
					const QString& displayName = QString(),
					bool isDefaultConstructed = false ) :
		IntModel( 0, 0, 0, parent, displayName, isDefaultConstructed )
	{
		setJournalling(false);
	}

	void addItem( QString item, std::unique_ptr<PixmapLoader> loader = nullptr );

	void replaceItem(std::size_t index, QString item, std::unique_ptr<PixmapLoader> loader = nullptr);

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
		return m_items[std::clamp(i, minValue(), maxValue())].first;
	}

	const PixmapLoader* itemPixmap( int i ) const
	{
		return m_items[std::clamp(i, minValue(), maxValue())].second.get();
	}

	int size() const
	{
		return m_items.size();
	}


private:
	using Item = std::pair<QString, std::unique_ptr<PixmapLoader>>;

	std::vector<Item> m_items;

} ;

} // namespace lmms

#endif // LMMS_COMBOBOX_MODEL_H
