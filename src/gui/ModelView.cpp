/*
 * ModelView.cpp - implementation of ModelView.cpp
 *
 * Copyright (c) 2007-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtGui/QWidget>

#include "ModelView.h"



ModelView::ModelView( Model * _model, QWidget * _this ) :
	m_widget( _this ),
	m_model( _model )
{
}




ModelView::~ModelView()
{
	if( m_model != NULL && m_model->defaultConstructed() )
	{
		delete m_model;
	}
}




void ModelView::setModel( Model * _model, bool _old_model_valid )
{
	if( _old_model_valid && m_model != NULL )
	{
		if( m_model->defaultConstructed() )
		{
			delete m_model;
		}
		else
		{
			m_model->disconnect( widget() );
		}
	}
	m_model = _model;

	doConnections();

	widget()->update();

	modelChanged();
}




void ModelView::doConnections()
{
	if( m_model != NULL )
	{
		QObject::connect( m_model, SIGNAL( dataChanged() ),
					widget(), SLOT( update() ) );

		QObject::connect( m_model, SIGNAL( propertiesChanged() ),
					widget(), SLOT( update() ) );
	}
}


