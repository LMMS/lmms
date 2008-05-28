/*
 * combobox_model.h - class comboBoxModel
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


#ifndef _COMBOBOX_MODEL_H
#define _COMBOBOX_MODEL_H

#include <QtCore/QVector>
#include <QtCore/QPair>

#include "automatable_model.h"
#include "templates.h"

class pixmapLoader;


class comboBoxModel : public intModel
{
	Q_OBJECT
public:
	comboBoxModel( ::model * _parent = NULL ) :
		intModel( 0, 0, 0, _parent )
	{
	}

	void addItem( const QString & _item, pixmapLoader * _loader = NULL );

	void clear( void );

	int findText( const QString & _txt ) const;

	inline const QString & currentText( void ) const
	{
		return( m_items[value()].first );
	}

	inline const pixmapLoader * currentData( void ) const
	{
		return( m_items[value()].second );
	}

	inline const QString & itemText( int _i ) const
	{
		return( m_items[tLimit<int>( _i, minValue(), maxValue() )].
									first );
	}

	inline const pixmapLoader * itemPixmap( int _i ) const
	{
		return( m_items[tLimit<int>( _i, minValue(), maxValue() )].
								second );
	}

	inline int size( void ) const
	{
		return( m_items.size() );
	}

private:
	typedef QPair<QString, pixmapLoader *> item;

	QVector<item> m_items;

} ;


#endif
