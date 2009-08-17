/*
 * QuickLoadDialog.cpp - implementation of QuickLoadDialog
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

#include "QuickLoadDialog.h"
#include "ResourceListModel.h"
#include "UnifiedResourceProvider.h"
#include "engine.h"

#include "ui_QuickLoadDialog.h"



QuickLoadDialog::QuickLoadDialog( QWidget * _parent ) :
	QDialog( _parent ),
	ui( new Ui::QuickLoadDialog ),
	m_listModel( new ResourceListModel(
							engine::resourceProvider()->database(), this ) )
{
	ui->setupUi( this );

	// setup list view to display our model
	ui->resourceListView->setModel( m_listModel );
	ui->resourceListView->selectionModel()->select(
										m_listModel->index( 0, 0 ),
										QItemSelectionModel::SelectCurrent );

	// connect filter edit with model
	connect( ui->filterEdit, SIGNAL( textChanged( const QString & ) ),
				m_listModel, SLOT( setFilter( const QString & ) ) );
}




QuickLoadDialog::~QuickLoadDialog()
{
	delete m_listModel;
}


