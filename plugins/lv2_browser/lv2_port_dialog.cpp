/*
 * lv2_port_dialog.cpp - dialog to test an LV2 plugin
 *
 * Copyright (c) 2009 Martin Andrews <mdda/at/users.sourceforge.net>
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


#include "lv2_port_dialog.h"

#include <QtGui/QLayout>
#include <QtGui/QTableWidget>

#include "embed.h"
#include "engine.h"
#include "Mixer.h"


lv2PortDialog::lv2PortDialog( const lv2_key_t & _key )
{
	lv2Manager * manager = static_lv2_manager;

	setWindowIcon( embed::getIconPixmap( "ports" ) );
	setWindowTitle( tr( "Ports" ) );
	setModal( true );

	QVBoxLayout * vlayout = new QVBoxLayout( this );
	vlayout->setSpacing( 0 );
	vlayout->setMargin( 0 );

	Uint16 pc = manager->getPortCount( _key );

	QStringList ports;
	ports.append( tr( "Name" ) );
	ports.append( tr( "Rate" ) );
	ports.append( tr( "Direction" ) );
	ports.append( tr( "Type" ) );
	ports.append( tr( "Min < Default < Max" ) );
//	ports.append( tr( "Logarithmic" ) );
//	ports.append( tr( "SR Dependent" ) );

	QTableWidget * settings = new QTableWidget( pc, ports.size(), this );
	settings->setHorizontalHeaderLabels( ports );

	for( Uint16 row = 0; row < pc; row++ )
	{
		for( Uint8 col = 0; col < ports.size(); ++col )
		{
			QTableWidgetItem * item = new QTableWidgetItem;
			item->setFlags( 0 );
			settings->setItem( row, col, item );
		}

		Uint8 col = 0;
		settings->item( row, col++ )->setText( manager->getPortName(
								_key, row ) );

		settings->item( row, col++ )->setText(
			manager->isPortAudio( _key, row ) ?
					tr( "Audio" ) : tr( "Control" ) );

		settings->item( row, col++ )->setText(
			manager->isPortInput( _key, row ) ?
					tr( "Input" ) : tr( "Output" ) );

		QStringList values;
		if( manager->isInteger( _key, row ) )
		{
			values = manager->listEnumeration( _key, row );
			settings->item( row, col )->setText( values.join("|") );
		}
		else
		{
			settings->item( row, col )->setText( tr( "Float" ) );
		}
		col++;

/*
		settings->item( row, col++ )->setText(
			manager->isPortToggled( _key, row ) ? tr( "Toggled" ) :
			manager->isInteger( _key, row ) ? tr( "Integer" ) :
								tr( "Float" ) );
*/

		float min = manager->getLowerBound( _key, row );
		float max = manager->getUpperBound( _key, row );
		float def = manager->getDefaultSetting( _key, row );
		QString range = "";

/*
		if( manager->areHintsSampleRateDependent( _key, row ) )
		{
			if( min != NOHINT )
			{
				min *= engine::getMixer()->processingSampleRate();
			}
			if( max != NOHINT )
			{
				max *= engine::getMixer()->processingSampleRate();
			}
		}
*/

		if( min == NOHINT )
		{
			range += "-Inf < ";
		}
		else if( values.size() >0 )
		{
			// Don't have a minimum
//			range += QString::number( static_cast<int>( min ) ) + " < ";
		}
		else
		{
			range += QString::number( min ) + " < ";
		}

		if( def == NOHINT )
		{
			range += "None < ";
		}
//		else if( manager->isInteger( _key, row ) )
		else if( values.size() > 0 )
		{
   def=( def < 0 )?0:(( def > values.size() )?values.size()-1:def);
   range += values.at( static_cast<int>( def ) );
//			range += QString::number( static_cast<int>( def ) ) + " < ";
		}
		else
		{
			range += QString::number( def ) + " < ";
		}

		if( max == NOHINT )
		{
			range += "Inf";
		}
		else if( values.size() >0 )
		{
   // Don't have a maximum
//			range += QString::number( static_cast<int>( min ) ) + " < ";
		}
		else
		{
			range += QString::number( max );
		}

		settings->item( row, col++ )->setText( range );

/*
		if( manager->isLogarithmic( _key, row ) )
		{
			settings->item( row, col )->setText( tr( "Yes" ) );
		}
		col++;
*/

/*
		if( manager->areHintsSampleRateDependent( _key, row ) )
		{
			settings->item( row, col )->setText( tr( "Yes" ) );
		}
	col++;
*/
	}

	vlayout->addWidget( settings );

	show();
}




lv2PortDialog::~lv2PortDialog()
{
}




#include  "moc_lv2_port_dialog.cxx"

