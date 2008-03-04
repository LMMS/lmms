/*
 * ladspa_control_view.cpp - widget for controlling a LADSPA port
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


#include <QtGui/QLayout>

#include "ladspa_control.h"
#include "ladspa_control_view.h"
#include "ladspa_base.h"
#include "led_checkbox.h"
#include "tempo_sync_knob.h"
#include "tooltip.h"


ladspaControlView::ladspaControlView( QWidget * _parent,
						ladspaControl * _ctl ) :
	QWidget( _parent ),
	modelView( _ctl ),
	m_ctl( _ctl )
{
	QHBoxLayout * layout = new QHBoxLayout( this );
	layout->setMargin( 0 );
	layout->setSpacing( 0 );

	ledCheckBox * link = NULL;

	if( m_ctl->m_link )
	{
		link = new ledCheckBox( "", this );
		link->setModel( &m_ctl->m_linkEnabledModel );
		toolTip::add( link, tr( "Link channels" ) );
		layout->addWidget( link );
	}

	knob * knb = NULL;

	switch( m_ctl->getPort()->data_type )
	{
		case TOGGLED:
		{
			ledCheckBox * toggle = new ledCheckBox(
						m_ctl->getPort()->name, this,
							QString::null,
							ledCheckBox::GREEN );
			toggle->setModel( m_ctl->getToggledModel() );
			layout->addWidget( toggle );
			if( link != NULL )
			{
				setFixedSize( link->width() + toggle->width(),
							toggle->height() );
			}
			else
			{
				setFixedSize( toggle->width(),
							toggle->height() );
			}
			break;
		}

		case INTEGER:
		case FLOAT:
			knb = new knob( knobBright_26, this,
						m_ctl->getPort()->name );
			break;

		case TIME:
			knb = new tempoSyncKnob( knobBright_26, this,
						m_ctl->getPort()->name );
			break;

		default:
			break;
	}

	if( knb != NULL )
	{
		if( m_ctl->getPort()->data_type != TIME )
		{
			knb->setModel( m_ctl->getKnobModel() );
		}
		else
		{
			knb->setModel( m_ctl->getTempoSyncKnobModel() );
		}
		knb->setLabel( m_ctl->getPort()->name );
		knb->setHintText( tr( "Value:" ) + " ", "" );
		knb->setWhatsThis( tr( "Sorry, no help available." ) );
		layout->addWidget( knb );
		if( link != NULL )
		{
			setFixedSize( link->width() + knb->width(),
						knb->height() );
		}
		else
		{
			setFixedSize( knb->width(), knb->height() );
		}
	}
}




ladspaControlView::~ladspaControlView()
{
}




