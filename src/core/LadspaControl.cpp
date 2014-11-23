/*
 * LadspaControl.cpp - model for controlling a LADSPA port
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <cstdio>

#include "LadspaControl.h"
#include "LadspaBase.h"


LadspaControl::LadspaControl( Model * _parent, port_desc_t * _port,
								bool _link ) :
	Model( _parent ),
	m_link( _link ),
	m_port( _port ),
	m_linkEnabledModel( _link, this, tr( "Link channels" ) ),
	m_toggledModel( false, this, m_port->name ),
	m_knobModel( 0, 0, 0, 1, this, m_port->name ),
	m_tempoSyncKnobModel( 0, 0, 0, 1, m_port->max, this, m_port->name )
{
	if( m_link )
	{
		connect( &m_linkEnabledModel, SIGNAL( dataChanged() ),
					 this, SLOT( linkStateChanged() ) );
	}

	switch( m_port->data_type )
	{
		case TOGGLED:
			connect( &m_toggledModel, SIGNAL( dataChanged() ),
					 this, SLOT( ledChanged() ) );
			if( m_port->def == 1.0f )
			{
				m_toggledModel.setValue( true );
			}
			// TODO: careful: we must prevent saved scales
			m_toggledModel.setScaleLogarithmic( m_port->suggests_logscale );
			break;

		case INTEGER:
			m_knobModel.setRange( static_cast<int>( m_port->max ),
					  static_cast<int>( m_port->min ),
					  1 + static_cast<int>( m_port->max -
							  m_port->min ) / 400 );
			m_knobModel.setInitValue(
					static_cast<int>( m_port->def ) );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			// TODO: careful: we must prevent saved scales
			m_knobModel.setScaleLogarithmic( m_port->suggests_logscale );
			break;

		case FLOATING:
			m_knobModel.setRange( m_port->min, m_port->max,
				( m_port->max - m_port->min )
				/ ( m_port->name.toUpper() == "GAIN"
					&& m_port->max == 10.0f ? 4000.0f :
								( m_port->suggests_logscale ? 8000.0f : 800.0f ) ) );
			m_knobModel.setInitValue( m_port->def );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			// TODO: careful: we must prevent saved scales
			m_knobModel.setScaleLogarithmic( m_port->suggests_logscale );
			break;

		case TIME:
			m_tempoSyncKnobModel.setRange( m_port->min, m_port->max,
					  ( m_port->max -
						m_port->min ) / 800.0f );
			m_tempoSyncKnobModel.setInitValue( m_port->def );
			connect( &m_tempoSyncKnobModel, SIGNAL( dataChanged() ),
					 this, SLOT( tempoKnobChanged() ) );
			// TODO: careful: we must prevent saved scales
			m_tempoSyncKnobModel.setScaleLogarithmic( m_port->suggests_logscale );
			break;

		default:
			break;
	}
}




LadspaControl::~LadspaControl()
{
}




LADSPA_Data LadspaControl::value()
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			return static_cast<LADSPA_Data>( m_toggledModel.value() );
		case INTEGER:
		case FLOATING:
			return static_cast<LADSPA_Data>( m_knobModel.value() );
		case TIME:
			return static_cast<LADSPA_Data>( m_tempoSyncKnobModel.value() );
		default:
			qWarning( "LadspaControl::value(): BAD BAD BAD\n" );
			break;
	}

	return 0;
}




void LadspaControl::setValue( LADSPA_Data _value )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.setValue( static_cast<bool>( _value ) );
			break;
		case INTEGER:
			m_knobModel.setValue( static_cast<int>( _value ) );
			break;
		case FLOATING:
			m_knobModel.setValue( static_cast<float>( _value ) );
			break;
		case TIME:
			m_tempoSyncKnobModel.setValue( static_cast<float>(
								_value ) );
			break;
		default:
			printf("LadspaControl::setValue BAD BAD BAD\n");
			break;
	}
}




void LadspaControl::saveSettings( QDomDocument& doc,
					   QDomElement& parent,
					   const QString& name )
{
	QDomElement e = doc.createElement( name );

	if( m_link )
	{
		m_linkEnabledModel.saveSettings( doc, e, "link" );
	}
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.saveSettings( doc, e, "data" );
			break;
		case INTEGER:
		case FLOATING:
			m_knobModel.saveSettings( doc, e, "data" );
			break;
		case TIME:
			m_tempoSyncKnobModel.saveSettings( doc, e, "data" );
			break;
		default:
			printf("LadspaControl::saveSettings BAD BAD BAD\n");
			break;
	}

	parent.appendChild( e );
}




void LadspaControl::loadSettings( const QDomElement& parent, const QString& name )
{
	QString dataModelName = "data";
	QString linkModelName = "link";
	QDomElement e = parent.namedItem( name ).toElement();

	// COMPAT < 1.0.0: detect old data format where there's either no dedicated sub
	// element or there's a direct sub element with automation link information
	if( e.isNull() || e.hasAttribute( "id" ) )
	{
		dataModelName = name;
		linkModelName = name + "link";
		e = parent;
	}

	if( m_link )
	{
		m_linkEnabledModel.loadSettings( e, linkModelName );
	}

	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.loadSettings( e, dataModelName );
			break;
		case INTEGER:
		case FLOATING:
			m_knobModel.loadSettings( e, dataModelName );
			break;
		case TIME:
			m_tempoSyncKnobModel.loadSettings( e, dataModelName );
			break;
		default:
			printf("LadspaControl::loadSettings BAD BAD BAD\n");
			break;
	}
}




void LadspaControl::linkControls( LadspaControl * _control )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			BoolModel::linkModels( &m_toggledModel, _control->toggledModel() );
			break;
		case INTEGER:
		case FLOATING:
			FloatModel::linkModels( &m_knobModel, _control->knobModel() );
			break;
		case TIME:
			TempoSyncKnobModel::linkModels( &m_tempoSyncKnobModel,
					_control->tempoSyncKnobModel() );
			break;
		default:
			break;
	}
}




void LadspaControl::ledChanged()
{
	emit changed( m_port->port_id, static_cast<LADSPA_Data>(
						m_toggledModel.value() ) );
}




void LadspaControl::knobChanged()
{
	emit changed( m_port->port_id, static_cast<LADSPA_Data>(
						m_knobModel.value() ) );
}




void LadspaControl::tempoKnobChanged()
{
	emit changed( m_port->port_id, static_cast<LADSPA_Data>(
					m_tempoSyncKnobModel.value() ) );
}




void LadspaControl::unlinkControls( LadspaControl * _control )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			BoolModel::unlinkModels( &m_toggledModel, _control->toggledModel() );
			break;
		case INTEGER:
		case FLOATING:
			FloatModel::unlinkModels( &m_knobModel, _control->knobModel() );
			break;
		case TIME:
			TempoSyncKnobModel::unlinkModels( &m_tempoSyncKnobModel,
					_control->tempoSyncKnobModel() );
			break;
		default:
			break;
	}
}




void LadspaControl::linkStateChanged()
{
	emit linkChanged( m_port->control_id, m_linkEnabledModel.value() );
}




void LadspaControl::setLink( bool _state )
{
	m_linkEnabledModel.setValue( _state );
}



#include "moc_LadspaControl.cxx"
