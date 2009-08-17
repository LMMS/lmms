/*
 * TreeRelation.h - header file for TreeRelation
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

#ifndef _TREE_RELATION_H
#define _TREE_RELATION_H

#include <QtCore/QList>

class QAbstractItemModel;

#define foreachTreeRelation(list)										\
		for(typename TreeRelation::List::Iterator it=list.begin();		\
					it!=list.end();++it)

#define foreachConstTreeRelation(list)									\
		for(typename TreeRelation::List::ConstIterator it=list.begin();	\
						it!=list.end();++it)


template<class T>
class TreeRelation
{
public:
	typedef TreeRelation<T> ThisTreeRelation;
	typedef QList<ThisTreeRelation *> List;

	TreeRelation( ThisTreeRelation * _parent = NULL, T * _item = NULL ) :
		m_parent( _parent ),
		m_temporaryMarker( false ),
		m_item( _item )
	{
		if( parent() )
		{
			parent()->addChild( this );
		}
		if( item() )
		{
			item()->setRelation( this );
		}
	}

	~TreeRelation()
	{
		foreachTreeRelation( children() )
		{
			delete *it;
		}
		if( item() )
		{
			item()->setRelation( NULL );
		}
		if( parent() )
		{
			parent()->removeChild( this );
		}
	}

	int rowCount( const QAbstractItemModel * _model = NULL ) const
	{
		int rc = 0;
		foreachConstTreeRelation( children() )
		{
			if( !(*it)->item()->isHidden( _model ) )
			{
				++rc;
			}
		}
		return rc;
	}

	ThisTreeRelation * getChild( int _row,
									const QAbstractItemModel * _model = NULL )
	{
		int rc = 0;
		foreachTreeRelation( children() )
		{
			if( !(*it)->item()->isHidden( _model ) )
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


	int row( const QAbstractItemModel * _model = NULL ) const
	{
		if( !parent() )
		{
			return 0;
		}

		int row = 0;
		foreachConstTreeRelation( parent()->children() )
		{
			if( !(*it)->item()->isHidden( _model ) )
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

	void addChild( ThisTreeRelation * _it )
	{
		children().push_back( _it );
	}

	void removeChild( ThisTreeRelation * _it )
	{
		children().removeAll( _it );
	}

	TreeRelation::List & children()
	{
		return m_children;
	}

	const TreeRelation::List & children() const
	{
		return m_children;
	}

	ThisTreeRelation * findChild( const QString & _name, int _base_dir )
	{
		if( _name.isEmpty() )
		{
			return NULL;
		}

		const int hash = qHash( _name );

		foreachTreeRelation( children() )
		{
			ThisTreeRelation * treeRel = *it;
			const T * item = treeRel->item();
			if( item &&
				item->nameHash() == hash &&
				item->name() == _name &&
				item->baseDir() == _base_dir )
			{
				return treeRel;
			}
		}
		return NULL;
	}

	const ThisTreeRelation * parent() const
	{
		return m_parent;
	}

	ThisTreeRelation * parent()
	{
		return m_parent;
	}

	void setParent( ThisTreeRelation * _parent )
	{
		m_parent = _parent;
	}

	T * item()
	{
		return m_item;
	}

	const T * item() const
	{
		return m_item;
	}

	bool temporaryMarker() const
	{
		return m_temporaryMarker;
	}

	void setTemporaryMarker( bool _on )
	{
		m_temporaryMarker = _on;
	}


private:
	// hide copy-ctor
	TreeRelation( const ThisTreeRelation & ) { }

	ThisTreeRelation * m_parent;
	TreeRelation::List m_children;

	bool m_temporaryMarker;

	T * m_item;

} ;


#endif
