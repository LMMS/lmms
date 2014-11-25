/*
 * MeterDialog.cpp - dialog for entering meter settings
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/yahoo.com>
 *
 * This file is part of LMMS - http://lmms.io
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


#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

#include "MeterDialog.h"
#include "MeterModel.h"
#include "embed.h"
#include "gui_templates.h"
#include "LcdSpinBox.h"


MeterDialog::MeterDialog( QWidget * _parent, bool _simple ) :
	QWidget( _parent ),
	ModelView( NULL, this )
{
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );

	QWidget * num = new QWidget( this );
	QHBoxLayout * num_layout = new QHBoxLayout( num );
	num_layout->setSpacing( 0 );
	num_layout->setMargin( 0 );


	m_numerator = new LcdSpinBox( 2, num, tr( "Meter Numerator" ) );

	num_layout->addWidget( m_numerator );

	if( !_simple )
	{
		QLabel * num_label = new QLabel( tr( "Meter Numerator" ), num );
		QFont f = num_label->font();
		num_label->setFont( pointSize<7>( f ) );
		num_layout->addSpacing( 5 );
		num_layout->addWidget( num_label );
	}
	num_layout->addStretch();


	QWidget * den = new QWidget( this );
	QHBoxLayout * den_layout = new QHBoxLayout( den );
	den_layout->setSpacing( 0 );
	den_layout->setMargin( 0 );

	m_denominator = new LcdSpinBox( 2, den, tr( "Meter Denominator" ) );
	if( _simple )
	{
		m_denominator->setLabel( tr( "TIME SIG" ) );
	}

	den_layout->addWidget( m_denominator );

	if( !_simple )
	{
		QLabel * den_label = new QLabel( tr( "Meter Denominator" ),
									den );
		QFont f = den_label->font();
		den_label->setFont( pointSize<7>( f ) );
		den_layout->addSpacing( 5 );
		den_layout->addWidget( den_label );
	}
	den_layout->addStretch();


	vlayout->addSpacing( _simple ? 1 : 3 );
	vlayout->addWidget( num );
	vlayout->addSpacing( 2 );
	vlayout->addWidget( den );
	vlayout->addStretch();
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

#include "moc_MeterDialog.cxx"
