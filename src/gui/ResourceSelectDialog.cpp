/*
 * ResourceSelectDialog.cpp - implementation of ResourceSelectDialog
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

#include <QtGui/QAbstractItemView>
#include <QtGui/QLineEdit>

#include "ResourceSelectDialog.h"
#include "ResourceListModel.h"
#include "ResourceTreeModel.h"
#include "engine.h"



ResourceSelectDialog::ResourceSelectDialog( QWidget * _parent,
												ModelTypes _modelType,
												DatabaseScope _databaseScope ) :
	QDialog( _parent ),
	m_model( NULL )
{
	ResourceDB * db = NULL;
	switch( _databaseScope )
	{
		case WorkingDirResources:
			db = engine::workingDirResourceDB();
			break;
		case WebResources:
			db = engine::webResourceDB();
			break;
		case AllResources:
		default:
			db = engine::mergedResourceDB();
			break;
	}

	switch( _modelType )
	{
		case ListModel:
			m_model = new ResourceListModel( db, this );
			break;
		case TreeModel:
			m_model = new ResourceTreeModel( db, this );
			break;
	}
}




ResourceSelectDialog::~ResourceSelectDialog()
{
	delete m_model;
}




ResourceItem * ResourceSelectDialog::selectedItem()
{
	if( result() != QDialog::Accepted )
	{
		return NULL;
	}

	QAbstractItemView * resourceView = findChild<QAbstractItemView *>();
	if( !resourceView )
	{
		return NULL;
	}

	return m_model->item( resourceView->currentIndex() );
}




void ResourceSelectDialog::setupUi( )
{
	QAbstractItemView * resourceView = findChild<QAbstractItemView *>();
	if( resourceView )
	{
		// setup view to display our model
		resourceView->setModel( m_model );
		resourceView->selectionModel()->select( m_model->index( 0, 0 ),
										QItemSelectionModel::SelectCurrent );
	}

	QLineEdit * filterEdit = findChild<QLineEdit *>();
	if( filterEdit )
	{
		// connect filter edit with model
		connect( filterEdit, SIGNAL( textChanged( const QString & ) ),
					m_model, SLOT( setKeywordFilter( const QString & ) ) );
	}
}




void ResourceSelectDialog::setTypeFilter( int _type )
{
	m_model->setTypeFilter( static_cast<ResourceItem::Type>( _type ) );
}



#include "moc_ResourceSelectDialog.cxx"

