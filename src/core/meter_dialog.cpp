#ifndef SINGLE_SOURCE_COMPILE

/*
 * meter_dialog.cpp - diloag for entering meter settings
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/yahoo.com>
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

#ifdef QT4

#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

#else

#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>

#endif

#include "meter_dialog.h"
#include "embed.h"
#include "gui_templates.h"


meterDialog::meterDialog( QWidget * _parent, track * _track ):
	QWidget( _parent, "meterDialog" )
{
	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 5 );
	vlayout->setMargin( 5 );

	QWidget * num = new QWidget( this );
	QHBoxLayout * num_layout = new QHBoxLayout( num );
	num_layout->setSpacing( 10 );
	m_numerator = new lcdSpinBox( 1, 32, 2, num, 
						"",
						_track->eng(), _track );
	connect( m_numerator, SIGNAL( valueChanged( int ) ), 
			this, SIGNAL( numeratorChanged( int ) ) );
	m_numerator->setValue( 4 );
	num_layout->addWidget( m_numerator );
	QLabel * num_label = new QLabel( num );
	num_label->setText( tr( "Meter Numerator" ) );
	QFont f = num_label->font();
	num_label->setFont( pointSize<7>( f ) );
	num_layout->addWidget( num_label );
	
	QWidget * dem = new QWidget( this );
	QHBoxLayout * dem_layout = new QHBoxLayout( dem );
	dem_layout->setSpacing( 10 );
	m_denominator = new lcdSpinBox( 1, 32, 2, dem, 
						"",
						_track->eng(), _track );
	connect( m_denominator, SIGNAL( valueChanged( int ) ), 
			this, SIGNAL( denominatorChanged( int ) ) );
	m_denominator->setValue( 4 );
	dem_layout->addWidget( m_denominator );
	QLabel * dem_label = new QLabel( dem );
	f = dem_label->font();
	dem_label->setFont( pointSize<7>( f ) );
	dem_label->setText( tr( "Meter Denominator" ) );
	dem_layout->addWidget( dem_label );
	
	vlayout->addWidget( num );
	vlayout->addWidget( dem );
	
	setFixedSize( dem_label->width() + m_denominator->width() + 10,
			m_numerator->height() + m_denominator->height() + 15 );
}




meterDialog::~meterDialog()
{
}




void meterDialog::saveSettings( QDomDocument & _doc, QDomElement & _this,
							const QString & _name )
{
	m_numerator->saveSettings( _doc, _this, _name + "_numerator" );
	m_denominator->saveSettings( _doc, _this, _name + "_denominator" );
}




void meterDialog::loadSettings( const QDomElement & _this,
							const QString & _name )
{
	m_numerator->loadSettings( _this, _name + "_numerator" );
	m_denominator->loadSettings( _this, _name + "_denominator" );
}



#include "meter_dialog.moc"

#endif
