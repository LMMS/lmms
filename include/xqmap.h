/*
 * XQMap.h - eXtended QMap template for Qt3
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
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


#ifndef XQMAP_H
#define XQMAP_H

#include <qmap.h>




template<class Key, class T>
class XQMapPrivate : public QMapPrivate<Key, T>
{
public:
	typedef QMapConstIterator<Key, T> ConstIterator;
	typedef QMapNode<Key, T> * NodePtr;

	XQMapPrivate( void );
	XQMapPrivate( const XQMapPrivate<Key, T> * _map );

	ConstIterator lowerBound( const Key & _k ) const;

};




template<class Key, class T>
Q_INLINE_TEMPLATES XQMapPrivate<Key, T>::XQMapPrivate( void )
{
}




template<class Key, class T>
Q_INLINE_TEMPLATES XQMapPrivate<Key, T>::XQMapPrivate(
					const XQMapPrivate<Key, T> * _map ) :
	QMapPrivate<Key, T>( _map )
{
}




template<class Key, class T>
Q_INLINE_TEMPLATES Q_TYPENAME XQMapPrivate<Key, T>::ConstIterator
	XQMapPrivate<Key, T>::lowerBound( const Key & _k ) const
{
	QMapNodeBase * y = QMapPrivate<Key, T>::header;         // Last node
	QMapNodeBase * x = QMapPrivate<Key, T>::header->parent; // Root node

	while( x != 0 ) {
		// If as k <= key(x) go left
		if( !( QMapPrivate<Key, T>::key( x ) < _k ) )
		{
			y = x;
			x = x->left;
		}
		else
		{
			x = x->right;
		}
	}

	// Was _k bigger then the biggest element of the tree? Return end()
	if( y == QMapPrivate<Key, T>::header )
	{
		return( ConstIterator( QMapPrivate<Key, T>::header ) );
	}
	return( ConstIterator( (NodePtr)y ) );
}




template<class Key, class T>
class XQMap
{
public:
	/**
	 * Typedefs
	 */
	typedef Key key_type;
	typedef T mapped_type;
	typedef QPair<const key_type, mapped_type> value_type;
	typedef value_type * pointer;
	typedef const value_type * const_pointer;
	typedef value_type & reference;
	typedef const value_type & const_reference;
#ifndef QT_NO_STL
	typedef ptrdiff_t difference_type;
#else
	typedef int difference_type;
#endif
	typedef size_t size_type;
	typedef QMapIterator<Key, T> iterator;
	typedef QMapConstIterator<Key, T> const_iterator;
	typedef QPair<iterator, bool> insert_pair;

	typedef QMapIterator<Key, T> Iterator;
	typedef QMapConstIterator<Key, T> ConstIterator;
	typedef T ValueType;
	typedef XQMapPrivate<Key, T> Priv;

	/**
	 * API
	 */
	XQMap( void )
	{
		m_sh = new XQMapPrivate<Key, T>;
	}
	XQMap( const XQMap<Key, T> & _m )
	{
		m_sh = _m.m_sh;
		m_sh->ref();
	}

#ifndef QT_NO_STL
	XQMap( const std::map<Key, T> & _m )
	{
		m_sh = new XQMapPrivate<Key, T>;
		Q_TYPENAME std::map<Key, T>::const_iterator it = _m.begin();
		for( ; it != _m.end(); ++it )
		{
			value_type p( ( *it ).first, ( *it ).second );
			insert( p );
		}
	}
#endif
	~XQMap()
	{
		if( m_sh->deref() )
		{
			delete m_sh;
		}
	}
	XQMap<Key, T> & operator=( const XQMap<Key, T> & _m );
#ifndef QT_NO_STL
	XQMap<Key, T> & operator=( const std::map<Key, T> & _m )
	{
		clear();
		Q_TYPENAME std::map<Key, T>::const_iterator it = _m.begin();
		for( ; it != _m.end(); ++it )
		{
			value_type p( ( *it ).first, ( *it ).second );
			insert( p );
		}
		return( *this );
    }
#endif

	iterator begin( void )
	{
		detach();
		return( m_sh->begin() );
	}
	iterator end( void )
	{
		detach();
		return( m_sh->end() );
	}
	const_iterator begin( void ) const
	{
		return( ( (const Priv *)m_sh )->begin() );
	}
	const_iterator end( void ) const
	{
		return( ( (const Priv *)m_sh )->end() );
	}
	const_iterator constBegin( void ) const
	{
		return( begin() );
	}
	const_iterator constEnd( void ) const
	{
		return( end() );
	}

	iterator replace( const Key & _k, const T & _v )
	{
		remove( _k );
		return( insert( _k, _v ) );
	}

	size_type size( void ) const
	{
		return( m_sh->node_count );
	}
	bool empty( void ) const
	{
		return( m_sh->node_count == 0 );
	}
	QPair<iterator, bool> insert( const value_type & _x );

	void erase( iterator _it )
	{
		detach();
		m_sh->remove( _it );
	}
	void erase( const key_type & _k );
	size_type count( const key_type & _k ) const;
	T & operator[]( const Key & _k );
	void clear( void );

	iterator find( const Key & _k )
	{
		detach();
		return( iterator( m_sh->find( _k ).node ) );
	}

	iterator lowerBound( const Key & _k )
	{
		detach();
		return( iterator( m_sh->lowerBound( _k ).node ) );
	}

	const_iterator find( const Key & _k ) const
	{
		return( m_sh->find( _k ) );
	}

	const_iterator lowerBound( const Key & _k ) const
	{
		return( m_sh->lowerBound( _k ) );
	}

	const T & operator[]( const Key & _k ) const
	{
		QT_CHECK_INVALID_MAP_ELEMENT;
		return( m_sh->find( _k ).data() );
	}
	bool contains( const Key & _k ) const
	{
		return( find( _k ) != end() );
	}

	size_type count( void ) const
	{
		return( m_sh->node_count );
	}

	QValueList<Key> keys( void ) const
	{
		QValueList<Key> r;
		for( const_iterator i = begin(); i != end(); ++i )
		{
			r.append( i.key() );
		}
		return( r );
	}

	QValueList<T> values( void ) const
	{
		QValueList<T> r;
		for( const_iterator i = begin(); i != end(); ++i )
		r.append( *i );
		return( r );
	}

	bool isEmpty( void ) const
	{
		return( m_sh->node_count == 0 );
	}

	iterator insert( const Key & _key, const T & _value,
						bool _overwrite = TRUE );
	void remove( iterator _it )
	{
		detach();
		m_sh->remove( _it );
	}
	void remove( const Key & _k );

#if defined( Q_FULL_TEMPLATE_INSTANTIATION )
	bool operator==( const XQMap<Key, T> & ) const
	{
		return( FALSE );
	}
#ifndef QT_NO_STL
	bool operator==( const std::map<Key, T> & ) const
	{
		return( FALSE );
	}
#endif
#endif


protected:
	/**
	 * Helpers
	 */
	void detach( void )
	{
		if( m_sh->count > 1 )
		{
			detachInternal();
		}
	}

	Priv * m_sh;


private:
	void detachInternal( void );

	friend class QDeepCopy< XQMap<Key, T> >;

} ;




template<class Key, class T>
Q_INLINE_TEMPLATES XQMap<Key, T> & XQMap<Key, T>::operator=(
						const XQMap<Key, T> & _m )
{
	_m.m_sh->ref();
	if( m_sh->deref() )
	{
		delete m_sh;
	}
	m_sh = _m.m_sh;
	return( *this );
}




template<class Key, class T>
Q_INLINE_TEMPLATES Q_TYPENAME XQMap<Key, T>::insert_pair
	XQMap<Key, T>::insert( const Q_TYPENAME XQMap<Key, T>::value_type & _x )
{
	detach();
	size_type n = size();
	iterator it = m_sh->insertSingle( _x.first );
	bool inserted = FALSE;
	if( n < size() )
	{
		inserted = TRUE;
		it.data() = _x.second;
	}
	return( QPair<iterator, bool>( it, inserted ) );
}




template<class Key, class T>
Q_INLINE_TEMPLATES void XQMap<Key, T>::erase( const Key & _k )
{
	detach();
	iterator it( m_sh->find( _k ).node );
	if( it != end() )
	{
		m_sh->remove( it );
	}
}

template<class Key, class T>
Q_INLINE_TEMPLATES Q_TYPENAME XQMap<Key, T>::size_type
	XQMap<Key, T>::count( const Key & _k ) const
{
	const_iterator it( m_sh->find( _k ).node );
	if( it != end() )
	{
		size_type c = 0;
		while( it != end() )
		{
			++it;
			++c;
		}
		return( c );
	}
	return( 0 );
}




template<class Key, class T>
Q_INLINE_TEMPLATES T & XQMap<Key, T>::operator[]( const Key & _k )
{
	detach();
	QMapNode<Key, T> * p = m_sh->find( _k ).node;
	if( p != m_sh->end().node )
	{
		return( p->data );
	}
	return( insert( _k, T() ).data() );
}




template<class Key, class T>
Q_INLINE_TEMPLATES void XQMap<Key, T>::clear( void )
{
	if( m_sh->count == 1 )
	{
		m_sh->clear();
	}
	else
	{
		m_sh->deref();
		m_sh = new XQMapPrivate<Key, T>;
	}
}




template<class Key, class T>
Q_INLINE_TEMPLATES Q_TYPENAME XQMap<Key, T>::iterator XQMap<Key, T>::insert(
			const Key & _key, const T & _value, bool _overwrite )
{
	detach();
	size_type n = size();
	iterator it = m_sh->insertSingle( _key );
	if( _overwrite || n < size() )
	{
		it.data() = _value;
	}
	return( it );
}




template<class Key, class T>
Q_INLINE_TEMPLATES void XQMap<Key, T>::remove( const Key & _k )
{
	detach();
	iterator it( m_sh->find( _k ).node );
	if( it != end() )
	{
		m_sh->remove( it );
	}
}




template<class Key, class T>
Q_INLINE_TEMPLATES void XQMap<Key, T>::detachInternal( void )
{
	m_sh->deref();
	m_sh = new XQMapPrivate<Key, T>( m_sh );
}




#ifndef QT_NO_DATASTREAM
template<class Key, class T>
Q_INLINE_TEMPLATES QDataStream & operator>>( QDataStream & _s,
							XQMap<Key, T> & _m )
{
	_m.clear();
	Q_UINT32 c;
	_s >> c;
	for( Q_UINT32 i = 0; i < c; ++i )
	{
		Key k;
		T t;
		_s >> k >> t;
		_m.insert( k, t );
		if( _s.atEnd() )
		{
			break;
		}
	}
	return( _s );
}




template<class Key, class T>
Q_INLINE_TEMPLATES QDataStream & operator<<( QDataStream & _s,
						const XQMap<Key, T> & _m )
{
	_s << (Q_UINT32)_m.size();
	QMapConstIterator<Key, T> it = _m.begin();
	for( ; it != _m.end(); ++it )
	{
		_s << it.key() << it.data();
	}
	return( _s );
}
#endif




#endif
