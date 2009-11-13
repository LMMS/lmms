/*
 * WelcomeScreen.h - header file for WelcomeScreen
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

#ifndef _WELCOME_SCREEN_H
#define _WELCOME_SCREEN_H

#include <QtCore/QModelIndex>
#include <QtGui/QWidget>


namespace Ui { class WelcomeScreen; }
class QListWidgetItem;
class RecentResourceListModel;


class WelcomeScreen : public QWidget
{
	Q_OBJECT
public:
	WelcomeScreen( QWidget * _parent );
	~WelcomeScreen();


private slots:
	void createNewProject();
	void importProject();
	void openTutorial();
	void instantMidiAction();
	void openRecentProject( const QModelIndex & );
	void openCommunityResource( const QModelIndex & );
	void openOnlineResource( QListWidgetItem * _item );


private:
	void hideWelcomeScreen();

	Ui::WelcomeScreen * ui;
	RecentResourceListModel * m_recentProjectsModel;
	RecentResourceListModel * m_communityResourcesModel;

} ;

#endif

