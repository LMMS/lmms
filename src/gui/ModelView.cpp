/*
 * ModelView.cpp - implementation of ModelView
 *
 * Copyright (c) 2007-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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

#include <QWidget>

#include "ModelView.h"

namespace lmms::gui
{


ModelView::ModelView( Model* model, QWidget* widget ) :
	m_widget( widget ),
	m_model( model )
{
}




ModelView::~ModelView()
{
	if( m_model != nullptr && m_model->isDefaultConstructed() )
	{
		delete m_model;
	}
}




void ModelView::setModel( Model* model, bool isOldModelValid )
{
	if( isOldModelValid && m_model != nullptr )
	{
		if( m_model->isDefaultConstructed() )
		{
			delete m_model;
		}
		else
		{
			m_model->disconnect( widget() );
		}
	}

	m_model = model;

	doConnections();

	widget()->update();

	modelChanged();
}




// Unsets the current model by setting a dummy empty model. The dummy model is marked as
// "defaultConstructed", so the next call to setModel will delete it.
void ModelView::unsetModel()
{
	setModel(new Model(nullptr, QString(), true));
}




void ModelView::doConnections()
{
	if( m_model != nullptr )
	{
		QObject::connect( m_model, SIGNAL(dataChanged()), widget(), SLOT(update()));
		QObject::connect( m_model, SIGNAL(propertiesChanged()), widget(), SLOT(update()));
	}
}


} // namespace lmms::gui
