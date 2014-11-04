/*
 * ladspa_port_dialog.cpp - dialog to test a LADSPA plugin
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "ladspa_port_dialog.h"

#include <QtGui/QLayout>
#include <QtGui/QTableWidget>

#include "embed.h"
#include "engine.h"
#include "ladspa_2_lmms.h"
#include "Mixer.h"


ladspaPortDialog::ladspaPortDialog( const ladspa_key_t & _key )
{
	ladspa2LMMS * manager = engine::getLADSPAManager();

	setWindowIcon( embed::getIconPixmap( "ports" ) );
	setWindowTitle( tr( "Ports" ) );
	setModal( true );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );

	int pc = manager->getPortCount( _key );

	QTableWidget * settings = new QTableWidget( pc, 7, this );

	QStringList ports;
	ports.append( tr( "Name" ) );
	ports.append( tr( "Rate" ) );
	ports.append( tr( "Direction" ) );
	ports.append( tr( "Type" ) );
	ports.append( tr( "Min < Default < Max" ) );
	ports.append( tr( "Logarithmic" ) );
	ports.append( tr( "SR Dependent" ) );
	settings->setHorizontalHeaderLabels( ports );

	for( int row = 0; row < pc; row++ )
	{
		for( int col = 0; col < 7; ++col )
		{
			QTableWidgetItem * item = new QTableWidgetItem;
			item->setFlags( 0 );
			settings->setItem( row, col, item );
		}

		int col = 0;
		settings->item( row, col++ )->setText( manager->getPortName( _key, row ) );

		settings->item( row, col++ )->setText( manager->isPortAudio( _key, row ) ?  tr( "Audio" ) : tr( "Control" ) );

		settings->item( row, col++ )->setText( manager->isPortInput( _key, row ) ?  tr( "Input" ) : tr( "Output" ) );

		settings->item( row, col++ )->setText( manager->isPortToggled( _key, row ) ? tr( "Toggled" ) : manager->isInteger( _key, row ) ? tr( "Integer" ) : tr( "Float" ) );

		float min = manager->getLowerBound( _key, row );
		float max = manager->getUpperBound( _key, row );
		float def = manager->getDefaultSetting( _key, row );
		QString range = "";

		if( manager->areHintsSampleRateDependent( _key, row ) )
		{
			if( min != NOHINT )
			{
				min *= engine::mixer()->processingSampleRate();
			}
			if( max != NOHINT )
			{
				max *= engine::mixer()->processingSampleRate();
			}
		}

		if( min == NOHINT )
		{
			range += "-Inf < ";
		}
		else if( manager->isInteger( _key, row ) )
		{
			range += QString::number( static_cast<int>( min ) ) +
									" < ";
		}
		else
		{
			range += QString::number( min ) + " < ";
		}

		if( def == NOHINT )
		{
			range += "None < ";
		}
		else if( manager->isInteger( _key, row ) )
		{
			range += QString::number( static_cast<int>( def ) ) + 
									" < ";
		}
		else
		{
			range += QString::number( def ) + " < ";
		}

		if( max == NOHINT )
		{
			range += "Inf";
		}
		else if( manager->isInteger( _key, row ) )
		{
			range += QString::number( static_cast<int>( max ) );
		}
		else
		{
			range += QString::number( max );
		}

		if( manager->isPortOutput( _key, row ) ||
					manager->isPortToggled( _key, row ) )
		{
			range = "";
		}

		settings->item( row, col++ )->setText( range );

		if( manager->isLogarithmic( _key, row ) )
		{
			settings->item( row, col )->setText( tr( "Yes" ) );
		}
		col++;

		if( manager->areHintsSampleRateDependent( _key, row ) )
		{
			settings->item( row, col )->setText( tr( "Yes" ) );
		}
	}


	vlayout->addWidget( settings );

	show();
}




ladspaPortDialog::~ladspaPortDialog()
{
}




#include  "moc_ladspa_port_dialog.cxx"

