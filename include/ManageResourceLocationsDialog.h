/*
 * ManageResourceLocationsDialog.h - header file for ManageResourceLocationsDialog
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

#ifndef _MANAGE_RESOURCE_LOCATIONS_DIALOG_H
#define _MANAGE_RESOURCE_LOCATIONS_DIALOG_H

#include <QtGui/QDialog>

#include "ResourceLocation.h"

namespace Ui { class ManageResourceLocationsDialog; }

/*! \brief The ManageResourceLocationsDialog class provides a dialog for adding,
 * editing and removing resource locations.
 */

class ManageResourceLocationsDialog : public QDialog
{
	Q_OBJECT
public:
	/*! \brief Constructs an ManageResourceLocationsDialog.
	*/
	ManageResourceLocationsDialog();

	/*! \brief Destroys the ManageResourceLocationsDialog. */
	virtual ~ManageResourceLocationsDialog();


private slots:
	void addLocation();
	void editLocation();
	void removeLocation();


private:
	Ui::ManageResourceLocationsDialog * ui;
	QList<ResourceLocation> m_locations;

} ;


#endif
