/*
 * LadspaControls.cpp - model for LADSPA plugin controls
 *
 * Copyright (c) 2008-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include <QtXml/QDomElement>

#include "LadspaEffect.h"


LadspaControls::LadspaControls( LadspaEffect * _eff ) :
	EffectControls( _eff ),
	m_effect( _eff ),
	m_processors( _eff->processorCount() ),
	m_noLink( false ),
	m_stereoLinkModel( true, this )
{

	connect( &m_stereoLinkModel, SIGNAL( dataChanged() ),
				this, SLOT( updateLinkStatesFromGlobal() ) );

	multi_proc_t controls = m_effect->getPortControls();
	m_controlCount = controls.count();

	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		control_list_t p;

		const bool linked_control = ( m_processors > 1 && proc == 0 );

		for( multi_proc_t::Iterator it = controls.begin(); it != controls.end(); it++ )
		{
			if( (*it)->proc == proc )
			{
				(*it)->control = new LadspaControl( this, *it,
							linked_control );

				p.append( (*it)->control );

				if( linked_control )
				{
					connect( (*it)->control, SIGNAL( linkChanged( int, bool ) ),
								this, SLOT( linkPort( int, bool ) ) );
				}
			}
		}

		m_controls.append( p );
	}

	// now link all controls
	if( m_processors > 1 )
	{
		for( multi_proc_t::Iterator it = controls.begin(); 
						it != controls.end(); it++ )
		{
			if( (*it)->proc == 0 )
			{
				linkPort( ( *it )->control_id, true );
			}
		}
	}
}




LadspaControls::~LadspaControls()
{
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		m_controls[proc].clear();
	}
	m_controls.clear();
}




void LadspaControls::saveSettings( QDomDocument & _doc, QDomElement & _this )
{
	if( m_processors > 1 )
	{
		_this.setAttribute( "link", m_stereoLinkModel.value() );
	}
	
	multi_proc_t controls = m_effect->getPortControls();
	_this.setAttribute( "ports", controls.count() );
	for( multi_proc_t::Iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->saveSettings( _doc, _this, n );
	}
}




void LadspaControls::loadSettings( const QDomElement & _this )
{
	if( m_processors > 1 )
	{
		m_stereoLinkModel.setValue( _this.attribute( "link" ).toInt() );
	}
	
	multi_proc_t controls = m_effect->getPortControls();
	for( multi_proc_t::Iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->loadSettings( _this, n );
	}
}




void LadspaControls::linkPort( int _port, bool _state )
{
	LadspaControl * first = m_controls[0][_port];
	if( _state )
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->linkControls( m_controls[proc][_port] );
		}
	}
	else
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->unlinkControls( m_controls[proc][_port] );
		}
		m_noLink = true;
		m_stereoLinkModel.setValue( false );
	}
}



void LadspaControls::updateLinkStatesFromGlobal()
{
	if( m_stereoLinkModel.value() )
	{
		for( int port = 0; port < m_controlCount / m_processors; port++ )
		{
			m_controls[0][port]->setLink( true );
		}
	}
	else if( !m_noLink )
	{
		for( int port = 0; port < m_controlCount / m_processors; port++ )
		{
			m_controls[0][port]->setLink( false );
		}
	}

	// if global channel link state has changed, always ignore link
	// status of individual ports in the future
	m_noLink = false;
}


#include "moc_LadspaControls.cxx"

