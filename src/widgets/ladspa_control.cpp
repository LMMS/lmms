#ifndef SINGLE_SOURCE_COMPILE

/*
 * ladspa_control.cpp - widget for controlling a LADSPA port
 *
 * Copyright (c) 2006 Danny McRae <khjklujn/at/users.sourceforge.net>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#include <qwhatsthis.h>

#include "ladspa_control.h"
#include "effect.h"


ladspaControl::ladspaControl( QWidget * _parent, port_desc_t * _port, engine * _engine, track * _track ) :
	QWidget( _parent, "ladspaControl" ),
	journallingObject( _engine ),
	m_port( _port ),
	m_track( _track ),
	m_toggle( NULL ),
	m_knob( NULL )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle = new ledCheckBox( m_port->name, this, "", eng(), m_track );
			setFixedSize( m_toggle->width(), m_toggle->height() );
			if( m_port->def == 1.0f )
			{
				m_toggle->setChecked( TRUE );
			}	
			break;
		case INTEGER:
			m_knob = new knob( knobBright_26, this, m_port->name, eng(), m_track);
			m_knob->setLabel( m_port->name );
			m_knob->setRange( static_cast<int>( m_port->max ), static_cast<int>( m_port->min ), 1 + static_cast<int>( m_port->max - m_port->min ) / 200 );
			m_knob->setInitValue( static_cast<int>( m_port->def ) );
			setFixedSize( m_knob->width(), m_knob->height() );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
#ifdef QT4
			m_knob->setWhatsThis(
#else
			QWhatsThis::add( m_knob,
#endif
					tr( "Sorry, no help available." ) );
			break;
		case FLOAT:
			m_knob = new knob( knobBright_26, this, m_port->name, eng(), m_track);
			m_knob->setLabel( m_port->name );
			m_knob->setRange( m_port->min, m_port->max, ( m_port->max - m_port->min ) / 200.0f );
			m_knob->setInitValue( m_port->def );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
#ifdef QT4
			m_knob->setWhatsThis(
#else
			QWhatsThis::add( m_knob,
#endif
					tr( "Sorry, no help available." ) );
			setFixedSize( m_knob->width(), m_knob->height() );
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
			value = static_cast<LADSPA_Data>( m_toggle->isChecked() );
			break;
		case INTEGER:
			value = static_cast<LADSPA_Data>( m_knob->value() );
			break;
		case FLOAT:
			value = static_cast<LADSPA_Data>( m_knob->value() );
			break;
		default:
			printf( "ladspaControl::getValue BAD BAD BAD\n" );
			break;
	}
	
	if( m_port->is_scaled )
	{
		value /= eng()->getMixer()->sampleRate();
	}
	
	return( value );
}




void ladspaControl::setValue( LADSPA_Data _value )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->setChecked( static_cast<bool>( _value ) );
			break;
		case INTEGER:
			m_knob->setValue( static_cast<int>( _value ) );
			break;
		case FLOAT:
			m_knob->setValue( static_cast<float>( _value ) );
			break;
		default:
			printf("ladspaControl::setValue BAD BAD BAD\n");
			break;
	}
}




void FASTCALL ladspaControl::saveSettings( QDomDocument & _doc, QDomElement & _this, const QString & _name )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->saveSettings( _doc, _this, _name );
			break;
		case INTEGER:
		case FLOAT:
			m_knob->saveSettings( _doc, _this, _name );
			break;
		default:
			printf("ladspaControl::saveSettings BAD BAD BAD\n");
			break;
	}
}



void FASTCALL ladspaControl::loadSettings( const QDomElement & _this, const QString & _name )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->loadSettings( _this, _name );
			break;
		case INTEGER:
		case FLOAT:
			m_knob->loadSettings( _this, _name );
			break;
		default:
			printf("ladspaControl::loadSettings BAD BAD BAD\n");
			break;
	}
}


#include "ladspa_control.moc"

#endif

#endif
