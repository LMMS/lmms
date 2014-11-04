/*
 * MidiPortMenu.cpp - a menu for subscribing a MidiPort to several external
 *                      MIDI ports
 *
 * Copyright (c) 2008-2009 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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

#include "MidiPortMenu.h"
#include "gui_templates.h"



MidiPortMenu::MidiPortMenu( MidiPort::Modes _mode ) :
	ModelView( NULL, this ),
	m_mode( _mode )
{
	setFont( pointSize<9>( font() ) );
	connect( this, SIGNAL( triggered( QAction * ) ),
			this, SLOT( activatedPort( QAction * ) ) );
}




MidiPortMenu::~MidiPortMenu()
{
}




void MidiPortMenu::modelChanged()
{
	MidiPort * mp = castModel<MidiPort>();
	if( m_mode == MidiPort::Input )
	{
		connect( mp, SIGNAL( readablePortsChanged() ),
				this, SLOT( updateMenu() ) );
	}
	else if( m_mode == MidiPort::Output )
	{
		connect( mp, SIGNAL( writablePortsChanged() ),
				this, SLOT( updateMenu() ) );
	}
	updateMenu();
}




void MidiPortMenu::activatedPort( QAction * _item )
{
	if( m_mode == MidiPort::Input )
	{
		castModel<MidiPort>()->subscribeReadablePort( _item->text(),
							_item->isChecked() );
	}
	else if( m_mode == MidiPort::Output )
	{
		castModel<MidiPort>()->subscribeWritablePort( _item->text(),
							_item->isChecked() );
	}
}




void MidiPortMenu::updateMenu()
{
	MidiPort * mp = castModel<MidiPort>();
	const MidiPort::Map & map = ( m_mode == MidiPort::Input ) ?
				mp->readablePorts() : mp->writablePorts();
	clear();
	for( MidiPort::Map::ConstIterator it = map.begin();
							it != map.end(); ++it )
	{
		QAction * a = addAction( it.key() );
		a->setCheckable( true );
		a->setChecked( it.value() );
	}
}



#include "moc_MidiPortMenu.cxx"

