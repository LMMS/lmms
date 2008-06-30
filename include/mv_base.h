/*
 * mv_base.h - base for M/V-architecture of LMMS
 *
 * Copyright (c) 2007-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _MV_BASE_H
#define _MV_BASE_H

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "export.h"


class modelView;


class EXPORT model : public QObject
{
	Q_OBJECT
public:
	model( model * _parent, bool _default_constructed = FALSE ) :
		QObject( _parent ),
		m_defaultConstructed( _default_constructed )
	{
	}

	virtual ~model()
	{
	}

	inline bool defaultConstructed( void )
	{
		return( m_defaultConstructed );
	}

	inline model * parentModel( void ) const
	{
		return static_cast<model *>( parent() );
	}

	virtual QString displayName( void ) const
	{
		return QString();
	}

	void setDisplayName( const QString & _displayName )
	{
	}

	virtual QString fullDisplayName( void ) const;

private:
	bool m_defaultConstructed;

signals:
	// emitted if actual data of the model (e.g. values) have changed
	void dataChanged( void );

	// emitted in case new data was not set as it's been equal to old data
	void dataUnchanged( void );

	// emitted if properties of the model (e.g. ranges) have changed
	void propertiesChanged( void );

} ;




class EXPORT modelView
{
public:
	modelView( model * _model, QWidget * _this );
	virtual ~modelView();

	virtual void setModel( model * _model, bool _old_model_valid = TRUE );

	inline model * getModel( void )
	{
		return( m_model.data() );
	}

	inline const model * getModel( void ) const
	{
		return( m_model.data() );
	}

	template<class T>
	T * castModel( void )
	{
		return( dynamic_cast<T *>( getModel() ) );
	}

	template<class T>
	const T * castModel( void ) const
	{
		return( dynamic_cast<const T *>( getModel() ) );
	}


protected:
	// sub-classes can re-implement this to track model-changes
	virtual void modelChanged( void )
	{
	}

	QWidget * widget( void )
	{
		return( m_widget );
	}

	virtual void doConnections( void );


private:
	QWidget * m_widget;
	QPointer<model> m_model;

} ;


#endif

