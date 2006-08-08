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


ladspaControl::ladspaControl( QWidget * _parent, port_desc_t * _port, engine * _engine, instrumentTrack * _track ) :
	QWidget( _parent, "ladspaControl" ),
	engineObject( _engine ),
	m_port( _port ),
	m_toggle( NULL ),
	m_knob( NULL )
{
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle = new ledCheckBox( m_port->name, this, "", eng(), _track );
			setFixedSize( m_toggle->width(), m_toggle->height() );
			if( m_port->def == 1.0f )
			{
				m_toggle->setChecked( TRUE );
			}	
			break;
		case INTEGER:
			m_knob = new knob( knobBright_26, this, m_port->name, eng(), _track);
			m_knob->setLabel( m_port->name );
			m_knob->setRange( m_port->max, m_port->min, 1 + static_cast<int>( m_port->max - m_port->min ) / 500 );
			m_knob->setInitValue( m_port->def );
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
			m_knob = new knob( knobBright_26, this, m_port->name, eng(), _track);
			m_knob->setLabel( m_port->name );
			if( ( m_port->max - m_port->min ) < 500.0f )
			{
				m_knob->setRange( m_port->min, m_port->max, 0.01 );
			}
			else
			{
				m_knob->setRange( m_port->min, m_port->max, ( m_port->max - m_port->min ) / 500.0f );
			}
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
	switch( m_port->data_type )
	{
		case TOGGLED:
			return( static_cast<LADSPA_Data>( m_toggle->isChecked() ) );
		default:
			return( static_cast<LADSPA_Data>( m_knob->value() ) );
	}
}


#include "ladspa_control.moc"

#endif

#endif
