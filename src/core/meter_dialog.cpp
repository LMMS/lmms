#ifndef SINGLE_SOURCE_COMPILE

/*
 * meter_dialog.cpp - dialog for entering meter settings
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/yahoo.com>
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

#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

#include "meter_dialog.h"
#include "embed.h"
#include "gui_templates.h"


meterDialog::meterDialog( QWidget * _parent, track * _track ):
	QWidget( _parent ),
	m_numeratorModel( new lcdSpinBoxModel( /* this */ ) ),
	m_denominatorModel( new lcdSpinBoxModel( /* this */ ) )
{
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 5 );
	vlayout->setMargin( 5 );


	QWidget * num = new QWidget( this );
	QHBoxLayout * num_layout = new QHBoxLayout( num );
	num_layout->setSpacing( 10 );


	m_numeratorModel->setTrack( _track );
	m_numeratorModel->setRange( 1, 32 );
	m_numeratorModel->setValue( 4 );
	connect( m_numeratorModel, SIGNAL( dataChanged() ), 
				this, SIGNAL( numeratorChanged() ) );

	m_numerator = new lcdSpinBox( 2, num, tr( "Meter Numerator" ) );
	m_numerator->setModel( m_numeratorModel );

	num_layout->addWidget( m_numerator );

	QLabel * num_label = new QLabel( num );
	num_label->setText( tr( "Meter Numerator" ) );
	QFont f = num_label->font();
	num_label->setFont( pointSize<7>( f ) );
	num_layout->addWidget( num_label );

	
	QWidget * den = new QWidget( this );
	QHBoxLayout * den_layout = new QHBoxLayout( den );
	den_layout->setSpacing( 10 );

	m_denominatorModel->setTrack( _track );
	m_denominatorModel->setRange( 1, 32 );
	m_denominatorModel->setValue( 4 );
	connect( m_denominatorModel, SIGNAL( dataChanged() ), 
				this, SIGNAL( denominatorChanged() ) );

	m_denominator = new lcdSpinBox( 2, den, tr( "Meter Denominator" ) );
	m_denominator->setModel( m_denominatorModel );

	den_layout->addWidget( m_denominator );

	QLabel * den_label = new QLabel( den );
	f = den_label->font();
	den_label->setFont( pointSize<7>( f ) );
	den_label->setText( tr( "Meter Denominator" ) );
	den_layout->addWidget( den_label );
	
	vlayout->addWidget( num );
	vlayout->addWidget( den );
	
	setFixedSize( den_label->width() + m_denominator->width() + 10,
			m_numerator->height() + m_denominator->height() + 15 );
}




meterDialog::~meterDialog()
{
}




void meterDialog::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
	m_numeratorModel->saveSettings( _doc, _this, _name + "_numerator" );
	m_denominatorModel->saveSettings( _doc, _this, _name + "_denominator" );
}




void meterDialog::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	m_numeratorModel->loadSettings( _this, _name + "_numerator" );
	m_denominatorModel->loadSettings( _this, _name + "_denominator" );
}



#include "meter_dialog.moc"

#endif
