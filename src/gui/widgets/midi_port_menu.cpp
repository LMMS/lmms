/*
 * midi_port_menu.cpp - a menu for subscribing a midiPort to several external
 *                      MIDI ports
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#include "midi_port_menu.h"
#include "gui_templates.h"



midiPortMenu::midiPortMenu( midiPort::Modes _mode ) :
	modelView( NULL, this ),
	m_mode( _mode )
{
	setFont( pointSize<9>( font() ) );
	connect( this, SIGNAL( triggered( QAction * ) ),
			this, SLOT( activatedPort( QAction * ) ) );
}




midiPortMenu::~midiPortMenu()
{
}




void midiPortMenu::modelChanged( void )
{
	midiPort * mp = castModel<midiPort>();
	if( m_mode == midiPort::Input )
	{
		connect( mp, SIGNAL( readablePortsChanged() ),
				this, SLOT( updatePorts() ) );
	}
	else if( m_mode == midiPort::Output )
	{
		connect( mp, SIGNAL( writablePortsChanged() ),
			this, SLOT( updatePorts() ) );
	}
	updatePorts();
}




void midiPortMenu::activatedPort( QAction * _item )
{
	if( m_mode == midiPort::Input )
	{
		castModel<midiPort>()->subscribeReadablePort( _item->text(),
							_item->isChecked() );
	}
	else if( m_mode == midiPort::Output )
	{
		castModel<midiPort>()->subscribeWritablePort( _item->text(),
							_item->isChecked() );
	}
}




void midiPortMenu::updatePorts( void )
{
	midiPort * mp = castModel<midiPort>();
	const midiPort::map & map = ( m_mode == midiPort::Input ) ?
				mp->readablePorts() : mp->writablePorts();
	clear();
	for( midiPort::map::const_iterator it = map.begin();
							it != map.end(); ++it )
	{
		QAction * a = addAction( it.key() );
		a->setCheckable( TRUE );
		a->setChecked( it.value() );
	}
}



#include "moc_midi_port_menu.cxx"

