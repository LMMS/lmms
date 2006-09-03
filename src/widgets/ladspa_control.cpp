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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "ladspa_manager.h"
#ifdef LADSPA_SUPPORT

#ifdef QT4

#include <QtGui/QWhatsThis>

#else

#include <qwhatsthis.h>

#endif


#include "ladspa_control.h"
#include "ladspa_effect.h"
#include "tooltip.h"
#include "tempo_sync_knob.h"


ladspaControl::ladspaControl( QWidget * _parent, 
				port_desc_t * _port, 
				engine * _engine, 
				track * _track,
			    	bool _link) :
	QWidget( _parent
#ifdef QT3
			, "ladspaControl"
#endif
					),
	journallingObject( _engine ),
	m_port( _port ),
	m_track( _track ),
	m_link( NULL ),
	m_toggle( NULL ),
	m_knob( NULL )
{
	m_layout = new QHBoxLayout( this
#ifdef QT3
					, 0, 0, "ladspaControlLayout"
#endif
					);
	
	if( _link )
	{
		m_link = new ledCheckBox( "", this, "", eng(), m_track );
		m_link->setChecked( FALSE );
		connect( m_link, SIGNAL( toggled( bool ) ),
			 this, SLOT( portLink( bool ) ) );
		m_layout->addWidget( m_link );
		toolTip::add( m_link, tr( "Link channels" ) );
	}
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle = new ledCheckBox( m_port->name, this, "", 
					eng(), m_track,
					ledCheckBox::GREEN );
			connect( m_toggle, SIGNAL( toggled( bool ) ),
				 this, SLOT( ledChange( bool ) ) );
			setFixedSize( m_toggle->width(), m_toggle->height() );
			if( m_port->def == 1.0f )
			{
				m_toggle->setChecked( TRUE );
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
			m_knob = new knob( knobBright_26, this, 
					   m_port->name, eng(), m_track);
			connect( m_knob, SIGNAL( valueChanged( float ) ),
				 this, SLOT( knobChange( float ) ) );
			m_knob->setLabel( m_port->name );
			m_knob->setRange( static_cast<int>( m_port->max ), 
					  static_cast<int>( m_port->min ), 
					  1 + static_cast<int>( m_port->max - 
							  m_port->min ) / 400 );
			m_knob->setInitValue( 
					static_cast<int>( m_port->def ) );
			setFixedSize( m_knob->width(), m_knob->height() );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
#ifdef QT4
			m_knob->setWhatsThis(
#else
			QWhatsThis::add( m_knob,
#endif
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
			m_knob = new knob( knobBright_26, this, 
					   m_port->name, eng(), m_track);
			connect( m_knob, SIGNAL( valueChanged( float ) ), 
				 this, SLOT( knobChange( float ) ) );
			m_knob->setLabel( m_port->name );
			m_knob->setRange( m_port->min, m_port->max, 
					  ( m_port->max - 
						m_port->min ) / 400.0f );
			m_knob->setInitValue( m_port->def );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
#ifdef QT4
			m_knob->setWhatsThis(
#else
			QWhatsThis::add( m_knob,
#endif
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
			m_knob = new tempoSyncKnob( knobBright_26, this, 
						m_port->name, eng(), m_track);
			connect( m_knob, SIGNAL( valueChanged( float ) ), 
					this, SLOT( knobChange( float ) ) );
			m_knob->setLabel( m_port->name );
			m_knob->setRange( m_port->min, m_port->max, 
					  ( m_port->max - 
						m_port->min ) / 400.0f );
			m_knob->setInitValue( m_port->def );
			m_knob->setHintText( tr( "Value:" ) + " ", "" );
#ifdef QT4
			m_knob->setWhatsThis(
#else
			QWhatsThis::add( m_knob,
#endif
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
	
	if( m_processLock.tryLock() )
	{
		switch( m_port->data_type )
		{
			case TOGGLED:
				value = static_cast<LADSPA_Data>( 
						m_toggle->isChecked() );
				break;
			case INTEGER:
			case FLOAT:
			case TIME:
				value = static_cast<LADSPA_Data>( 
							m_knob->value() );
				break;		
			default:
				printf( 
				"ladspaControl::getValue BAD BAD BAD\n" );
				break;
		}
		m_processLock.unlock();
	}
	
	return( value );
}




void ladspaControl::setValue( LADSPA_Data _value )
{
	m_processLock.lock();
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->setChecked( static_cast<bool>( _value ) );
			break;
		case INTEGER:
			m_knob->setValue( static_cast<int>( _value ) );
			break;
		case FLOAT:
		case TIME:
			m_knob->setValue( static_cast<float>( _value ) );
			break;
		default:
			printf("ladspaControl::setValue BAD BAD BAD\n");
			break;
	}
	
	m_processLock.unlock();
}




void FASTCALL ladspaControl::saveSettings( QDomDocument & _doc, 
					   QDomElement & _this, 
					   const QString & _name )
{
	m_processLock.lock();
	
	if( m_link != NULL )
	{
		m_link->saveSettings( _doc, _this, _name + "link" );
	}
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->saveSettings( _doc, _this, _name );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			m_knob->saveSettings( _doc, _this, _name );
			break;
		default:
			printf("ladspaControl::saveSettings BAD BAD BAD\n");
			break;
	}
	
	m_processLock.unlock();
}



void FASTCALL ladspaControl::loadSettings( const QDomElement & _this, 
					   const QString & _name )
{
	m_processLock.lock();
	
	if( m_link != NULL )
	{
		m_link->loadSettings( _this, _name + "link" );
	}
	switch( m_port->data_type )
	{
		case TOGGLED:
			m_toggle->loadSettings( _this, _name );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			m_knob->loadSettings( _this, _name );
			break;
		default:
			printf("ladspaControl::loadSettings BAD BAD BAD\n");
			break;
	}
	
	m_processLock.unlock();
}




void FASTCALL ladspaControl::linkControls( ladspaControl * _control )
{
	m_processLock.lock();
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			ledCheckBox::linkObjects( m_toggle, _control->getToggle() );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			knob::linkObjects( m_knob, _control->getKnob() );
			break;
		default:
			break;
	}
	
	m_processLock.unlock();
}




void ladspaControl::ledChange( bool _state )
{
	emit( changed( m_port->port_id, static_cast<LADSPA_Data>( _state ) ) );
}




void ladspaControl::knobChange( float _value )
{
	emit( changed( m_port->port_id, static_cast<LADSPA_Data>( _value ) ) );
}




void FASTCALL ladspaControl::unlinkControls( ladspaControl * _control )
{
	m_processLock.lock();
	
	switch( m_port->data_type )
	{
		case TOGGLED:
			ledCheckBox::unlinkObjects( m_toggle, _control->getToggle() );
			break;
		case INTEGER:
		case FLOAT:
		case TIME:
			knob::unlinkObjects( m_knob, _control->getKnob() );
			break;
		default:
			break;
	}
	
	m_processLock.unlock();
}




void ladspaControl::portLink( bool _state )
{
	emit( linkChanged( m_port->control_id, _state ) );
}




void FASTCALL ladspaControl::setLink( bool _state )
{
	if( m_link != NULL )
	{
		m_link->setChecked( _state );
	}
}



#include "ladspa_control.moc"

#endif

#endif
