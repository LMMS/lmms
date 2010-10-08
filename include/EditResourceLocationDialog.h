/*
 * EditResourceLocationDialog.h - header file for EditResourceLocationDialog
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

#ifndef _EDIT_RESOURCE_LOCATION_DIALOG_H
#define _EDIT_RESOURCE_LOCATION_DIALOG_H

#include <QtGui/QDialog>

#include "ResourceLocation.h"

namespace Ui { class EditResourceLocationDialog; }

/*! \brief The EditResourceLocationDialog class provides a dialog for editing
 * a certain ResourceLocation.
 */

class EditResourceLocationDialog : public QDialog
{
	Q_OBJECT
public:
	/*! \brief Constructs an EditResourceLocationDialog.
	* \param location A ResourceLocation object
	*/
	EditResourceLocationDialog(
					const ResourceLocation & location = ResourceLocation() );

	/*! \brief Destroys the EditResourceLocationDialog. */
	virtual ~EditResourceLocationDialog();

	/*! \brief Returns a ResourceLocation object based on user input. */
	const ResourceLocation & location() const
	{
		return m_location;
	}


private slots:
	void updateAndCheckInput();


private:
	Ui::EditResourceLocationDialog * ui;
	ResourceLocation m_location;

} ;


#endif
