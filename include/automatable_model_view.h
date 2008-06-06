/*
 * automatable_model_view.h - class automatableModelView
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


#ifndef _AUTOMATABLE_MODEL_VIEW_H
#define _AUTOMATABLE_MODEL_VIEW_H

#include "mv_base.h"
#include "automatable_model.h"


class QMenu;



class EXPORT automatableModelView : public modelView
{
public:
	automatableModelView( ::model * _model ) :
		modelView( _model ),
		m_description( QString::null ),
		m_unit( QString::null )
	{
	}

	// some basic functions for convenience
	automatableModel * modelUntyped( void )
	{
		return( castModel<automatableModel>() );
	}

	const automatableModel * modelUntyped( void ) const
	{
		return( castModel<automatableModel>() );
	}

	virtual void setModel( model * _model, bool _old_model_valid = TRUE );

	template<typename T>
	inline T value( void ) const
	{
		return( modelUntyped() ? modelUntyped()->value<T>() : 0 );
	}

	inline void setValue( const float _value )
	{
		if( modelUntyped() )
		{
			modelUntyped()->setValue( _value );
		}
	}


	inline void setDescription( const QString & _desc )
	{
		m_description = _desc;
	}

	inline void setUnit( const QString & _unit )
	{
		m_unit = _unit;
	}


protected:
	void addDefaultActions( QMenu * _menu );

	QString m_description;
	QString m_unit;

} ;




class automatableModelViewSlots : public QObject
{
	Q_OBJECT
public:
	automatableModelViewSlots( 
			automatableModelView * _amv,
			QObject * _parent );

public slots:
	void execConnectionDialog( void );
	void removeConnection( void );

protected:
	automatableModelView * amv;

} ;




#define generateTypedModelView(type)					\
class EXPORT type##ModelView : public automatableModelView			\
{\
public:\
	type##ModelView( ::model * _model ) :\
		automatableModelView( _model )\
	{\
	}\
\
	type##Model * model( void )\
	{\
		return( castModel<type##Model>() );\
	}\
\
	const type##Model * model( void ) const\
	{\
		return( castModel<type##Model>() );\
	}\
} ;


generateTypedModelView(float);
generateTypedModelView(int);
generateTypedModelView(bool);


#endif

