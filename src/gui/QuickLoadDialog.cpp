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

#include "ui_QuickLoadDialog.h"


QuickLoadDialog::QuickLoadDialog( QWidget * _parent,
									ResourceItem::Type _typeFilter,
									DatabaseScope _databaseScope ) :
	ResourceSelectDialog( _parent, ResourceSelectDialog::ListModel,
							_databaseScope ),
	ui( new Ui::QuickLoadDialog )
{
	// setup form
	ui->setupUi( this );

	// setup ResourceSelectDialog
	setupUi();

	// setup type combobox + type filtering
	for( int i = ResourceItem::TypeUnknown+1; i < ResourceItem::NumTypes; ++i )
	{
		ui->resourceTypeComboBox->addItem(
			ResourceItem::descriptiveTypeName(
				static_cast<ResourceItem::Type>( i ) ) );
	}

	connect( ui->resourceTypeComboBox, SIGNAL( currentIndexChanged( int ) ),
				this, SLOT( setTypeFilter( int ) ) );

	if( _typeFilter != ResourceItem::TypeUnknown )
	{
		ui->resourceTypeComboBox->setCurrentIndex( _typeFilter );
	}
}




QuickLoadDialog::~QuickLoadDialog()
{
}


