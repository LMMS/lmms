/*
 * ManageResourceLocationsDialog.cpp - implementation of ManageResourceLocationsDialog
 *
 * Copyright (c) 2010 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "ManageResourceLocationsDialog.h"
#include "EditResourceLocationDialog.h"
#include "MainWindow.h"
#include "engine.h"

#include "ui_ManageResourceLocationsDialog.h"



ManageResourceLocationsDialog::ManageResourceLocationsDialog() :
	QDialog( engine::mainWindow() ),
	ui( new Ui::ManageResourceLocationsDialog )
{
	ui->setupUi( this );

	connect( ui->addLocationButton, SIGNAL( clicked() ),
				this, SLOT( addLocation() ) );
	connect( ui->editLocationButton, SIGNAL( clicked() ),
				this, SLOT( editLocation() ) );
	connect( ui->removeLocationButton, SIGNAL( clicked() ),
				this, SLOT( removeLocation() ) );
}




ManageResourceLocationsDialog::~ManageResourceLocationsDialog()
{
}




void ManageResourceLocationsDialog::addLocation()
{
	EditResourceLocationDialog editDialog;
	if( editDialog.exec() )
	{
	}
}



void ManageResourceLocationsDialog::editLocation()
{
}




void ManageResourceLocationsDialog::removeLocation()
{
}



#include "moc_ManageResourceLocationsDialog.cxx"

