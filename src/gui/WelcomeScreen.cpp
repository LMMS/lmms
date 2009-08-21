/*
 * WelcomeScreen.cpp - implementation of WelcomeScreen
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

#include <QtGui/QDesktopServices>

#include "WelcomeScreen.h"
#include "RecentResourceListModel.h"
#include "engine.h"
#include "embed.h"

#include "ui_WelcomeScreen.h"



WelcomeScreen::WelcomeScreen( QWidget * _parent ) :
	QWidget( _parent ),
	ui( new Ui::WelcomeScreen )
{
	ui->setupUi( this );

	// polish UI
	foreach( QPushButton * btn,
						ui->WelcomeFrame->findChildren<QPushButton *>() )
	{
		btn->setText( " " + btn->text() + " " );
	}

	ui->iconLabel->setPixmap( embed::getIconPixmap( "icon" ) );
	ui->newProjectButton->setIcon(
					embed::getIconPixmap( "project_new_from_template" ) );
	ui->importProjectButton->setIcon(
					embed::getIconPixmap( "project_import" ) );
	ui->openTutorialButton->setIcon(
					embed::getIconPixmap( "help" ) );
	ui->instantMidiActionButton->setIcon(
					embed::getIconPixmap( "instrument_track" ) );

	// connect signals of buttons
	connect( ui->newProjectButton, SIGNAL( clicked() ),
				this, SLOT( createNewProject() ) );
	connect( ui->importProjectButton, SIGNAL( clicked() ),
				this, SLOT( importProject() ) );
	connect( ui->openTutorialButton, SIGNAL( clicked() ),
				this, SLOT( openTutorial() ) );
	connect( ui->instantMidiActionButton, SIGNAL( clicked() ),
				this, SLOT( instantMidiAction() ) );

	// setup recent projects list view
	RecentResourceListModel * recentProjectsModel =
		new RecentResourceListModel( engine::workingDirResourceDB(), -1, this );
	recentProjectsModel->resourceListModel()->
							setTypeFilter( ResourceItem::TypeProject );
	ui->recentProjectsListView->setModel( recentProjectsModel );

	// setup recent community resources list view
	RecentResourceListModel * recentCommunityResourcesModel =
		new RecentResourceListModel( engine::webResourceDB(), 100, this );
	ui->communityResourcesListView->setModel( recentCommunityResourcesModel );

	// setup online resources list widget
	for( int i = 0; i < ui->onlineResourcesListWidget->count(); ++i )
	{
		ui->onlineResourcesListWidget->item( i )->setIcon(
										embed::getIconPixmap( "ListArrow" ) );
	}

	connect( ui->onlineResourcesListWidget,
					SIGNAL( itemClicked( QListWidgetItem * ) ),
				this, SLOT( openOnlineResource( QListWidgetItem * ) ) );
}




WelcomeScreen::~WelcomeScreen()
{
}




void WelcomeScreen::createNewProject()
{
}




void WelcomeScreen::importProject()
{
}




void WelcomeScreen::openTutorial()
{
}




void WelcomeScreen::instantMidiAction()
{
}




void WelcomeScreen::openOnlineResource( QListWidgetItem * _item )
{
	// the URL to be opened is encoded in status tip (no other
	// possibility to store such information in Qt Designer)
	QDesktopServices::openUrl( _item->statusTip() );
}


#include "moc_WelcomeScreen.cxx"

