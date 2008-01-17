/*
 * ladspa_control.cpp - model for controlling a LADSPA port
 *
 * Copyright (c) 2006-2008 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include "ladspa_control.h"
#include "automatable_model_templates.h"
#include "ladspa_base.h"


ladspaControl::ladspaControl( model * _parent, port_desc_t * _port, 
					track * _track, bool _link ) :
	model( _parent ),
	m_port( _port ),
        m_linkEnabledModel( FALSE, FALSE, TRUE, boolModel::defaultRelStep(),
								this ),
        m_toggledModel( FALSE, FALSE, TRUE, boolModel::defaultRelStep(),
								this ),
        m_knobModel( 0, 0, 0, 1, this )
{
	if( _link )
	{
		m_linkEnabledModel.setValue( FALSE );
		m_linkEnabledModel.setTrack( _track );
		connect( &m_linkEnabledModel, SIGNAL( dataChanged() ),
					 this, SLOT( linkStateChanged() ) );

	}
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.setTrack( _track );
			connect( &m_toggledModel, SIGNAL( dataChanged() ),
					 this, SLOT( ledChanged() ) );
			if( m_port->def == 1.0f )
			{
				m_toggledModel.setValue( TRUE );
			}
			break;

		case INTEGER:
			m_knobModel.setTrack( _track );
			m_knobModel.setRange( static_cast<int>( m_port->max ), 
					  static_cast<int>( m_port->min ), 
					  1 + static_cast<int>( m_port->max - 
							  m_port->min ) / 400 );
			m_knobModel.setInitValue( 
					static_cast<int>( m_port->def ) );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			break;

		case FLOAT:
			m_knobModel.setTrack( _track );
			m_knobModel.setRange( m_port->min, m_port->max,
				( m_port->max - m_port->min )
				/ ( m_port->name.toUpper() == "GAIN"
					&& m_port->max == 10.0f ? 4000.0f :
								400.0f ) );
			m_knobModel.setInitValue( m_port->def );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			break;

		case TIME:
			m_knobModel.setTrack( _track );
			m_knobModel.setRange( m_port->min, m_port->max, 
					  ( m_port->max - 
						m_port->min ) / 400.0f );
			m_knobModel.setInitValue( m_port->def );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			break;

		default:
			break;
	}
}




ladspaControl::~ladspaControl()
{
}




LADSPA_Data ladspaControl::getValue( void )
{
	LADSPA_Data value = 0.0f;
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			value = static_cast<LADSPA_Data>( 
						m_toggledModel.value() );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			value = static_cast<LADSPA_Data>(
							m_knobModel.value() );
			break;		
		default:
			printf( "ladspaControl::getValue BAD BAD BAD\n" );
			break;
	}
	
	return( value );
}




void ladspaControl::setValue( LADSPA_Data _value )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.setValue( static_cast<bool>( _value ) );
			break;
		case INTEGER:
			m_knobModel.setValue( static_cast<int>( _value ) );
			break;
		case FLOAT:
		case TIME:
			m_knobModel.setValue( static_cast<float>( _value ) );
			break;
		default:
			printf("ladspaControl::setValue BAD BAD BAD\n");
			break;
	}
}




void FASTCALL ladspaControl::saveSettings( QDomDocument & _doc, 
					   QDomElement & _this, 
					   const QString & _name )
{
	if( m_link )
	{
		m_linkEnabledModel.saveSettings( _doc, _this, _name + "link" );
	}
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.saveSettings( _doc, _this, _name );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			m_knobModel.saveSettings( _doc, _this, _name );
			break;
		default:
			printf("ladspaControl::saveSettings BAD BAD BAD\n");
			break;
	}
}



void FASTCALL ladspaControl::loadSettings( const QDomElement & _this, 
					   const QString & _name )
{
	if( m_link )
	{
		m_linkEnabledModel.loadSettings( _this, _name + "link" );
	}
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.loadSettings( _this, _name );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			m_knobModel.loadSettings( _this, _name );
			break;
		default:
			printf("ladspaControl::loadSettings BAD BAD BAD\n");
			break;
	}
}




void FASTCALL ladspaControl::linkControls( ladspaControl * _control )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			boolModel::linkModels( &m_toggledModel,
						_control->getToggledModel() );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			knobModel::linkModels( &m_knobModel,
						_control->getKnobModel() );
			break;
		default:
			break;
	}
}




void ladspaControl::ledChanged( void )
{
	emit( changed( m_port->port_id, static_cast<LADSPA_Data>(
						m_toggledModel.value() ) ) );
}




void ladspaControl::knobChanged( void )
{
	emit( changed( m_port->port_id, static_cast<LADSPA_Data>(
						m_knobModel.value() ) ) );
}




void FASTCALL ladspaControl::unlinkControls( ladspaControl * _control )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			boolModel::unlinkModels( &m_toggledModel,
						_control->getToggledModel() );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			knobModel::unlinkModels( &m_knobModel,
						_control->getKnobModel() );
			break;
		default:
			break;
	}
}




void ladspaControl::linkStateChanged( void )
{
	emit( linkChanged( m_port->control_id, m_linkEnabledModel.value() ) );
}




void FASTCALL ladspaControl::setLink( bool _state )
{
	m_linkEnabledModel.setValue( _state );
}



#include "ladspa_control.moc"
