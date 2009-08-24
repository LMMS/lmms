/*
 * AutomatableModelView.h - class AutomatableModelView
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ModelView.h"
#include "AutomatableModel.h"

class QMenu;


class EXPORT AutomatableModelView : public ModelView
{
public:
	AutomatableModelView( Model * _model, QWidget * _this );
	virtual ~AutomatableModelView();

	// some basic functions for convenience
	AutomatableModel * modelUntyped()
	{
		return( castModel<AutomatableModel>() );
	}

	const AutomatableModel * modelUntyped() const
	{
		return( castModel<AutomatableModel>() );
	}

	virtual void setModel( Model * _model, bool _old_model_valid = true );

	template<typename T>
	inline T value() const
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

	void addDefaultActions( QMenu * _menu );


protected:
	virtual void mousePressEvent( QMouseEvent * _ev );

	QString m_description;
	QString m_unit;

} ;




class AutomatableModelViewSlots : public QObject
{
	Q_OBJECT
public:
	AutomatableModelViewSlots( 
			AutomatableModelView * _amv,
			QObject * _parent );

public slots:
	void execConnectionDialog();
	void removeConnection();
	void editSongGlobalAutomation();
	void arm();
	void deArm();

protected:
	AutomatableModelView * amv;

} ;




#define generateTypedModelView(type)							\
class EXPORT type##ModelView : public AutomatableModelView		\
{																\
public:															\
	type##ModelView( Model * _model, QWidget * _this ) :		\
		AutomatableModelView( _model, _this )					\
	{															\
	}															\
																\
	type##Model * model()										\
	{															\
		return( castModel<type##Model>() );						\
	}															\
																\
	const type##Model * model() const							\
	{															\
		return( castModel<type##Model>() );						\
	}															\
}


generateTypedModelView(Float);
generateTypedModelView(Int);
generateTypedModelView(Bool);


#endif

