/*
 * ComboBoxModel.h - declaration of class ComboBoxModel
 *
 * Copyright (c) 2008-2011 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef _COMBOBOX_MODEL_H
#define _COMBOBOX_MODEL_H

#include <QtCore/QVector>
#include <QtCore/QPair>

#include "AutomatableModel.h"
#include "templates.h"

class PixmapLoader;


class EXPORT ComboBoxModel : public IntModel
{
	Q_OBJECT
public:
	ComboBoxModel( Model * _parent = NULL,
			const QString & _display_name = QString(),
				bool _default_constructed = false ) :
		IntModel( 0, 0, 0, _parent, _display_name,
							_default_constructed )
	{
	}

	virtual ~ComboBoxModel()
	{
		clear();
	}

	void addItem( const QString & _item, PixmapLoader * _loader = NULL );

	void clear();

	int findText( const QString & _txt ) const;

	inline QString currentText() const
	{
		return ( size() > 0 && value() < size() ) ? m_items[value()].first : QString();
	}

	inline const PixmapLoader * currentData() const
	{
		return m_items[value()].second;
	}

	inline const QString & itemText( int _i ) const
	{
		return m_items[tLimit<int>( _i, minValue(), maxValue() )].first;
	}

	inline const PixmapLoader * itemPixmap( int _i ) const
	{
		return m_items[tLimit<int>( _i, minValue(), maxValue() )].second;
	}

	inline int size() const
	{
		return m_items.size();
	}

private:
	typedef QPair<QString, PixmapLoader *> Item;

	QVector<Item> m_items;

} ;


#endif
