/*
 * MeterDialog.cpp - dialog for entering meter settings
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo.com>
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


#include <QLayout>
#include <QPushButton>
#include <QLabel>

#include "MeterDialog.h"
#include "MeterModel.h"
#include "gui_templates.h"
#include "IntegerSpinBox.h"
#include "ToolTip.h"


MeterDialog::MeterDialog( QWidget * _parent ) :
	QWidget( _parent ),
	ModelView( NULL, this )
{
	QGridLayout * mainLayout = new QGridLayout( this );
	mainLayout->setSpacing( 0 );
	mainLayout->setMargin( 0 );

	m_numerator = new IntegerSpinBox( 2, this, tr( "Meter Numerator" ) );
	ToolTip::add( m_numerator, tr( "Meter numerator" ) );

	m_denominator = new IntegerSpinBox( 2, this, tr( "Meter Denominator" ) );
	ToolTip::add( m_denominator, tr( "Meter denominator" ) );
	
	// Add caption label:
	QLabel * mainLabel = new QLabel( tr("TIME SIG"), this );
	mainLabel->setObjectName( "integerDisplayTitle" );
	mainLabel->setAlignment( Qt::AlignHCenter );
	mainLayout->addWidget( mainLabel, 0, 0, 1, 3 );
	
	// Add integer displays:
	QLabel * forwardSlash = new QLabel( "/", this );
	forwardSlash->setObjectName( "integerDisplayDigits" );
	forwardSlash->setFixedWidth(6);

	// TODO: remove "_simple" parameter!
	mainLayout->addWidget( m_numerator, 1, 0 );
	mainLayout->addWidget( forwardSlash, 1, 1 );
	mainLayout->addWidget( m_denominator, 1, 2 );
	
	setMaximumHeight( 35 );
}




MeterDialog::~MeterDialog()
{
}




void MeterDialog::modelChanged()
{
	MeterModel * mm = castModel<MeterModel>();
	m_numerator->setModel( &mm->numeratorModel() );
	m_denominator->setModel( &mm->denominatorModel() );
}

