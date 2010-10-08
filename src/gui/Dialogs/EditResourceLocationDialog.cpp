/*
 * EditResourceLocationDialog.cpp - implementation of EditResourceLocationDialog
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

#include <QtGui/QPushButton>

#include "EditResourceLocationDialog.h"
#include "MainWindow.h"
#include "engine.h"

#include "ui_EditResourceLocationDialog.h"



EditResourceLocationDialog::EditResourceLocationDialog(
										const ResourceLocation & location ) :
	QDialog( engine::mainWindow() ),
	ui( new Ui::EditResourceLocationDialog ),
	m_location( location )
{
	ui->setupUi( this );

	ui->nameEdit->setText( m_location.name() );
	ui->typeComboBox->setCurrentIndex(
			m_location.type() != ResourceLocation::Unknown ?
				m_location.type() : ResourceLocation::LocalDirectory );
	ui->addressEdit->setText( m_location.address() );

	connect( ui->nameEdit, SIGNAL( textChanged( const QString & ) ),
				this, SLOT( updateAndCheckInput() ) );
	connect( ui->typeComboBox, SIGNAL( currentIndexChanged( int ) ),
				this, SLOT( updateAndCheckInput() ) );
	connect( ui->addressEdit, SIGNAL( textChanged( const QString & ) ),
				this, SLOT( updateAndCheckInput() ) );

	updateAndCheckInput();
}




EditResourceLocationDialog::~EditResourceLocationDialog()
{
}



void EditResourceLocationDialog::updateAndCheckInput()
{
	m_location.setName( ui->nameEdit->text() );
	m_location.setType( static_cast<ResourceLocation::Type>(
										ui->typeComboBox->currentIndex() ) );
	m_location.setAddress( ui->addressEdit->text() );

	ui->buttonBox->
			button( QDialogButtonBox::Ok )->setEnabled( m_location.isValid() );
}



#include "moc_EditResourceLocationDialog.cxx"

