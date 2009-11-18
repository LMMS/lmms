/*
 * PreferencesDialog.cpp - implementation of PreferencesDialog
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

#include "PreferencesDialog.h"
#include "embed.h"
#include "engine.h"
#include "MainWindow.h"
#include "ui_PreferencesDialog.h"


PreferencesDialog::PreferencesDialog() :
	QDialog( engine::mainWindow() ),
	ui( new Ui::PreferencesDialog )
{
	ui->setupUi( this );

	// set up icons in page selector view on the left side
	static const char * icons[] = {
		"preferences-system",
		"folder-64",
		"preferences-desktop-sound",
		"setup-midi",
		"setup-plugins"
	} ;
	for( int i = 0; i < qMin<int>( sizeof( icons ),
								ui->configPageSelector->count() ); ++i )
	{
		ui->configPageSelector->item( i )->setIcon(
									embed::getIconPixmap( icons[i] ) );
	}
}



