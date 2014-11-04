/*
 * ModelView.h - declaration of ModelView base class
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#ifndef MODEL_VIEW_H
#define MODEL_VIEW_H

#include "Model.h"


class EXPORT ModelView
{
public:
	ModelView( Model* model, QWidget* widget );
	virtual ~ModelView();

	virtual void setModel( Model* model, bool isOldModelValid = true );

	Model* model()
	{
		return m_model;
	}

	const Model* model() const
	{
		return m_model;
	}

	template<class T>
	T* castModel()
	{
		return dynamic_cast<T*>( model() );
	}

	template<class T>
	const T* castModel() const
	{
		return dynamic_cast<const T*>( model() );
	}


protected:
	// sub-classes can re-implement this to track model-changes
	virtual void modelChanged()
	{
	}

	QWidget* widget()
	{
		return m_widget;
	}

	virtual void doConnections();


private:
	QWidget* m_widget;
	QPointer<Model> m_model;

} ;


#endif

