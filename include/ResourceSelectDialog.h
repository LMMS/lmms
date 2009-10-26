/*
 * ResourceSelectDialog.h - header file for ResourceSelectDialog
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

#ifndef _RESOURCE_SELECT_DIALOG_H
#define _RESOURCE_SELECT_DIALOG_H

#include <QtGui/QDialog>

class ResourceItem;
class ResourceModel;

class ResourceSelectDialog : public QDialog
{
	Q_OBJECT
public:
	enum ModelTypes
	{
		ListModel,
		TreeModel
	} ;

	enum DatabaseScope
	{
		WorkingDirResources,
		WebResources,
		AllResources
	} ;

	ResourceSelectDialog( QWidget * _parent, ModelTypes _modelType,
							DatabaseScope _databaseScope = AllResources );
	virtual ~ResourceSelectDialog();

	// returns the selected item (NULL if the dialog was not accepted or no
	// valid item was selected)
	ResourceItem * selectedItem();


protected:
	void setupUi();


private slots:
	void setTypeFilter( int );


private:
	ResourceModel * m_model;

} ;

#endif

