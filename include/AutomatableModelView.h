/*
 * AutomatableModelView.h - class AutomatableModelView
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - http://lmms.io
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
	AutomatableModelView( Model* model, QWidget* _this );
	virtual ~AutomatableModelView();

	// some basic functions for convenience
	AutomatableModel* modelUntyped()
	{
		return castModel<AutomatableModel>();
	}

	const AutomatableModel* modelUntyped() const
	{
		return castModel<AutomatableModel>();
	}

	virtual void setModel( Model* model, bool isOldModelValid = true );

	template<typename T>
	inline T value() const
	{
		return modelUntyped() ? modelUntyped()->value<T>() : 0;
	}

	inline void setDescription( const QString& desc )
	{
		m_description = desc;
	}

	inline void setUnit( const QString& unit )
	{
		m_unit = unit;
	}

	void addDefaultActions( QMenu* menu );


protected:
	virtual void mousePressEvent( QMouseEvent* event );

	QString m_description;
	QString m_unit;

} ;




class AutomatableModelViewSlots : public QObject
{
	Q_OBJECT
public:
	AutomatableModelViewSlots( AutomatableModelView* amv, QObject* parent );

public slots:
	void execConnectionDialog();
	void removeConnection();
	void editSongGlobalAutomation();
	void unlinkAllModels();
	void removeSongGlobalAutomation();


protected:
	AutomatableModelView* m_amv;

} ;




#define generateTypedModelView(type)							\
class EXPORT type##ModelView : public AutomatableModelView		\
{																\
public:															\
	type##ModelView( Model* model, QWidget* _this ) :			\
		AutomatableModelView( model, _this )					\
	{															\
	}															\
																\
	type##Model* model()										\
	{															\
		return castModel<type##Model>();						\
	}															\
																\
	const type##Model* model() const							\
	{															\
		return castModel<type##Model>();						\
	}															\
}


generateTypedModelView(Float);
generateTypedModelView(Int);
generateTypedModelView(Bool);


#endif

