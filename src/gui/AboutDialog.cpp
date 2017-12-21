/*
 * AboutDialog.cpp - implementation of about-dialog
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
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


#include "lmmsversion.h"
#include "AboutDialog.h"
#include "embed.h"
#include "versioninfo.h"



AboutDialog::AboutDialog(QWidget* parent) :
	QDialog(parent),
	Ui::AboutDialog()
{
	setupUi( this );


	iconLabel->setPixmap( embed::getIconPixmap( "icon", 64, 64 ) );

	versionLabel->setText( versionLabel->text().
					arg( LMMS_VERSION ).
					arg( PLATFORM ).
					arg( MACHINE ).
					arg( QT_VERSION_STR ).
					arg( COMPILER_VERSION ) );
	versionLabel->setTextInteractionFlags(
					versionLabel->textInteractionFlags() |
					Qt::TextSelectableByMouse );

	copyrightLabel->setText( copyrightLabel->text().
					arg( LMMS_PROJECT_COPYRIGHT ) );

	authorLabel->setPlainText( embed::getText( "AUTHORS" ) );

	licenseLabel->setPlainText( embed::getText( "LICENSE.txt" ) );

	involvedLabel->setPlainText( embed::getText( "CONTRIBUTORS" ) );
}
