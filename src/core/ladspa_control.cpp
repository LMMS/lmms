/*
 * ladspa_control.cpp - widget for controlling a LADSPA port
 *
 * Copyright (c) 2006-2007 Danny McRae <khjklujn/at/users.sourceforge.net>
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


#include <QtGui/QWhatsThis>

#include "ladspa_control.h"
#include "automatable_model_templates.h"
#include "ladspa_base.h"
#include "led_checkbox.h"
#include "tempo_sync_knob.h"
#include "tooltip.h"


ladspaControl::ladspaControl( QWidget * _parent, 
				port_desc_t * _port, 
				track * _track,
			    	bool _link) :
	QWidget( _parent ),
	m_port( _port ),
	m_track( _track ),
	m_link( NULL ),
	m_toggle( NULL ),
	m_knob( NULL ),
        m_linkEnabledModel( FALSE, FALSE, TRUE ),
        m_toggledModel( FALSE, FALSE, TRUE ),
        m_knobModel()
{
	m_layout = new QHBoxLayout( this );
	
	if( _link )
	{
		m_link = new ledCheckBox( "", this );
		toolTip::add( m_link, tr( "Link channels" ) );

		m_linkEnabledModel.setValue( FALSE );
		m_linkEnabledModel.setTrack( _track );
		m_link->setModel( &m_linkEnabledModel );
		connect( &m_linkEnabledModel, SIGNAL( dataChanged() ),
					 this, SLOT( linkStateChanged() ) );

		m_layout->addWidget( m_link );
	}
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggledModel.setTrack( m_track );
			m_toggle = new ledCheckBox( m_port->name, this,
							QString::null,
							ledCheckBox::GREEN );
			m_toggle->setModel( &m_toggledModel );
			connect( &m_toggledModel, SIGNAL( dataChanged() ),
					 this, SLOT( ledChanged() ) );
			setFixedSize( m_toggle->width(), m_toggle->height() );
			if( m_port->def == 1.0f )
			{
				m_toggledModel.setValue( TRUE );
			}
			if( _link )
			{
				m_layout->addWidget( m_toggle );
				setFixedSize( m_link->width() + 
						m_toggle->width(),
						m_toggle->height() );
			}
			break;
		case INTEGER:
			m_knobModel.setTrack( m_track );
			m_knobModel.setRange( static_cast<int>( m_port->max ), 
					  static_cast<int>( m_port->min ), 
					  1 + static_cast<int>( m_port->max - 
							  m_port->min ) / 400 );
			m_knobModel.setInitValue( 
					static_cast<int>( m_port->def ) );
			m_knob = new knob( knobBright_26, this, m_port->name );
			m_knob->setModel( &m_knobModel );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );

			m_knob->setLabel( m_port->name );
			setFixedSize( m_knob->width(), m_knob->height() );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
			m_knob->setWhatsThis(
					tr( "Sorry, no help available." ) );
			if( _link )
			{
				m_layout->addWidget( m_knob );
				setFixedSize( m_link->width() + 
						m_knob->width(),
						m_knob->height() );
			}
			break;
		case FLOAT:
			m_knobModel.setTrack( m_track );
			m_knobModel.setRange( m_port->min, m_port->max,
				( m_port->max - m_port->min )
				/ ( m_port->name.toUpper() == "GAIN"
					&& m_port->max == 10.0f ? 4000.0f :
								400.0f ) );
			m_knobModel.setInitValue( m_port->def );
			m_knob = new knob( knobBright_26, this, m_port->name );
			m_knob->setModel( &m_knobModel );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			m_knob->setLabel( m_port->name );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
			m_knob->setWhatsThis(
					tr( "Sorry, no help available." ) );
			setFixedSize( m_knob->width(), m_knob->height() );
			if( _link )
			{
				m_layout->addWidget( m_knob );
				setFixedSize( m_link->width() + 
						m_knob->width(),
						m_knob->height() );
			}
			break;
		case TIME:
			m_knobModel.setTrack( m_track );
			m_knobModel.setRange( m_port->min, m_port->max, 
					  ( m_port->max - 
						m_port->min ) / 400.0f );
			m_knobModel.setInitValue( m_port->def );
			m_knob = new tempoSyncKnob( knobBright_26, this,
								m_port->name );
			m_knob->setModel( &m_knobModel );
			connect( &m_knobModel, SIGNAL( dataChanged() ),
						 this, SLOT( knobChanged() ) );
			m_knob->setLabel( m_port->name );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
			m_knob->setWhatsThis(
					tr( "Sorry, no help available." ) );
			setFixedSize( m_knob->width(), m_knob->height() );
			if( _link )
			{
				m_layout->addWidget( m_knob );
				setFixedSize( m_link->width() + 
						m_knob->width(),
						m_knob->height() );
			}
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
	if( m_link != NULL )
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
	if( m_link != NULL )
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
